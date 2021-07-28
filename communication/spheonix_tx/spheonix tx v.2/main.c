/*
 * spheonix tx v.2.c
 *
 * Created: 2019-04-27
 * Author : Jiho
 *               Atmega8 pin map
 *
 *			RESET   = PC6   PC5 =	SCL
 *			 RX		= PD0   PC4 =	SDA
 *           TX		= PD1   PC3 =   Input Mode SW
 *		   PStick-	= PD2   PC2 =	KeyOut3
 *		   PStick+	= PD3   PC1 =	KeyOut2
 *       Safety SW	= PD4   PC0 =	KeyOut1
 *           VCC	= VCC   GND =	GND
 *           GND	= GND   AREF=   
 *         KeyIn3	= PB6   AVCC=   
 *         KeyIn4	= PB7   PB5 =	KeyIn2
 *         LED_G	= PD5   PB4 =	KeyIn1
 *         LED_R	= PD6   PB3 =	YStick-
 *         RS485 IO = PD7   PB2 =   YStick+
 *		   Fire SW	= PB0   PB1 =	Buzzer
 *
 *
 */

#define F_CPU 8000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "pata_UART.h"
#include "pata_keypad.h"

// Pin number
#define RS485IO			0x80
#define FIRE_SW			0x01
#define SAFE_SW			0x10
#define LED_G			0x20
#define LED_R			0x40
#define YStickP			0x04
#define YStickM			0x08
#define PStickP			0x04
#define PStickM			0x08
#define ModeSW			0x08


// Constant
#define LENGTH			7
#define STARTBIT		0
#define YAWH			1
#define YAWL			2
#define PIT				3
#define FIRE			4
#define MODE			5
#define CHKSUM			6
#define ON				1
#define OFF				0

// Password
#define PASSWORD1		1
#define PASSWORD2		4
#define PASSWORD3		5
#define PASSWORD4		0
#define PASSWORD5		4
#define PASSWORD6		1

// Communication
#define CONNECTED		0xAA


// Macro
#define BUZZER1		{buzzer(100);_delay_ms(100);buzzer(0);_delay_ms(100);buzzer(100);_delay_ms(100);buzzer(0);_delay_ms(100);}	// Short, 2 time
#define BUZZER2		{buzzer(100);_delay_ms(500);buzzer(0);}																		// Long,  1 time
#define GLED_ON		{PORTD |= LED_G;}
#define RLED_ON		{PORTD |= LED_R;}
#define GLED_OFF	{PORTD &= ~LED_G;}
#define RLED_OFF	{PORTD &= ~LED_R;}


// Function
void init();
uint16_t keypad_read_m();
uint8_t keypad_read_d();
void check_password();
void buzzer(uint8_t freq);
void RS485_tx();
void RS485_rx(uint8_t *addr, uint8_t length);
void gen_checksum(uint8_t *adress);
void UART_rx_m_s(uint8_t *data, uint8_t length);



// Variable
// Timer
volatile uint8_t timer_temp_a = 0;
volatile uint8_t timer_temp_b = 0;
volatile uint8_t timer_100ms  = 0;
volatile uint8_t timer_40ms   = 0;

// Keypad read
uint8_t keypad_d8 = 0xFF;
uint16_t keypad_d16;
uint8_t yaw_h, yaw_l;

// Password
uint8_t password = 0;

// Communication
uint8_t cmd[LENGTH];
uint8_t launcher_angle[2];

// etc
uint8_t fire_temp = 0;
uint8_t fire_toggle = OFF;
uint8_t buzzer_toggle = OFF;
uint8_t timer_toggle = ON;
uint8_t fire_send = OFF;






int main(void)
{
	// Initial setting
	init();
	UART_init(BAUD_14k, TXRX);
	sei();
   
	// Power on buzzer
	BUZZER1;
   
	// Entering password
	check_password();
   
	// Enabled buzzer
	BUZZER1;
	//RLED_ON;
   
   
	// Main routine
    while (1)
    {
		// Turret Move Mode
		if (PIND & SAFE_SW)
		{
			if (buzzer_toggle == ON)
			{
				BUZZER1;
				buzzer_toggle = OFF;
			}
			RLED_ON;
			GLED_OFF;
			
			// Selecting keypad and joystick
			// Keypad
			if (PINC & ModeSW)
			{
				// Set mode
				cmd[MODE] = 0xBB;
			
				// Yaw & Pitch angle input
				keypad_d16 = keypad_read_m();
				if (keypad_d16 != 0xFFFF)	// if there is input
				{
					if (YPtoggle == YAW)
					{
						cmd[YAWH] = (keypad_d16>>8);
						cmd[YAWL] = keypad_d16;
					}
					else
					{
						cmd[PIT] = keypad_d16;
					}
				}
			
			}
			// Joystick
			else
			{			
				// Set mode
				cmd[MODE] = 0xAA;
			
				// Joystick input
				// Yaw
				if (!(PINB & YStickP))
				{
					cmd[YAWL] = 0xF0;
				}
				else if (!(PINB & YStickM))
				{
					cmd[YAWL] = 0x0F;
				}
				else
				{
					cmd[YAWL] = 0;
				}
				// Pitch
				if (!(PIND & PStickP))
				{
					cmd[PIT] = 0xF0;
				}
				else if (!(PIND & PStickM))
				{
					cmd[PIT] = 0x0F;
				}
				else
				{
					cmd[PIT] = 0;
				}
			
			}
		}
		// Fire Mode
		else
		{
			if (buzzer_toggle == OFF)
			{
				BUZZER2;
				buzzer_toggle = ON;
				cmd[MODE] = 0xCC;
				RLED_OFF;
				GLED_ON;
			}
			
			
			// Read rocket number
			keypad_d8 = keypad_read_d();
			if (keypad_d8 != 0xFF)
			{
				fire_temp = keypad_d8;
			}
			
			if (!(PINB & FIRE_SW))
			{
				fire_toggle = ON;
			}
			else if ((PINB & FIRE_SW) && fire_toggle)
			{
				fire_toggle = OFF;
				fire_send = ON;
				timer_100ms = 0;
				timer_temp_a = 0;
				
			}
			else if (fire_send)
			{
				if (timer_100ms < 5)
				{
					timer_toggle = OFF;
					cmd[FIRE] = fire_temp;
				}
				else
				{
					fire_send = OFF;
					cmd[FIRE] = 0;
					fire_temp = 0;
				}
				
			}
			
		}
		
		
		// Display
		
		
		
		
		gen_checksum(cmd);
		// Transmit every 40ms
		if (timer_40ms != 0)
		{
			// TX
			RS485_tx();
			
			// RX
			//RS485_rx(launcher_angle, 2);
			
			timer_40ms = 0;
			
		}
      
	}
   
}

/****************************************************************/
/*							Timer								*/
/*																*/
/****************************************************************/
ISR (TIMER0_OVF_vect)
{
	TCNT0 = 5;
	timer_temp_a++;
	timer_temp_b++;
	
	if (timer_temp_a >= 12)
	{
		timer_100ms++;
		timer_temp_a = 0;
	}
	if (timer_temp_b >= 5)
	{
		timer_40ms++;
		timer_temp_b = 0;
	}
	
}


/****************************************************************/
/*						Initialize								*/
/*																*/
/****************************************************************/
void init()
{
	DDRB = 0x02;
	DDRD = 0xE2;
	DDRC = 0x07;
	
	// Timer0 interrupt setting
	TCCR0 = (1 << CS02);   // prescalar: 256
	TIMSK = (1 << TOIE0);   // start interrupt
	
	// PWM setting (fast PWM mode)
	TCCR1A = (1 << WGM11) |
			 (1 << COM1A1)| (1<<COM1B1);   // OC1A non-inverting mode
	TCCR1B = (3 << WGM12) | (1 << CS11);   // prescalar: 8
	ICR1 = 9999;
	
	cmd[STARTBIT] = 0xAA;
	
}


/****************************************************************/
/*						Check password							*/
/*																*/
/****************************************************************/
void check_password()
{
	while (1)
	{
		uint8_t password_temp;
		password_temp = keypad_read_d();
		
		if ((PASSWORD1 == password_temp) && (digit_count == 1))
			password = 1;
		if ((PASSWORD2 == password_temp) && (digit_count == 2))
			password |= 0x02;
		if ((PASSWORD3 == password_temp) && (digit_count == 3))
			password |= 0x04;
		
		// If key is pressed 3 times
		if (digit_count == 3)
		{
			if (password == 0x07)	// If password is correct, escape from while(1)
				break;
			else					// If password is wrong, set buzzer and retry
			{
				password = 0;
				digit_count = 0;
				BUZZER2;
			}
		}
	}
}

/****************************************************************/
/*						Ring buzzer								*/
/*																*/
/****************************************************************/
void buzzer(uint8_t freq)
{
	OCR1A = (ICR1 / 127 * freq);
}

/****************************************************************/
/*						RS485 Communication						*/
/*																*/
/*[Start][ Yaw_h ][ Yaw_l ][ Pitch ][ Fire ][ Mode ][ Checksum ]*/
/*																*/
/*	- Checksum = Mod of sum of each bit							*/
/*																*/
/****************************************************************/
void RS485_tx()
{
	PORTD |= RS485IO;
	UART_tx_m(cmd, LENGTH);
   
}
void RS485_rx(uint8_t *addr, uint8_t length)
{
	PORTD &= ~RS485IO;
	UART_rx_m(addr, length);
	
}
void gen_checksum(uint8_t *cmd)
{
	cmd[CHKSUM] = (uint8_t)((cmd[YAWH] + cmd[YAWL] + cmd[PIT] + cmd[FIRE] + cmd[MODE]) % 0xFF);
	
}
void UART_rx_m_s(uint8_t *data, uint8_t length)
{
	for (uint8_t i = 0; i < length; i++)
	{
		while(!(UCSRA & (1<<RXC)))
		{
			
		}
		UDR = data[i];
	}
}