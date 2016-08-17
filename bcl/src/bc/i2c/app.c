#include <bc/i2c/sys.h>
#include <ft260.h>

static bool _bc_app_i2c_tag_write(bc_tag_transfer_t *transfer, bool *communication_fault);
static bool _bc_app_i2c_tag_read(bc_tag_transfer_t *transfer, bool *communication_fault);

bc_tag_interface_t *bc_i2c_app_get_tag_interface(void)
{
	static const bc_tag_interface_t interface =
	{
		.write = _bc_app_i2c_tag_write,
		.read = _bc_app_i2c_tag_read
	};

	return (bc_tag_interface_t *) &interface;
}

static bool _bc_app_i2c_tag_write(bc_tag_transfer_t *transfer, bool *communication_fault)
{
	*communication_fault = true;

	ft260_i2c_set_bus(APP);

	unsigned char buf[1+transfer->length];
	buf[0] = transfer->address;
	memcpy(buf+1,transfer->buffer,transfer->length);
	int res = ft260_i2c_write(transfer->device_address, buf, 1+transfer->length );
	if(res<1){
		return false;
	}

	*communication_fault = false;

	return true;
}

static bool _bc_app_i2c_tag_read(bc_tag_transfer_t *transfer, bool *communication_fault)
{
	*communication_fault = true;

	ft260_i2c_set_bus(APP);

	unsigned char buf[1];
    buf[0] = transfer->address;
    ft260_i2c_write(transfer->device_address, buf, 1);
    int res = ft260_i2c_read(transfer->device_address, transfer->buffer, transfer->length);
	if(res<transfer->length){
		return false;
	}

	*communication_fault = false;

	return true;
}
