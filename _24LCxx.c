#include "_24LCxx.h"

void TWIInit(void)
{
	TWSR = 0x00;
	TWBR = 0x0C;
	TWCR = (1<<TWEN);
}

void TWIStart(void)
{
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	while((TWCR & (1<<TWINT)) == 0);
}

void TWIStop(void)
{
	TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);
}

void TWIWrite(uint8_t data)
{
	TWDR = data;
	TWCR = ((1<<TWINT) | (1<<TWEN));
	while((TWCR & (1<<TWINT)) == 0);
}

uint8_t TWIReadACK(void)
{
	TWCR = ((1<<TWINT) | (1<<TWEN)| (1<<TWEA));
	while((TWCR & (1<<TWINT)) == 0);
	return TWDR;
}

uint8_t TWIReadNACK(void)
{
	TWCR = ((1<<TWINT) | (1<<TWEN));
	while((TWCR & (1<<TWINT)) == 0);
	return TWDR;
}

uint8_t TWIReadStatus(void)
{
	uint8_t status = 0;
	status = TWSR & 0xF8;
	return status;
}

uint8_t TWI_EEWrite8(uint16_t adress, uint8_t data)
{
	uint8_t status = 0;
	TWIStart();
	status = TWIReadStatus();
	if(status != 0x08)
	{
		return status;
	}
	TWIWrite(ID);
	status = TWIReadStatus();
	if(status != 0x18)
	{
		return status;
	}
	TWIWrite((uint8_t)(adress >> 8));
	status = TWIReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	TWIWrite((uint8_t)(adress));
	status = TWIReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	TWIWrite(data);
	status = TWIReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	
	TWIStop();
	return 0x00;
}

uint8_t TWI_EERead8(uint16_t adress, uint8_t *data)
{
	uint8_t status = 0;
	TWIStart();
	status = TWIReadStatus();
	if(status != 0x08)
	{
		return status;
	}
	TWIWrite(ID);
	status = TWIReadStatus();
	if(status != 0x18)
	{
		return status;
	}
	TWIWrite((uint8_t)(adress >> 8));
	status = TWIReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	TWIWrite((uint8_t)(adress));
	status = TWIReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	TWIStart();
	status = TWIReadStatus();
	if(status != 0x10)
	{
		return status;
	}
	TWIWrite(ID|1);
	status = TWIReadStatus();
	if(status != 0x40)
	{
		return status;
	}
	*data = TWIReadNACK();
	status = TWIReadStatus();
	if(status != 0x58)
	{
		return status;
	}
	TWIStop();
	return 0x00;
}

uint8_t TWI_EEWrite16(uint16_t adress, uint16_t data)
{
	uint8_t status = 0;
	TWIStart();
	status = TWIReadStatus();
	if(status != 0x08)
	{
		return status;
	}
	TWIWrite(ID);
	status = TWIReadStatus();
	if(status != 0x18)
	{
		return status;
	}
	TWIWrite((uint8_t)(adress >> 8));
	status = TWIReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	TWIWrite((uint8_t)(adress));
	status = TWIReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	
	
	TWIWrite((uint8_t)(data >> 8));
	status = TWIReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	TWIWrite((uint8_t)(data));
	status = TWIReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	
	TWIStop();
	return 0x00;
}

uint8_t TWI_EERead16(uint16_t adress, uint16_t *data)
{
	uint8_t status = 0;
	uint16_t output = 0;
	TWIStart();
	status = TWIReadStatus();
	if(status != 0x08)
	{
		return status;
	}
	TWIWrite(ID);
	status = TWIReadStatus();
	if(status != 0x18)
	{
		return status;
	}
	TWIWrite((uint8_t)(adress >> 8));
	status = TWIReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	TWIWrite((uint8_t)(adress));
	status = TWIReadStatus();
	if(status != 0x28)
	{
		return status;
	}
	TWIStart();
	status = TWIReadStatus();
	if(status != 0x10)
	{
		return status;
	}
	TWIWrite(ID|1);
	status = TWIReadStatus();
	if(status != 0x40)
	{
		return status;
	}
	output |= (uint16_t)(TWIReadACK())<<8;
	status = TWIReadStatus();
	if(status != 0x50)
	{
		return status;
	}
	output |= (0x0FF & TWIReadNACK());
	status = TWIReadStatus();
	if(status != 0x58)
	{
		return status;
	}
	*data = output;
	TWIStop();
	return 0x00;
}
