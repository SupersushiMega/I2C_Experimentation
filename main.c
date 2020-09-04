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
	
	const uint8_t size = 128;
	
	const uint8_t rectWidth = 10;
	const uint8_t rectHeight = 4;
	const uint8_t rectSpeed = 2;
	
	const uint8_t BlockDist = 1;
	
	uint8_t BlockGridSize[2];
	BlockGridSize[0] =	25;
	BlockGridSize[1] =	10;
	
	uint8_t BlockSize[2];
	BlockSize[0] = (size / BlockGridSize[0]);
	BlockSize[1] = ((size / 2) / BlockGridSize[1]);
	
	uint8_t BlocksStatus[BlockGridSize[0]][BlockGridSize[1]];
	int x;
	int y;
	for (x = 0; x < BlockGridSize[0]; x++)
	{
		for (y = 0; y < BlockGridSize[1]; y++)
		{
			BlocksStatus[x][y] = 1;
			MoveTo(x * (BlockSize[0]) + BlockDist, (size / 2) + (y * BlockSize[1]) + BlockDist);
			FillRect(BlockSize[0] - BlockDist, BlockSize[1] - BlockDist);
		}
	}
	int8_t RectX;
	int8_t BallMove[] = {-1, -1};
	uint8_t BallData[] = {size / 2, size / 2, 3};
	RectX = 0;
	while (1)
	{
		//Input and RectangleClear
		//==============================================================
		fore = BLACK;
		MoveTo(RectX,0);
		if (T3)
		{
			FillRect(rectWidth,8);
			RectX += rectSpeed;
		}
		else if (T1)
		{
			FillRect(rectWidth,8);
			RectX -= rectSpeed;
		}
		
		//==============================================================
		
		//Autoplay
		//==============================================================
		if (0)
		{
			if(BallData[0] > (RectX + (rectWidth / 2)))
			{
				FillRect(rectWidth,8);
				RectX += rectSpeed;
			}
			else
			{
				FillRect(rectWidth,8);
				RectX -= rectSpeed;
			}
		}
		
		//==============================================================
		
		//Ball movement and CircleClear
		//==============================================================
		glcd_draw_circle(BallData[0], BallData[1], BallData[2]);
		BallData[0] += BallMove[0];
		BallData[1] += BallMove[1];
		//==============================================================
		
		//Check if Rectangle is Coliding with Wall
		//==============================================================
		if (RectX > size - rectWidth)
		{
			RectX =	size - rectWidth;
		}
		else if (RectX < 0)
		{
			RectX = 0;
		}
		//==============================================================
		
		//Check if Ball is Coliding with Wall
		//==============================================================
		if (BallData[0] > size - BallData[2] || BallData[0] == 0)
		{
			BallMove[0] = -BallMove[0];
		}
		//==============================================================
		
		//Check if Ball is Coliding with Wall or Rectangle
		//==============================================================
		else if (BallData[1] == size - BallData[2] || (BallData[1] < (rectHeight + BallData[2]) && (BallData[0] < (rectWidth + RectX)) && (BallData[0] > RectX)))
		{
			BallMove[1] = -BallMove[1];
		}
		//==============================================================
		
		//Check if Ball is Coliding with Block
		//==============================================================
		for (x = 0; x < BlockGridSize[0]; x++)
		{
			for (y = 0; y < BlockGridSize[1]; y++)
			{
				MoveTo(x * (BlockSize[0]) + BlockDist, (size / 2) + (y * BlockSize[1]) + BlockDist);
				if (BlocksStatus[x][y] == 1)
				{
					fore = BLACK;
					if (BallData[0] + BallData[2] > (x * BlockSize[0]) && BallData[0] - BallData[2] < ((x * BlockSize[0]) + BlockSize[0]) && (((BallData[1] - (size / 2)) == (y * BlockSize[1])) || ((BallData[1] - (size / 2)) == ((y * BlockSize[1]) + BlockSize[1]))))
					{
						BlocksStatus[x][y] = 0;
						BallMove[1] = -BallMove[1];
						FillRect(BlockSize[0] - BlockDist, BlockSize[1] - BlockDist);
					}
					
					else if ((BallData[0] + BallData[2] == (x * BlockSize[0]) || BallData[0] - BallData[2] == ((x * BlockSize[0]) + BlockSize[0])) && ((BallData[1] - (size / 2)) > (y * BlockSize[1])) && ((BallData[1] - (size / 2)) < ((y * BlockSize[1]) + BlockSize[1])))
					{
						BlocksStatus[x][y] = 0;
						BallMove[0] = -BallMove[0];
						FillRect(BlockSize[0] - BlockDist, BlockSize[1] - BlockDist);
					}
					
					else if ((BallData[0] < (x * BlockSize[0]) && BallData[0] > ((x * BlockSize[0]) + BlockSize[0])) && ((BallData[1] - (size / 2)) > (y * BlockSize[1])) && ((BallData[1] - (size / 2)) < ((y * BlockSize[1]) + BlockSize[1])))
					{
						BlocksStatus[x][y] = 0;
						BallMove[0] = -BallMove[0];
						MoveTo(x * (BlockSize[0]) + BlockDist, (size / 2) + (y * BlockSize[1]) + BlockDist);
						FillRect(BlockSize[0] - BlockDist, BlockSize[1] - BlockDist);
					}
				}
			}
		}
		//==============================================================
		
		fore = WHITE; // White
		
		MoveTo(RectX,0);
		FillRect(rectWidth, 8);
		
		
		
		glcd_draw_circle(BallData[0], BallData[1], BallData[2]);
	}
	
	fore = RED;
	
	
	//~ fore = WHITE; // White
	//~ scale = 2;
	//~ MoveTo(40,80);
	//~ fore = GREEN;
	//~ FillRect(40,40);
	//~ MoveTo(80,80);
	//~ fore = RED;
	//~ FillRect(40,40);
	
	//~ MoveTo(40,40);
	//~ fore = YELLOW;
	//~ FillRect(40,40);
	//~ MoveTo(80,40);
	//~ fore = BLUE;
	//~ FillRect(40,40);
	
	//~ fore=WHITE;
	//~ MoveTo(10,10);
	//~ PlotText(PSTR("Text"));

	  
	  for (;;)
	  {

		
	  }//end of for()
}//end of main

ISR (TIMER1_COMPA_vect)
{
	
}
