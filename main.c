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

void ScoreBoard(uint32_t Score, uint16_t PlayMin, uint8_t PlaySec);

const uint8_t size = 128;

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
	
	const uint8_t rectWidth = 20;
	const uint8_t rectHeight = 4;
	const uint8_t rectSpeed = 2;
	
	const uint8_t BlockDist = 1;
	
	const uint8_t StartLives = 3;
	uint8_t lives = StartLives;
	
	char buffer[20];
	
	uint8_t i = 0;	//throwaway variable for for loops
	
	float tmp;	//Variable for storage of temporary data
	
	uint8_t isActive = 1; //Used to check if All Blocks have been destroyed
	
	uint8_t collided = 0;	//Variable to check if Ball has already collided with a Block
	uint8_t collided_Platform = 0;	//Variable to check if the last collision with of the ball was with Rectangle
	
	uint8_t Frame = 0;	//Current Frame
	uint8_t UpdateFrequency = 1; //The amount of Frames between DisplayUpdates
	
	uint8_t BlockGridSize[2];
	BlockGridSize[0] =	10;	//Horizontal Size
	BlockGridSize[1] =	5;	//Vertical Size
	
	uint8_t BlockSize[2];
	BlockSize[0] = (size / BlockGridSize[0]);
	BlockSize[1] = ((size / 2) / BlockGridSize[1]);
	
	uint8_t BlocksStatus[BlockGridSize[0]][BlockGridSize[1]];
	
	uint64_t Score = 0;	//Variable to store PlayerScore;
	int x;
	int y;
	void BuildGrid(void)
	{
		for (x = 0; x < BlockGridSize[0]; x++)
		{
			for (y = 0; y < BlockGridSize[1]; y++)
			{
				fore = rand();
				BlocksStatus[x][y] = 1;
				MoveTo(x * (BlockSize[0]) + BlockDist, (size / 2) + (y * BlockSize[1]) + BlockDist);
				FillRect(BlockSize[0] - BlockDist, BlockSize[1] - BlockDist);
			}
		}
	}
	BuildGrid();
	int8_t RectX;
	int8_t RectY;
	
	float BallMove[] = {0, -1};
	uint16_t BallData[] = {size / 2, size / 2, 2, 2};	//Data for Ball {x Coordinate, y Coordinate, radius, thickness of outside line}
	long double BallCoord[] = {size / 2, size / 2};
	uint16_t BallLastRenderedPos[] = {size / 2, size / 2};	//The last rendered Position of the Ball 
	
	RectX = 0;
	RectY = 10;
	
	uint8_t comboMult = 0; //The comboMutliplier
	while (1)
	{
		Frame++;
		//Input and RectangleClear
		//==============================================================
		fore = BLACK;
		if (T3)
		{
			MoveTo(RectX - 1, RectY);
			FillRect(2, rectHeight);
			RectX += rectSpeed;
		}
		else if (T1)
		{
			MoveTo(RectX + rectWidth, RectY);
			FillRect(2, rectHeight);
			RectX -= rectSpeed;
		}
		
		//==============================================================
		
		//Autoplay
		//==============================================================
		if (0)
		{
			if(BallData[0] > (RectX + (rectWidth / 2)))
			{
				MoveTo(RectX - 1, RectY);
				FillRect(2, rectHeight);
				RectX += rectSpeed;
			}
			else
			{
				MoveTo(RectX + rectWidth, RectY);
				FillRect(2, rectHeight);
				RectX -= rectSpeed;
			}
		}
		
		//==============================================================
		
		//Ball movement
		//==============================================================
		BallCoord[0] += BallMove[0];
		BallCoord[1] += BallMove[1];
		BallData[0] = BallCoord[0];
		BallData[1] = BallCoord[1];
		//==============================================================
		
		//Check if Rectangle is Coliding with Wall
		//==============================================================
		if (RectX > size - rectWidth)
		{
			RectX =	size - rectWidth;
		}
		
		else if (RectX <= 0)
		{
			RectX = 0;
		}
		//==============================================================
		
		//Check if Ball is Coliding with Wall
		//==============================================================
		if (BallData[0] > size - BallData[2])
		{
			BallData[0] = size - BallData[2] - 3;
			BallCoord[0] = size - BallData[2] - 3;
			BallMove[0] = -BallMove[0];
			collided_Platform = 0;
		}
		else if(BallData[0] < BallData[2])
		{
			BallData[0] = BallData[2];
			BallCoord[0] = BallData[2];
			BallMove[0] = -BallMove[0];
			collided_Platform = 0;
		}
		//==============================================================
		
		//Check if Ball is Coliding with Roof
		//==============================================================
		if (BallData[1] > size - BallData[2])
		{
			BallMove[1] = -BallMove[1];
			collided_Platform = 0;
		}
		//==============================================================
		
		//Check if Ball is Coliding with Rectangle
		//==============================================================
		if ((BallData[1] < (RectY + rectHeight + BallData[2]) && (BallData[0] < (rectWidth + RectX)) && (BallData[0] > RectX)) && collided_Platform != 1)
		{
			tmp = ((BallCoord[0] - RectX) / rectWidth);
			if (BallMove[0] < 0)
			{
				BallMove[0] = tmp * 2 * -1; 
			}
			
			else if (BallMove[0] > 0)
			{
				BallMove[0] = tmp * 2; 
			}
			else
			{
				BallMove[0] = 0.1;
			}
			BallMove[1] = -BallMove[1];
			
			if(BallMove[0] > 1)
			{
				BallMove[0] = 1;
			}
			
			else if(BallMove[0] < -1)
			{
				BallMove[0] = -1;
			}
			collided_Platform = 1;
			if (isActive == 0)
			{
				BuildGrid();
			}
			comboMult = 0;
		}
		//==============================================================
		
		//Check if Ball is Coliding Floor and if true reduce Lives if lives 0 then reset game
		//==============================================================
		if (BallData[1] < 3)
		{
			lives--;
			BallData[0] = size / 2;
			BallData[1] = (size / 2) - 10;
			BallCoord[0] = size / 2;
			BallCoord[1] = (size / 2) - 10;
			BallMove[0] = 0;
			BallMove[1] = -1;
			if(lives == 0)
			{
				ScoreBoard(Score, minute, second);
				lives = StartLives;
				Score = 0;
				ClearDisplay();
				BuildGrid();
				collided_Platform = 0;
				minute = 0;
				second = 0;
			}
		}
		//==============================================================
		
		//Check if Ball is Coliding with Block
		//==============================================================
		collided = 0;
		isActive = 0;
		for (y = 0; y < BlockGridSize[1]; y++)
		{
			for (x = 0; x < BlockGridSize[0]; x++)
			{
				MoveTo(x * (BlockSize[0]) + BlockDist, (size / 2) + (y * BlockSize[1]) + BlockDist);
				if (BlocksStatus[x][y] == 1)
				{
					if (collided == 0)
					{
						fore = BLACK;
						//Check if Ball is Coliding with Block horizontal side
						//==============================================================
						if ((BallData[0] + BallData[2]) > (x * BlockSize[0]) && (BallData[0] - BallData[2]) < ((x * BlockSize[0]) + BlockSize[0]) && (((BallData[1] - (size / 2)) == (y * BlockSize[1])) || ((BallData[1] - (size / 2)) == ((y * BlockSize[1]) + BlockSize[1]))))
						{
							comboMult++;
							BlocksStatus[x][y] = 0;
							BallMove[1] = -BallMove[1];
							FillRect(BlockSize[0] - BlockDist, BlockSize[1] - BlockDist);
							collided = 1;
							collided_Platform = 0;
							Score += 1 * comboMult;
						}
						//==============================================================
		
						//Check if Ball is Coliding with Block vertical side
						//==============================================================
						else if ((BallData[0] + BallData[2] == (x * BlockSize[0]) || (BallData[0] - BallData[2] == ((x * BlockSize[0]) + BlockSize[0]))) && (((BallData[1] - (size / 2)) > (y * BlockSize[1])) && ((BallData[1] - (size / 2)) < ((y * BlockSize[1]) + BlockSize[1]))))
						{
							comboMult++;
							BlocksStatus[x][y] = 0;
							BallMove[0] = -BallMove[0];
							FillRect(BlockSize[0] - BlockDist, BlockSize[1] - BlockDist);
							collided = 1;
							collided_Platform = 0;
							Score += 1 * comboMult;
						}
						//==============================================================
						
						//Check if Ball is inside Block
						//==============================================================
						else if ((BallData[0]) > (x * BlockSize[0]) && (BallData[0]) < ((x * BlockSize[0]) + BlockSize[0]) && (((BallData[1] - (size / 2)) > (y * BlockSize[1])) && ((BallData[1] - (size / 2)) < ((y * BlockSize[1]) + BlockSize[1]))))
						{
							comboMult++;
							BlocksStatus[x][y] = 0;
							BallMove[0] = -BallMove[0];
							FillRect(BlockSize[0] - BlockDist, BlockSize[1] - BlockDist);
							collided = 1;
							collided_Platform = 0;
							Score += 1 * comboMult;
						}
						//==============================================================
					}
					isActive = 1;
				}
			}
		}
		//==============================================================

		
		//DisplayUpdate
		//==============================================================
		
		if (Frame % UpdateFrequency == 0)
		{
			Frame = 0;
			for (i = 0; i < BallData[3]; i++)
			{
				fore = BLACK; // Black
				glcd_draw_circle(BallLastRenderedPos[0], BallLastRenderedPos[1], BallData[2] - i);
				fore = WHITE; // White
				glcd_draw_circle(BallData[0], BallData[1], BallData[2] - i);
			}
			BallLastRenderedPos[0] = BallData[0];
			BallLastRenderedPos[1] = BallData[1];
			
			fore = WHITE; // White
			
			MoveTo(RectX,RectY);
			FillRect(rectWidth, rectHeight);
			
			MoveTo(0, 0);
			fore = RED;
			sprintf(buffer, "Score: %d", Score);
			PlotString(buffer);
			
			MoveTo(0, size - 10);
			if (minute < 10)
			{
				sprintf(buffer, "Time: 0%d:", minute);
				PlotString(buffer);
			}
			else
			{
				sprintf(buffer, "Time: %d:", minute);
				PlotString(buffer);
			}
			
			if (second < 10)
			{
				sprintf(buffer, "0%d:", second);
				PlotString(buffer);
			}
			else
			{
				sprintf(buffer, "%d:", second);
				PlotString(buffer);
			}
			
			for (i = 0; i < StartLives; i++)
			{
				if (i < lives)
				{
					fore = RED;
				}
				else
				{
					fore = BLACK;
				}
				MoveTo(size - 8, ((i * 8) + 8));
				FillRect(7,7);
			}
		}
		//==============================================================
	}	
	
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

void ScoreBoard(uint32_t Score, uint16_t PlayMin, uint8_t PlaySec)
	{
		//Define Variables
		//==============================================================
		const uint16_t startAdress = 0;	//First eeprom adress to write to/read from
		const uint8_t BoardSize = 6;	//amount of displayed Scores
		uint8_t sorted = 0;	//used to chek if Scores are Sorted
		uint64_t Counter;	//Counting Variable
		uint64_t Counter2;	//Counting Variable
		char buffer[20];	//Buffer for writing Numbers
		struct BoardItems	//Used for storing the Scores
		{
			uint32_t Score;
			uint16_t ScoreMin;
			uint8_t ScoreSec;
			char PlayerName[3];
		};
		
		struct BoardItems Board[BoardSize];	//Score Storage for Unsorted and sorted Scores
		struct BoardItems SortBuffer[BoardSize];	//Buffer for sorting
		//==============================================================
		
		//Change -1 values to zero to keep the sorting system working
		//==============================================================
		for (Counter = startAdress; Counter < (startAdress + (68 * BoardSize)); Counter++)
		{
			fore = WHITE;
			sprintf(buffer, "%d", eeprom_read_byte((uint8_t*)Counter));
			PlotString(buffer);
			if(eeprom_read_byte((uint8_t*)Counter) == 255)
			{
				eeprom_write_byte((uint8_t*)startAdress + Counter, 0);	//Read eeprom
			}
		}
		//==============================================================
		
		for (Counter = 0; Counter < BoardSize; Counter++)
		{
			Board[Counter].Score = eeprom_read_dword((uint32_t*)startAdress + (32 * Counter));	//Read Score from eeprom
			Board[Counter].ScoreMin = eeprom_read_word((uint16_t*)startAdress + (16 * Counter) + (32 * BoardSize));	//Read Minutes from eeprom
			Board[Counter].ScoreSec = eeprom_read_byte((uint8_t*)startAdress + (8 * Counter) + (48 * BoardSize));	//Read Seconds from eeprom
			eeprom_read_block(Board[Counter].PlayerName, (void*)startAdress + (12 * Counter) + (56 * BoardSize), 3);	//Read Player Name from eeprom
		}
		
		Board[BoardSize - 1].Score = Score;			//Input new Score
		Board[BoardSize - 1].ScoreMin = PlayMin;	//Input new Minute Time
		Board[BoardSize - 1].ScoreSec = PlaySec;	//Input new Seconds Time
		
		Board[BoardSize - 1].PlayerName[0] = 'A';	//Set Player Name to AAA
		Board[BoardSize - 1].PlayerName[1] = 'A';	//|
		Board[BoardSize - 1].PlayerName[2] = 'A';	//|
		
		ClearDisplay();
		//Name Input
		//==============================================================
		for (Counter = 0; Counter < 3; Counter++)
		{
			scale = 3;
			while(!T2)
			{
				if(T1)
				{
					Board[BoardSize - 1].PlayerName[Counter]--;	//Last Letter
				}
				
				if(T3)
				{
					Board[BoardSize - 1].PlayerName[Counter]++;	//Next Letter
				}
				while(T1 || T3);
				MoveTo(10, size / 2);
				for (Counter2 = 0; Counter2 < 3; Counter2++)
				{
					if (Counter == Counter2)
					{
						fore = BLACK;
						back = WHITE;
					}
					
					else
					{
						fore = WHITE;
						back = BLACK;
					}
					PlotChar(Board[BoardSize - 1].PlayerName[Counter2]);
				}
				fore = WHITE;
				back = BLACK;
			}
			while(T2);
		}
		scale = 1;
		
		fore = WHITE;
		
		//BoardSorting
		//==============================================================
		while(sorted == 0)
		{
			sorted = 1;
			for (Counter = 1; Counter < BoardSize; Counter++)
			{
				if (Board[Counter - 1].Score < Board[Counter].Score)	//If last Score is smaller than current one then set them switched inside of Buffer
				{
					sorted = 0;
					SortBuffer[Counter] = Board[Counter - 1];

					SortBuffer[Counter - 1] = Board[Counter];

				}
				else 	//If not set current one in same position in Buffer
				{
					SortBuffer[Counter] = Board[Counter];

					if (Counter == 1)	//if current Score is 1 then set score 0 to position 0 buffer
					{
						SortBuffer[Counter - 1] = Board[Counter - 1];
					}
				}
			}
			for (Counter = 0; Counter < BoardSize; Counter++)
			{
				Board[Counter] = SortBuffer[Counter];	//Set Board equall to buffer
			}
		}
		//==============================================================
		
		for (Counter = 0; Counter < BoardSize; Counter++)
		{
			//Store Data
			//==========================================================
			eeprom_write_dword((uint32_t*)startAdress + (32 * Counter), Board[Counter].Score);
			eeprom_write_word((uint16_t*)startAdress + (16 * Counter) + (32 * BoardSize), Board[Counter].ScoreMin);	
			eeprom_write_byte((uint8_t*)startAdress + (8 * Counter) + (48 * BoardSize), Board[Counter].ScoreSec);
			eeprom_write_block(Board[Counter].PlayerName, (void*)startAdress + (12 * Counter) + (56 * BoardSize), 3);
			//==========================================================
		}
		//==============================================================
		ClearDisplay();
		fore = WHITE;
		//==============================================================
		
		//Print Score
		//==============================================================
		for (Counter = 0; Counter < BoardSize; Counter++)
		{
			MoveTo(0, ((size - 10) - (10 * Counter)));
			for (Counter2 = 0; Counter2 < 3; Counter2++)
			{
				PlotChar(Board[Counter].PlayerName[Counter2]);
			}
			sprintf(buffer, "  %d:",Board[Counter].ScoreMin);
			PlotString(buffer);
			sprintf(buffer, "%d", Board[Counter].ScoreSec);
			PlotString(buffer);
			sprintf(buffer, "  %d", Board[Counter].Score);
			PlotString(buffer);
		}
		while(!T2)
		{
			//~ if (T1)
			//~ {
				//~ for (Counter = 0; Counter < E2END; Counter++)
				//~ {
					//~ eeprom_update_byte((uint8_t*)Counter, 0);	//Read eeprom
				//~ }
			//~ }
		}
	}

ISR (TIMER1_COMPA_vect)
{
	
}
