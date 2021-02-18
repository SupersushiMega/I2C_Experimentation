#ifndef I2C_H
#define I2C_H
#endif

#include <avr/io.h>

void I2CInit(void);
void I2CStart(void);
void I2CStop(void);
void I2CWrite(uint8_t data);

uint8_t I2CReadNACK(void);
uint8_t I2CReadACK(void);
uint8_t I2CReadStatus(void);


//ID: adress of Part
//adress: data adress in EEpprom
//data: data to store
//*data: variable to store read data into
//Function output: error code
//======================================================================
uint8_t I2C_Write8(uint8_t ID, uint8_t adress, uint8_t data);
uint8_t I2C_Read8(uint8_t ID, uint8_t adress, uint8_t *data);
		 
uint8_t I2C_Write16(uint8_t ID, uint16_t adress, uint16_t data);
uint8_t I2C_Read16(uint8_t ID, uint16_t adress, uint16_t *data);
//======================================================================
