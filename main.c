/* Tiny TFT Graphics Library - see http://www.technoblogy.com/show?L6I

   David Johnson-Davies - www.technoblogy.com - 13th June 2019
   ATtiny85 @ 8 MHz (internal oscillator; BOD disabled)
   
   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license: 
   http://creativecommons.org/licenses/by/4.0/
*/

#define F_CPU 8000000UL                 // set the CPU clock
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <math.h>
#include <avr/eeprom.h>
#include <string.h>
#include <util/delay.h>
//#include "_24LCxx.h"
#include "I2C.h"
#include "st7735.h"

#define BACKLIGHT_ON PORTB |= (1<<PB2)
#define BACKLIGHT_OFF PORTB &= ~(1<<PB2)						

#define LED_OFF PORTC &= ~(1<<PC3)
#define LED_ON PORTC |= (1<<PC3)

//Buttons 
#define T1 	!(PIND & (1<<PD6))
#define T2	!(PIND & (1<<PD2))
#define T3	!(PIND & (1<<PD5))

/* some RGB color definitions                                                 */
#define BLACK        0x0000      
#define RED          0x001F      
#define GREEN        0x07E0      
#define YELLOW       0x07FF      
#define BLUE         0xF800      
#define CYAN         0xFFE0      
#define White        0xFFFF     
#define BLUE_LIGHT   0xFD20      
#define TUERKISE     0xAFE5      
#define VIOLET       0xF81F		
#define WHITE		0xFFFF

#define SEK_POS 10,110

#define RELOAD_ENTPRELL 1 

//#define EEDEVADR 0b10100000

// Pins already defined in st7735.c
extern int const DC;
extern int const MOSI;
extern int const SCK;
extern int const CS;
// Text scale and plot colours defined in st7735.c
extern int fore; 		// foreground colour
extern int back;      	// background colour
extern int scale;     	// Text size


volatile uint8_t ms10,ms100,sec,min, entprell;


char stringbuffer[20]; // buffer to store string 


ISR (TIMER1_COMPA_vect);

uint8_t ISR_zaehler = 0;
uint8_t ms = 0;
uint8_t second = 0;
uint16_t minute = 0;
ISR (TIMER0_OVF_vect)
{
	TCNT0 = 0;
	ISR_zaehler++;
	if(ISR_zaehler == 12)
	{
		ms++;
		ISR_zaehler = 0;
		if (ms == 10)
		{
			second ++;
			ms = 0;
			if (second == 60)
			{
				minute ++;
				second = 0;
			}
		}
	}
}//End of ISR

void SPI_MasterTransmit(uint8_t cData)
{/* Start transmission */
	SPDR = cData;
	/* Wait for transmission complete */
	while(!(SPSR & (1<<SPIF)))
	;
}

void PlotNumb(char Param[], int32_t number);

const uint8_t size = 128;

int32_t t_fine;

int32_t BME280_compensate_T_double(int32_t rawTemp)
{ 
	uint16_t dig_T1 = 0;
	int16_t dig_T2 = 0;
	int16_t dig_T3 = 0;
	
	uint8_t temp1 = 0;
	uint8_t temp2 = 0;
	
	I2C_Read8(0xee, 0x88, &temp2);
	I2C_Read8(0xee, 0x89, &temp1);
	dig_T1 = (temp1<<8) | temp2;
	
	temp1 = 0;
	temp2 = 0;
	
	I2C_Read8(0xee, 0x8A, &temp2);
	I2C_Read8(0xee, 0x8B, &temp1);
	dig_T2 = (temp1<<8) | temp2;
	
	temp1 = 0;
	temp2 = 0;
	
	I2C_Read8(0xee, 0x8C, &temp2);
	I2C_Read8(0xee, 0x8D, &temp1);
	dig_T3 = (temp1<<8) | temp2;
	
	int32_t var1;
	int32_t var2;
	int32_t T;
	var1  = ((((rawTemp>>3) - ((int32_t)dig_T1<<1))) * ((int32_t)dig_T2)) >> 11; 
	var2  = (((((rawTemp>>4) - ((int32_t)dig_T1)) * ((rawTemp>>4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14; 
	t_fine = (var1 + var2);
	T  = ((t_fine * 5 + 128) >> 8);
	return T;
}

uint8_t initRTC(void)
{
	uint8_t status = 0;
	I2CStart();	
	status = I2CReadStatus();							
	if(status != 0x08)return status; 	
	I2CWrite(0b11011110);			//SRAM Write
	status = I2CReadStatus();		
	if(status != 0x18)return status;		
	I2CWrite(0x07);				//angesprochenes Register
	status = I2CReadStatus();
	if(status != 0x28)return status;		
	I2CWrite(0x00);					//zu schreibender Wert
	status = I2CReadStatus();
	if(status != 0x28)return status;	
	status = I2CReadStatus();	
	I2CStop();
	return status;
}

uint8_t getSec(uint8_t *secondOut)
{
	uint8_t RawSecData = 0;
	uint8_t error = 0;
	error = I2C_Read8(0b11011110, 0x00, &RawSecData);
	*secondOut = (RawSecData & 0x0F) + (10 * ((RawSecData & 0x70)>>4));
	return error;
}

int main(void)
{
	DDRB |= (1<<DC) | (1<<CS) | (1<<MOSI) |( 1<<SCK); 	// All outputs
	PORTB = (1<<SCK) | (1<<CS) | (1<<DC);          		// clk, dc, and cs high
	DDRB |= (1<<PB2);									//lcd Backlight output
	PORTB |= (1<<CS) | (1<<PB2);                  		// cs high
	DDRC |= (1<<PC3);									//Reset Output
	DDRD |= (1<<PD7);									//Reset Output
	PORTD |= (1<<PD7);	
									//Reset High
	DDRD &= ~((1<<PD6) | (1<<PD2) | (1<<PD5)); 	//Taster 1-3
	PORTD |= ((1<<PD6) | (1<<PD2) | (1<<PD5)); 	//PUllups für Taster einschalten
	
		//Init SPI		CLK/2
	//==================================================================
	SPCR |= (1<<SPE) | (1<<MSTR);
	SPSR |= (1<<SPI2X);
	//==================================================================
	
		//Timer 1 Configuration
	OCR1A = 1249;	//OCR1A = 0x3D08;==1sec
	
    TCCR1B |= (1 << WGM12);
    // Mode 4, CTC on OCR1A

    TIMSK1 |= (1 << OCIE1A);
    //Set interrupt on compare match

    TCCR1B |= (1 << CS11) | (1 << CS10);
    // set prescaler to 64 and start the timer

    sei();
    // enable interrupts
    
    ms10=0;
    ms100=0;
    sec=0;
    min=0;
    entprell=0;
	
	BACKLIGHT_ON;
	LED_ON;

	setup();
	
	//Konfiguration Timer Overflow
	//==================================================================
	TCCR0A	= 0x00;
	TCCR0B	= 0x04;
	TIMSK0	|= (1 << TOIE0);
	TIFR0 |= (1 << TOV0);
	sei();
	//==================================================================
	
	//~ uint8_t buff = 0x80;
	//~ int32_t TempData = 0x80;
	
	uint8_t data = 13;
	uint8_t sec = 0;
	uint8_t error = 0;
	I2CInit();
	
	//error = initRTC();
	
	if(error)
	{
		PlotNumb("0x%X, ", error);
		while(1);
	}
	
	//~ error = I2C_Write8(0xee, 0xF5, 0x00);
	//~ error = I2C_Write8(0xee, 0xF4, 0b10100111);
	//~ error = I2C_Write8(0xee, 0xF2, 0x01);
	
	error = I2C_Write8(0b11011110, 0x00, 0x80);
	
	while(1)
	{	
		
		//~ MoveTo(0,0);
		//~ PlotNumb("%d, ", BME280_compensate_T_double(TempData));
		//~ PlotString("    ");
		//~ TempData = 0;
		//~ buff = 0;
		//~ error = I2C_Read8(0xee, 0xFC, &buff);
		//~ TempData |= ((int32_t)buff);
		//~ buff = 0;
		//~ error = I2C_Read8(0xee, 0xFB, &buff);
		//~ TempData |= ((int32_t)buff<<8);
		//~ buff = 0;
		//~ error = I2C_Read8(0xee, 0xFA, &buff);
		//~ TempData |= ((int32_t)buff<<16);
		//~ //PlotNumb("0x%X, ", error);
		
		MoveTo(0,0);
		error = getSec(&sec);
		PlotNumb("0x%X, ", error);
		PlotNumb("%d, ", sec);
	}
	  
	for (;;)
	{

	}//end of for()
}//end of main

void PlotNumb(char Param[], int32_t number)
{
	char buffer[20] = {0};	//define and initialise buffer variable
	sprintf(buffer, Param, number);	//convert value to string and store in buffer
	PlotString(buffer);	//Draw string in buffer
}

ISR (TIMER1_COMPA_vect)
{
	
}
