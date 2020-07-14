#include <stdint.h>
#include "TM4C123.h"                    // Device header


void ADC_Initialize(void){

	SYSCTL->RCGCGPIO |= 0x00000010;      	// Activated the clock for port E (bit 4 of RCGCGPIO)
	while((SYSCTL->PRGPIO&0x01) == 0){};  // Wait for the clock to stablize
	GPIOE->DIR &= ~0x10;									// Make PE4 input
	GPIOE->AFSEL |= 0x10;									// Enable alternate function on PE4
	GPIOE->DEN &= ~0x10;									// Disable digital function on PE4
	GPIOE->AMSEL |= 0x10;              		// Enable analog pin function on port E pin 4(4 bit of GPIOAMSEL)
	SYSCTL->RCGCADC |= 0x00000001;  			// Activate ADC0
	while((SYSCTL->PRADC&0x01) == 0){};		// Wait for the clock to stablize
	ADC0->PC &= ~0xF;               			// Clear max sample rate field
  ADC0->PC |= 0x1;                			// Configure for 125K samples/sec
	ADC0->SSPRI = 0x0123;           			// Sequencer 3 is lowest priority
	ADC0->ACTSS &= ~0x0008;        				// Disable sample sequencer 3
	ADC0->EMUX &= ~0xF000;         				// Seq3 is software trigger
	ADC0->SSMUX3 &= ~0xFFFFFFF0;       		// Clear SS3 field
	ADC0->SSMUX3 += 9;             				// Set channel AIN9 (PE4)
	ADC0->SSCTL3 = 0x0006;         				// No->TS0,D0, Yes->IE0,END0
  ADC0->ACTSS |= 0x0008;         				// Enable sample sequencer 3
}

unsigned long ADC0_InSeq3(void){  
	
	unsigned long result;
  ADC0->PSSI = 0x0008;            			// Initiate SS3
  while((ADC0->RIS&0x08)==0){};   			// Wait for conversion done
  result = ADC0->SSFIFO3&0xFFF;   			// Read result
  ADC0->ISC = 0x0008;             			// Acknowledge completion
  return result;
}


void PWM_Initialize(void){

	SYSCTL->RCGCGPIO |= 0x02;      				// Activated the clock for port B (bit 1 of RCGCGPIO)
	while((SYSCTL->PRGPIO&0x02) == 0){};  // Wait for the clock to stablize for port B
	GPIOB->DIR |= 0xF0;									  // Make PB4,PB5,PB6,PB7 output
	GPIOB->AFSEL |= 0xF0;									// Enable alternate function on port B pin 4,5,6,7 
	GPIOB->AMSEL &= ~0xF0;              	// Disable analog pin function on port B pin 4,5,6,7
	GPIOB->ODR &= ~0xF0;									// Disable Open drain on port B pin 4,5,6,7
	GPIOB->DEN |= 0xF0;										// Enable digital function on port B pin 4,5,6,7
	GPIOB->PCTL |= (GPIOB->PCTL 
	& 0x0000FFFF) + 0x44440000;						// Set M0PWM2->PB4, M0PWM3->PB5, M0PWM0->PB6, M0PWM1->PB7 digital function
	SYSCTL->RCGCPWM |= 0x00000001;  			// Activate PWM0
	while((SYSCTL->PRPWM&0x01) == 0){};		// Wait for the clock to stablize PWM module 0
	// PWM module gets system clock i.e PWMDIV is not used.
	PWM0_0_CTL_R	=	0;
	
	
}
