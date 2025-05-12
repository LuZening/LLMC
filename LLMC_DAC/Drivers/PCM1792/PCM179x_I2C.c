
#include "pcm179x.h"
#include "stm32h7xx_hal.h"

extern I2C_HandleTypeDef hi2c1;
int PCM179x_write_reg(pcm179x_t *p,  uint8_t addr, uint8_t d)
{
	int r;
	uint8_t buf[3];
	buf[0] = addr;
	buf[1] = d;
	r = HAL_I2C_Master_Transmit(&hi2c1, p->addr_i2c, buf, 2, 10);
	return r;
}

int PCM179x_read_reg(pcm179x_t *p, uint8_t addr, uint8_t* pd)
{
	int r;
	r = HAL_I2C_Master_Transmit(&hi2c1, p->addr_i2c, &addr, 1, 10);
	if(r != HAL_OK ) goto READ_REG_FAILED;
	r = HAL_I2C_Master_Receive(&hi2c1, p->addr_i2c, pd, 1, 10);
	if(r != HAL_OK ) goto READ_REG_FAILED;

READ_REG_FAILED:
	return r;

}
