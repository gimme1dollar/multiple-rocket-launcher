/*
 * pata_keypad.h
 *
 * Created: 2019-04-18 오후 8:28:47
 *  Author: RIGHT
 
 			    Telemetry	TX		RX	   Reset	SCL		SDA	   ADC4	   ADC3
 			    
 			    PD2=====PD1=====PD0=====PC6=====PC5=====PC4=====PC3=====PC2
 			    =															   =
 			    Module	PD3																	 PC1	ADC2
 			    =																	  =
 			    Menu	PD4																	 PC0	ADC1
 			    =																	  =
 			    GND	GND																	 ADC7
 			    =																	  =
 			    VCC	VCC																 	 GND	GND
 			    =																	  =
 			    GND	GND								 ATMEGA8							 AREF	AREF
 			    =																	  =
 			    VCC	VCC																	 ADC6	Battery ADC
 			    =																	  =
 			    PO3	PB6																	 AVCC	AVCC
 			    =																	  =
 			    PO4	PB7																	 PB5	PO2		(SCK)
 			    =
 			    PD5		PD6		PD7		PB0		PB1		PB2		PB3		PB4
 			    
 			    R LED   G LED	PI1	   Buzzer	PI2		PI3		PO1
 			    (SS)	  (MOSI)  (MISO)
 */ 


#ifndef PATA_KEYPAD_H_
#define PATA_KEYPAD_H_

#define KeyOut1         0x01
#define KeyOut2         0x02
#define KeyOut3         0x04
#define KeyIn1			0x10
#define KeyIn2			0x20
#define KeyIn3			0x40
#define KeyIn4			0x80

#define KEY0         0x02
#define KEY1         0x01
#define KEY2         0x02
#define KEY3         0x04
#define KEY4         0x08
#define KEY5         0x10
#define KEY6         0x20
#define KEY7         0x40
#define KEY8         0x80
#define KEY9         0x01
#define KEYSTR         0x04
#define KEYSHP         0x08

#define MASK1         0x10
#define MASK2         0x20
#define MASK3         0x40
#define MASK4         0x80

#ifndef YAW
#define YAW			2
#endif
#ifndef PIT
#define PIT			3
#endif

#ifndef ON
#define ON            1
#endif
#ifndef OFF
#define OFF            0
#endif

// Macro
#define DigitAdd	{if (digit_count < 3) digit_count++; else digit_count = 1; digit_toggle = 1;}
#define DigitAdd4	{if (digit_count < 4) digit_count++; else digit_count = 1; digit_toggle = 1;}
#define YPToggle	{if (YPtoggle == PIT) {YPtoggle = YAW; digit_count = 0; digit_toggle = 1;}	 else {YPtoggle = PIT; digit_count = 0; digit_toggle = 1;}}

#include <stdint.h>
#include <util/delay.h>



uint8_t keypad_in[3];
uint8_t key_h, key_l;		// High, low bit temp memory
uint16_t keypad_raw;
uint8_t key_d8 = 0xFF;
uint16_t key_d16;
uint16_t key_m16;
uint8_t key_toggle[12] = {OFF};
uint8_t digit_count = 0;
uint8_t digit_toggle = 0;	// Become 1 when keypad is pressed (Flag rised when update)

uint8_t YPtoggle = OFF;


// Keypad IO initialize
void keypad_init();

// Key in
uint16_t keypad_read();
uint16_t keypad_read_m();



void keypad_init()
{
   DDRB = 0x0F;
   //PORTB = KeyIn1|KeyIn2|KeyIn3|KeyIn4;   // Key input pin pull-up
   
}

uint16_t keypad_read()
{
	//PORTC = 0x05;
	PORTC = 0x06;
	keypad_in[0] = ~PINB;
	_delay_us(100);
   
	//PORTC = 0x06;
	PORTC = 0x05;
	keypad_in[1] = ~PINB;
	_delay_us(100);
   
	PORTC =	0x03;
	keypad_in[2] = ~PINB;
	_delay_us(100);
   
	key_l = 0;
	key_h = 0;
   
	if (keypad_in[1] & MASK1)   // 1
		key_l |= 0x01;
	if (keypad_in[1] & MASK2)   // 4
		key_l |= 0x08;
	if (keypad_in[1] & MASK3)   // 7
		key_l |= 0x40;
	if (keypad_in[1] & MASK4)   // *
		key_h |= 0x04;
   
	if (keypad_in[2] & MASK1)   // 2
		key_l |= 0x02;
	if (keypad_in[2] & MASK2)   // 5
		key_l |= 0x10;
	if (keypad_in[2] & MASK3)   // 8
		key_l |= 0x80;
	if (keypad_in[2] & MASK4)   // 0
		key_h |= 0x02;
   
	if (keypad_in[0] & MASK1)   // 3
		key_l |= 0x04;
	if (keypad_in[0] & MASK2)   // 6
		key_l |= 0x20;
	if (keypad_in[0] & MASK3)   // 9
		key_h |= 0x01;
	if (keypad_in[0] & MASK4)   // #
		key_h |= 0x08;
   
	key_d16 = (key_h<<8)|key_l;
   
	return key_d16;
	
}

/****************************************************************/
/*					Keypad read	(Single digit)					*/
/*																*/
/*	- Read single key press and convert it into decimal.		*/
/*																*/
/*	- Return : key_d8, 8 bit									*/
/*																*/
/****************************************************************/
uint8_t keypad_read_d()
{
	key_d8 = 0xFF;
	keypad_raw = keypad_read();
	key_h = (keypad_raw>>8);
	key_l = (uint8_t)keypad_raw;
	
	// 0
	if (key_h & KEY0)
		key_toggle[0] = ON;
	else if ((!(key_h & KEY0)) && (key_toggle[0] == ON))
	{
		DigitAdd;
		key_toggle[0] = OFF;
		key_d8 = 0;
	}
	// 1
	if (key_l & KEY1)
		key_toggle[1] = ON;
	else if ((!(key_l & KEY1)) && (key_toggle[1] == ON))
	{
		DigitAdd;
		key_toggle[1] = OFF;
		key_d8 = 1;
	}
	// 2
	if (key_l & KEY2)
		key_toggle[2] = ON;
	else if ((!(key_l & KEY2)) && (key_toggle[2] == ON))
	{
		DigitAdd;
		key_toggle[2] = OFF;
		key_d8 = 2;
	}
	// 3
	if (key_l & KEY3)
		key_toggle[3] = ON;
	else if ((!(key_l & KEY3)) && (key_toggle[3] == ON))
	{
		DigitAdd;
		key_toggle[3] = OFF;
		key_d8 = 3;
	}
	// 4
	if (key_l & KEY4)
		key_toggle[4] = ON;
	else if ((!(key_l & KEY4)) && (key_toggle[4] == ON))
	{
		DigitAdd;
		key_toggle[4] = OFF;
		key_d8 = 4;
	}
	// 5
	if (key_l & KEY5)
		key_toggle[5] = ON;
	else if ((!(key_l & KEY5)) && (key_toggle[5] == ON))
	{
		DigitAdd;
		key_toggle[5] = OFF;
		key_d8 = 5;
	}
	// 6
	if (key_l & KEY6)
		key_toggle[6] = ON;
	else if ((!(key_l & KEY6)) && (key_toggle[6] == ON))
	{
		DigitAdd;
		key_toggle[6] = OFF;
		key_d8 = 6;
	}
	// 7
	if (key_l & KEY7)
		key_toggle[7] = ON;
	else if ((!(key_l & KEY7)) && (key_toggle[7] == ON))
	{
		DigitAdd;
		key_toggle[7] = OFF;
		key_d8 = 7;
	}
	// 8
	if (key_l & KEY8)
		key_toggle[8] = ON;
	else if ((!(key_l & KEY8)) && (key_toggle[8] == ON))
	{
		DigitAdd;
		key_toggle[8] = OFF;
		key_d8 = 8;
	}
	// 9
	if (key_h & KEY9)
		key_toggle[9] = ON;
	else if ((!(key_h & KEY9)) && (key_toggle[9] == ON))
	{
		DigitAdd;
		key_toggle[9] = OFF;
		key_d8 = 9;
	}
	// *
	if (key_h & KEYSTR)
		key_toggle[10] = ON;
	else if ((!(key_h & KEYSTR)) && (key_toggle[10] == ON))
	{
		DigitAdd;
		YPToggle;
		key_toggle[10] = OFF;
		key_d8 = 0x0B;
	}
	// #
	if (key_h & KEYSHP)
		key_toggle[11] = ON;
	else if ((!(key_h & KEYSHP)) && (key_toggle[11] == ON))
	{
		DigitAdd;
		key_toggle[11] = OFF;
		key_d8 = 0x0C;
	}
	
	
	// Return digit as decimal
	return key_d8;
	
}

/****************************************************************/
/*				Keypad read	(Multiple digit)					*/
/*																*/
/*	- Read multiple key presses and convert it into decimal.	*/
/*	- Return 0xFFFF as default.									*/
/*	- If 3 press is made, 3 digit 16 bit value is returned		*/
/*																*/
/*																*/
/*	- Return  : key_m16, 16 bit								*/
/*	  default : 0xFFFF											*/
/*																*/
/****************************************************************/
uint16_t keypad_read_m()
{
	uint8_t key_d8_temp;
	
	key_d8_temp = keypad_read_d();
	
	if ((digit_count == 1) && (digit_toggle))
	{
		key_m16 = 0;
		key_m16 = key_d8_temp*100;
		digit_toggle = 0;
	}
	else if ((digit_count == 2) && (digit_toggle))
	{
		key_m16 += key_d8_temp*10;
		digit_toggle = 0;
	}
	else if ((digit_count == 3) && (digit_toggle))
	{
		key_m16 += key_d8_temp;
		digit_toggle = 0;
		
		// return 3-digit decimal number
		return key_m16;
	}
	
	
	// If keypad is not pressed, return 0xFFFF as default
	return 0xFFFF;
}

#endif /* PATA_KEYPAD_H_ */