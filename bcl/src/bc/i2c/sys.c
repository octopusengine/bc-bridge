#include <bc/i2c/sys.h>
#include <ft260.h>

static bool _bc_sys_i2c_tag_write(bc_tag_transfer_t *transfer, bool *communication_fault);
static bool _bc_sys_i2c_tag_read(bc_tag_transfer_t *transfer, bool *communication_fault);

bc_tag_interface_t *bc_i2c_sys_get_tag_interface(void)
{
	static const bc_tag_interface_t interface =
	{
		.write = _bc_sys_i2c_tag_write,
		.read = _bc_sys_i2c_tag_read
	};

	return (bc_tag_interface_t *) &interface;
}

static bool _bc_sys_i2c_tag_write(bc_tag_transfer_t *transfer, bool *communication_fault)
{
	*communication_fault = true;

	ft260_i2c_set_bus(FT260_I2C_BUS_0);

	// TODO 
	uint8_t buffer[1+transfer->length];

	buffer[0] = transfer->address;
	memcpy(buffer + 1,transfer->buffer, transfer->length);
	if (!ft260_i2c_write(transfer->device_address, buffer, 1 + transfer->length ))
	{
		return false;
	}

	*communication_fault = false;

	return true;
}

static bool _bc_sys_i2c_tag_read(bc_tag_transfer_t *transfer, bool *communication_fault)
{
	uint8_t buffer[1];

	*communication_fault = true;

	ft260_i2c_set_bus(FT260_I2C_BUS_0);

	buffer[0] = transfer->address;

	ft260_i2c_write(transfer->device_address, buffer, 1);

	if (!ft260_i2c_read(transfer->device_address, transfer->buffer, transfer->length))
	{
		return false;
	}

	*communication_fault = false;

	return true;
}