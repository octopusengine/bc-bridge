#include <bc/i2c/sys.h>
#include <stm32l0xx_hal.h>

extern I2C_HandleTypeDef hi2c1;

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

	if (HAL_I2C_Mem_Write(&hi2c1, transfer->device_address << 1, transfer->address, transfer->address_16_bit ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT, transfer->buffer, transfer->length, 0xFFFFFFFF) != HAL_OK)
	{
		return false;
	}

	*communication_fault = false;

	return true;
}

static bool _bc_app_i2c_tag_read(bc_tag_transfer_t *transfer, bool *communication_fault)
{
	*communication_fault = true;

	if (HAL_I2C_Mem_Read(&hi2c1, transfer->device_address << 1, transfer->address, transfer->address_16_bit ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT, transfer->buffer, transfer->length, 0xFFFFFFFF) != HAL_OK)
	{
		return false;
	}

	*communication_fault = false;

	return true;
}
