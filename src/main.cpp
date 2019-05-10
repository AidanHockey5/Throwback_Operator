#include <Arduino.h>
#include "OPL3.h"

void tick(void);
void Timer_Init();

//Sound Chips
OPL3 opl;

//VGM

void Timer_Init() 
{
  //Output 14.32MHz clock on PA1
  /****configure clocks for PORTA & TIMER2********/
  RCC_BASE->APB2ENR |= RCC_APB2ENR_IOPAEN; // enable port A clock
  RCC_BASE->APB1ENR |= RCC_APB1ENR_TIM2EN; // enable clock for timer 2

  /* Porta Pin 1 &2 as alternate function output Push pull */
  GPIOA->regs->CRL = 0x000000BB;  
  TIMER2_BASE->ARR = 4; // Set Auto reload value
  TIMER2_BASE->PSC = 0; // Set Prescalar value
  /* Output Compare Mode, ENABLE Preload,PWM  mode:2*/
  TIMER2_BASE->CCMR1 = 0x00007800;
  TIMER2_BASE->EGR |= 0x00000001;     // ENABLE update generation
  /*CC2E : channel 2 enabled; polarity : active high*/      
  TIMER2_BASE->CCER = 0x00000010; 
  TIMER2_BASE->CR1 |= 0x00000080;     // Auto preload ENABLE
  TIMER2_BASE->CR1 |= 0x00000001;     // ENABLE Timer counter  
  TIMER2_BASE->CCR2 = 2;              // SET duty cycle to 50%

  Timer4.pause();
  Timer4.setPrescaleFactor(1);
  Timer4.setOverflow(1633);
  Timer4.setChannel1Mode(TIMER_OUTPUT_COMPARE);
  Timer4.attachCompare1Interrupt(tick);
  Timer4.refresh();
  Timer4.resume();  
  
  pinMode(PA0, OUTPUT);
  pinMode(PA2, OUTPUT);
  digitalWrite(PA0, LOW);
  digitalWrite(PA2, LOW);
}

void tick(void) 
{
  //GPIOC->regs->BSRR = (1U << 15) << (16 * ((t >> 0)&1));
}

void setup() 
{
  Timer_Init();
  delay(1);
  opl.Reset();
}

void loop() 
{

}

