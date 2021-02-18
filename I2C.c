#include "I2C.h"

void I2CInit(void)
{
	TWSR = 0x00;
	TWBR = 0x0C;
	TWCR = (1<<TWEN);
}

void I2CStart(void)
{
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	while((TWCR & (1<<TWINT)) == 0);
}

void I2CStop(void)
{
	TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);
}

void I2CWrite(uint8_t data)
{
	TWDR = data;
	TWCR = ((1<<TWINT) | (1<<TWEN));
	while((TWCR & (1<<TWINT)) == 0);
}

uint8_t I2CReadACK(void)
{
	TWCR = ((1<<TWINT) | (1<<TWEN)| (1<<TWEA));
	while((TWCR & (1<<TWINT)) == 0);
	return TWDR;
}

uint8_t I2CReadNACK(void)
{
	TWCR = ((1<<TWINT) | (1<<TWEN));
	while((TWCR & (1<<TWINT)) == 0);
	return TWDR;
}

uint8_t I2CReadStatus(void)
{
	uint8_t status = 0;
	status = TWSR & 0xF8;
	return status;
}

uint8_t I2C_Write8(uint8_t ID, uint8_t adress, uint8_t data)
{
	uint8_t status = 0;
	I2CStart();
	status = I2CReadStatus();
	if(status != 0x08)
	{
		return status;
	}
	I2CWrite(ID);
	status = I2CReadStatus();
	if(status != 0x18)
	{
		return status;
	}
	I2CWrite(adress);
	status = I2CReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	I2CWrite(data);
	status = I2CReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	
	I2CStop();
	return 0x00;
}

uint8_t I2C_Read8(uint8_t ID, uint8_t adress, uint8_t *data)
{
	uint8_t status = 0;
	I2CStart();
	status = I2CReadStatus();
	if(status != 0x08)
	{
		return status;
	}
	I2CWrite(ID);
	status = I2CReadStatus();
	if(status != 0x18)
	{
		return status;
	}
	I2CWrite(adress);
	status = I2CReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	I2CStart();
	status = I2CReadStatus();
	if(status != 0x10)
	{
		return status;
	}
	I2CWrite(ID|1);
	status = I2CReadStatus();
	if(status != 0x40)
	{
		return status;
	}
	*data = I2CReadNACK();
	status = I2CReadStatus();
	if(status != 0x58)
	{
		return status;
	}
	I2CStop();
	return 0x00;
}

uint8_t I2C_Write16(uint8_t ID, uint16_t adress, uint16_t data)
{
	uint8_t status = 0;
	I2CStart();
	status = I2CReadStatus();
	if(status != 0x08)
	{
		return status;
	}
	I2CWrite(ID);
	status = I2CReadStatus();
	if(status != 0x18)
	{
		return status;
	}
	I2CWrite((uint8_t)(adress >> 8));
	status = I2CReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	I2CWrite((uint8_t)(adress));
	status = I2CReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	
	
	I2CWrite((uint8_t)(data >> 8));
	status = I2CReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	I2CWrite((uint8_t)(data));
	status = I2CReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	
	I2CStop();
	return 0x00;
}

uint8_t I2C_Read16(uint8_t ID, uint16_t adress, uint16_t *data)
{
	uint8_t status = 0;
	uint16_t output = 0;
	I2CStart();
	status = I2CReadStatus();
	if(status != 0x08)
	{
		return status;
	}
	I2CWrite(ID);
	status = I2CReadStatus();
	if(status != 0x18)
	{
		return status;
	}
	I2CWrite((uint8_t)(adress >> 8));
	status = I2CReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	I2CWrite((uint8_t)(adress));
	status = I2CReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	I2CStart();
	status = I2CReadStatus();
	if(status != 0x10)
	{
		return status;
	}
	I2CWrite(ID|1);
	status = I2CReadStatus();
	if(status != 0x40)
	{
		return status;
	}
	output |= (uint16_t)(I2CReadACK())<<8;
	status = I2CReadStatus();
	if(status != 0x50)
	{
		return status;
	}
	output |= (0x0FF & I2CReadNACK());
	status = I2CReadStatus();
	if(status != 0x58)
	{
		return status;
	}
	*data = output;
	I2CStop();
	return 0x00;
}
