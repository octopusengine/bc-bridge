#include "task_i2c.h"
#include "bc_log.h"
#include "task.h"
#include "bc_talk.h"
#include "bc_bridge.h"

static uint8_t _task_i2c_fifo_size(task_i2c_parameters_t *parameters);

void *task_i2c_worker(void *task_parameter)
{
    task_worker_t *self = (task_worker_t *) task_parameter;
    task_i2c_parameters_t *parameters = (task_i2c_parameters_t *)self->parameters;

    bc_log_info("task_i2c_worker: started instance ");

    task_worker_set_init_done(self);

    task_i2c_action_t *action;

    char slaves[1024] = "";
    char address_text[3];
    char topic[32];
    bool add_comma;
    uint8_t address;

    bc_i2c_transfer_t transfer;

    while (true)
    {
        bc_os_semaphore_get(&self->semaphore);
//        bc_os_semaphore_timed_get(&self->semaphore, 5000);

        bc_log_debug("task_i2c_worker: wake up signal");

        if (task_worker_is_quit_request(self))
        {
            bc_log_debug("task_i2c_worker: quit_request");
            break;
        }

        if ((parameters->head - parameters->tail) != 0 )
        {

            action = &parameters->actions[parameters->tail];

            if(++parameters->tail > TASK_I2C_ACTIONS_LENGTH-1)
            {
                parameters->tail = 0;
            }

            if (action->type == TASK_I2C_ACTION_TYPE_SCAN)
            {

                add_comma=false;
                slaves[0] = 0x00;

                for (address = 1; address < 128; address++)
                {
                    if (bc_bridge_i2c_ping(self->_bridge, action->channel, address))
                    {
                        if (add_comma)
                        {
                            strcat(slaves, ", ");
                        }

                        if (bc_talk_is_clown_device(address))
                        {
                            bc_talk_make_topic(action->channel, address, topic, sizeof(topic));
                            sprintf(address_text, "[\"%02X\",\"%s\"]", address, topic);
                        }
                        else
                        {
                            sprintf(address_text, "[\"%02X\",null]", address);
                        }

                        strcat(slaves, address_text);
                        add_comma = true;
                    }
                }

                bc_talk_publish_begin_auto_subtopic(action->channel, self->_device_address, "/config/scan/ok");
                bc_talk_publish_add_value("slaves", "[%s]", slaves);
                bc_talk_publish_end();

            }
            else if (action->type == TASK_I2C_ACTION_TYPE_WRITE)
            {

                bc_i2c_transfer_init(&transfer);
                transfer.channel = action->attributes->channel;
                transfer.device_address = action->attributes->device_address;
                transfer.address = action->attributes->write.buffer[0];
                transfer.buffer = action->attributes->write.buffer+1;
                transfer.length = action->attributes->write.length-1;

                bc_bridge_i2c_write(self->_bridge, &transfer);

                bc_talk_publish_i2c(action->attributes);

                free( action->attributes->write.buffer );
                free( action->attributes );

            }
            else if (action->type == TASK_I2C_ACTION_TYPE_READ)
            {
                bc_i2c_transfer_init(&transfer);
                transfer.channel = action->attributes->channel;
                transfer.device_address = action->attributes->device_address;
                if (action->attributes->write.length == 0)
                {
                    transfer.address = BC_BRIDGE_I2C_ONLY_READ;
                    action->attributes->read.encoding = BC_TALK_DATA_ENCODING_HEX;
                }
                else
                {
                    transfer.address = action->attributes->write.buffer[0];

                    if (action->attributes->write.length == 2)
                    {
                        transfer.address |= ((uint16_t)action->attributes->write.buffer[1]) << 8;
                        transfer.address_16_bit = true;
                    }

                    action->attributes->read.encoding = action->attributes->write.encoding;
                }

                transfer.length = action->attributes->read_length;
                transfer.buffer = malloc( transfer.length * sizeof(uint8_t) );

                bc_bridge_i2c_read(self->_bridge, &transfer);

                action->attributes->read.buffer = transfer.buffer;
                action->attributes->read.length = transfer.length;

                bc_talk_publish_i2c(action->attributes);

                free( transfer.buffer );
                if (action->attributes->write.buffer != NULL)
                {
                    free(action->attributes->write.buffer);
                }
                free( action->attributes );

            }

        }

    }

    return NULL;
}

bool task_i2c_set_scan(task_info_t *task_info, uint8_t channel)
{
    task_lock(task_info);

    task_i2c_parameters_t *parameters= (task_i2c_parameters_t *)task_info->parameters;

    if (_task_i2c_fifo_size(parameters) == TASK_I2C_ACTIONS_LENGTH )
    {
        task_unlock(task_info);

        bc_log_error("task_i2c_set_scan: full fifo");

        return false;
    }

    parameters->actions[parameters->head].type = TASK_I2C_ACTION_TYPE_SCAN;
    parameters->actions[parameters->head].channel = channel;

    if(++parameters->head > TASK_I2C_ACTIONS_LENGTH-1)
    {
        parameters->head = 0;
    }

    task_unlock(task_info);

    task_semaphore_put(task_info);

    return true;
}

bool task_i2c_set_command(task_info_t *task_info, task_i2c_action_type_t type, bc_talk_i2c_attributes_t *attributes)
{
    task_lock(task_info);

    task_i2c_parameters_t *parameters = (task_i2c_parameters_t *)task_info->parameters;

    if (_task_i2c_fifo_size(parameters) == TASK_I2C_ACTIONS_LENGTH )
    {
        task_unlock(task_info);

        bc_log_error("task_i2c_set_command: full fifo");

        return false;
    }

    parameters->actions[parameters->head].type = type;
    parameters->actions[parameters->head].attributes = attributes;


    if(++parameters->head > TASK_I2C_ACTIONS_LENGTH-1)
    {
        parameters->head = 0;
    }


    task_unlock(task_info);

    task_semaphore_put(task_info);

    return true;
}

static uint8_t _task_i2c_fifo_size(task_i2c_parameters_t *parameters)
{
    if (parameters->head == parameters->tail)
    {
        return 0;
    }
    else if ((parameters->head == (TASK_I2C_ACTIONS_LENGTH - 1) && parameters->tail == 0) || (parameters->head == (parameters->tail - 1)))
    {
        return TASK_I2C_ACTIONS_LENGTH;
    }
    else if (parameters->head < parameters->tail)
    {
        return (parameters->head) + (TASK_I2C_ACTIONS_LENGTH - parameters->tail);
    }
    else
    {
        return parameters->head - parameters->tail;
    }
}

