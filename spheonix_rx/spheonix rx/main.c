/*
 * motor driver test (spheonix).c
 *
 * Created: 2019-01-16 오후 9:32:00
 * Author : pata
 *
 *
 *               Atmega8 pin map
 *
 *         RESET   = PC6   PC5 =
 *           RX   = PD0   PC4 =
 *           TX   = PD1   PC3 =   G_LED
 *           IN1   = PD2   PC2 =   R_LED
 *           IN2   = PD3   PC1 =   ENC_PIT
 *           IN3   = PD4   PC0 =   ENC_YAW
 *           VCC   = VCC   GND =
 *           GND   = GND   AREF=
 *         ROCKET5   = PB6   AVCC=
 *         ROCKET6   = PB7   PB5 =   ROCKET4
 *           IN4   = PD5   PB4 =   ROCKET3
 *         BUZZER   = PD6   PB3 =   ROCKET2
 *         RS485 IO = PD7   PB2 =   PWM_P
 *         ROCKET1   = PB0   PB1 =   PWM_Y
 *
 */ 

#define F_CPU 8000000UL
#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include "pata_UART.h"

#define FRONT      0x02
#define BACK      0x01
#define IN1         0x10
#define IN2         0x20
#define IN3         0x40
#define IN4         0x80

#define CW         0
#define CCW         1
#define FULL      127
#define HALF      100
#define QUAT      60
#define STOP      0

#define FIRE      3

#define ROCKET1      0x01
#define ROCKET2      0x08
#define ROCKET3      0x10
#define ROCKET4      0x20
#define ROCKET5      0x40
#define ROCKET6      0x80
#define BUZZER      0x40
#define ENC_YAW      0x01
#define ENC_PIT      0x02


#define BREAK_Y      {PORTD &= ~(1<<IN1)|(1<<IN2);}
#define BREAK_P      {PORTD &= ~(1<<IN3)|(1<<IN4);}
   

// Function prototype
void init();
void motor_mov(uint8_t cmd_y, uint8_t cmd_p, uint8_t cur_y, uint8_t cur_p);
void fire_trig(uint8_t fire);
void encoder_read();
void calc_motspd();

// Variable
// Timer
volatile uint16_t timer_100ms_f = 0;
volatile uint16_t timer_temp_f = 0;

// Command
uint8_t cmd[4] = {0,0,0,0};      // yaw_h, yaw_l, pitch, fire
uint16_t cmd_y, cmd_p;
uint8_t  motspd_y, motspd_p;
uint8_t  motdir_y, motdir_p;

// Status
uint16_t cur_y, cur_p;
uint8_t fire_toggle = 0;




/********************************************************************/
/*                        Timer                        */
/*                                                   */
/*      - 8ms timer                                       */
/*                                                   */
/********************************************************************/
ISR (TIMER0_OVF_vect)
{
   TCNT0 = 5;
   timer_temp_f++;
   
   if (timer_temp_f >= 12)
   {
      timer_100ms_f++;
      timer_temp_f = 0;
   }
   
}


int main(void)
{
   init();
   UART_init(BAUD_14k, TXRX);
   sei();
   
   while (1)
   {
      // Receive command
      UART_rx_m(cmd, 4);
      cmd_y = cmd[1]|(cmd[0]<<8);
      cmd_p = (uint16_t)cmd[2];
      
      
      // Move turret
      motor_mov(cmd_y, cmd_p, cur_y, cur_p);
      
      // Fire
      fire_trig(cmd[FIRE]);
      
      
   }
}



/********************************************************************/
/*                     Initialize                        */
/*                                                   */
/*      - Motor is locked when rocket is firing                  */
/*      - More than two rockets cannot be shoot at the same time   */
/*                                                   */
/********************************************************************/
void init()
{
   DDRB = 0xFF;
   DDRD = 0xFE;
   DDRC = 0xFF;
   
   // Timer0 interrupt setting
   TCCR0 = (1 << CS02);   // prescalar: 256
   TIMSK = (1 << TOIE0);   // start interrupt
   
   // PWM setting (fast PWM mode)
   TCCR1A = (1 << WGM11) |
          (1 << COM1A1) | (1<<COM1B1);   // OC1A non-inverting mode
   TCCR1B = (3 << WGM12) | (1 << CS11);   // prescalar: 8
   ICR1 = 9999;

   
                                    
}


/********************************************************************/
/*                     Fire control                     */
/*                                                   */
/*      - Motor is locked when rocket is firing                  */
/*      - More than two rockets cannot be shoot at the same time   */
/*                                                   */
/********************************************************************/
void motor_mov(uint8_t Y_direc, uint8_t P_direc)
{
   // Direction setting
   // Yaw
   if (Y_direc == 0)
   {
      PORTD |= IN1;
      PORTD |= IN2;
   }
   else if ((Y_direc & 0xF0) == 0x10)
   {
      PORTD |= IN1;
      PORTD &= ~IN2;
   }
   else if ((Y_direc & 0xF0) == 0x20)
   {
      PORTD &= ~IN1;
      PORTD |= IN2;
   }
   
   if (P_direc == 0)
   {
      PORTD |= IN3;
      PORTD |= IN4;
   }
   else if ((P_direc & 0xF0) == 0x10)
   {
      PORTD |= IN3;
      PORTD &= ~IN4;
   }
   else if ((P_direc & 0xF0) == 0x20)
   {
      PORTD &= ~IN3;
      PORTD |= IN4;
   }
   
   
   // PWM
   OCR1A = (ICR1 / 127 * motspd_y);
   OCR1B = (ICR1 / 127 * motspd_p);
   
   
}

void calc_motspd()
{
   // Read angle
   encoder_read();
   
   
   // Yaw direction
   if (cmd_y > cur_y)
   {
      if (abs(cmd_y-cur_y) < 180)
         motdir_y = CW;
      else
         motdir_y = CCW;
   } 
   else
   {
      if (abs(cmd_y-cur_y) < 180)
         motdir_y = CCW;
      else
         motdir_y = CW;
   }
   // Pitch direction
   if (cmd_p > cur_p)
   {
      motdir_p = CW;
   } 
   else
   {
      motdir_p = CCW;
   }
   
   // Yaw speed
   if (abs(cmd_y-cur_y) > 30)
   {
      motspd_y = FULL;
   } 
   else if (abs(cmd_y-cur_y) > 18)
   {
      motspd_y = HALF;
   }
   else if (abs(cmd_y-cur_y) > 5)
   {
      motspd_y = QUAT;
   }
   else if (abs(cmd_y-cur_y) <= 5)
   {
      motspd_y = STOP;
      
   }
   
   // Pitch speed
   if (abs(cmd_p-cur_p) > 30)
   {
      motspd_p = FULL;
   }
   else if (abs(cmd_p-cur_p) > 18)
   {
      motspd_p = HALF;
   }
   else if (abs(cmd_p-cur_p) > 5)
   {
      motspd_p = QUAT;
   }
   else if (abs(cmd_p-cur_p) <= 5)
   {
      motspd_p = STOP;
      
   }
   
}

void encoder_read()
{
   // Yaw angle read
   ADMUX = (1 << REFS0) | (1 << ADLAR) | ENC_YAW;      // external vref, choose adc pin(0). read only 8 bit
   ADCSRA = (1 << ADEN) | (1 << ADSC) | (6 << ADPS0);   // start adc read, adc prescalar 64
   while (ADCSRA & (1 << ADSC));   // wait till conversion is finished
   cur_y = ADCW;
   
   // Pitch angle read
   ADMUX = (1 << REFS0) | (1 << ADLAR) | ENC_PIT;      // external vref, choose adc pin(0). read only 8 bit
   ADCSRA = (1 << ADEN) | (1 << ADSC) | (6 << ADPS0);   // start adc read, adc prescalar 64
   while (ADCSRA & (1 << ADSC));   // wait till conversion is finished
   cur_p = ADCW;
   
}


/********************************************************************/
/*                     Fire control                     */
/*                                                   */
/*      - Motor is locked when rocket is firing                  */
/*      - More than two rockets cannot be shoot at the same time   */
/*                                                   */
/*      - Fuse is ignited for 500ms   (can be adjusted)            */
/*      - Buzzer is set when fuse is hot                     */
/*                                                   */
/********************************************************************/
void fire_trig(uint8_t fire)
{
   // Timer init
   if ((fire_toggle != 0) && (fire != 0))
   {
      timer_temp_f = 0;
      timer_100ms_f = 0;
      fire_toggle = 1;
   }
   
   // Ignite fuse for 500ms
   if ((timer_100ms_f < 7) && (fire != 0))
   {
      // Ignite fuse
      switch (fire)
      {
         case ROCKET1:
            BREAK_Y;
            BREAK_P;
            PORTB = ROCKET1;
            break;
            
         case ROCKET2:
            BREAK_Y;
            BREAK_P;
            PORTB = ROCKET2;
            break;
            
         case ROCKET3:
            BREAK_Y;
            BREAK_P;
            PORTB = ROCKET3;
            break;
            
         case ROCKET4:
            BREAK_Y;
            BREAK_P;
            PORTB = ROCKET4;
            break;
            
         case ROCKET5:
            BREAK_Y;
            BREAK_P;
            PORTB = ROCKET5;
            break;
            
         case ROCKET6:
            BREAK_Y;
            BREAK_P;
            PORTB = ROCKET6;
            break;
            
      }
      
      // Buzzer
      PORTD |= BUZZER;
   }
   // After 500ms
   else
   {
      // Turn fuse off
      PORTB = 0;
      
      // Turn Buzzer off
      PORTD &= ~BUZZER;
      
      fire_toggle = 0;
      fire = 0;
      
   }
      

}