#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "TM4C123.h"                    // Device header
#include "os.h"                     		// Micrium.Micrium::RTOS:uC/OS Kernel
#include "PLL.h"

OS_TCB AppTaskStartTCB;									// TCB for all the Tasks
OS_TCB Adc0Seq3TCB;
OS_TCB PWMDutySetTCB;
OS_TCB InverterPllTCB;
OS_TCB DCConverterMpptTCB;

CPU_STK Adc0Seq3STK[128u];							//STK for all the tasks
CPU_STK AppTaskStartSTK[128u];
CPU_STK PWMDutySetSTK[128u];
CPU_STK InverterPllSTK[128u];
CPU_STK DCConverterMpptSTK[128u];

OS_ERR os_err;													// Error for OS

void AppTaskStartTASK(void *p_arg);			// declare all the Tasks
void Adc0Seq3TASK(void *p_arg);
void PWMDutySetTASK(void *p_arg);
void InverterPllTASK(void *p_arg);
void DCConverterMpptTASK(void *p_arg);

void ADC_Initialize(void);							// Initializations of PWM & ADC
void PWM_Initialize(void);

unsigned long ADC_ReadData(void);				// other functions
_Bool ZeroCrossDetector(unsigned long x);
float LowPassFilter(bool x, float y);

volatile bool XOR,PreXOR=0;   					// Delcare all the PLL variables
	volatile int GAIN =250;								// Set gain, we need to adjust gain
	volatile float LPF,PreLPF=0,dt=400e-6;
	volatile double W=0,H=0,A0=0,A1=0,A2=0;
	volatile double X=0,Y=0,PreX=0,PreY=1;

unsigned long PllInput,MpptInputV,MpptInputI;			// Global Variable for PLL, MPPT
double PLLOutput=0,PLLOutputPos,PLLOutputNeg;
unsigned long AppStarterTaskCOUNT=0,count=0,count1=0;
void delay(int milli_number_of_seconds);

int main(){
	//PLL_Init();
	
	PWM_Initialize();
	
	OSInit(&os_err);
	OSSchedRoundRobinCfg(1,10u,&os_err);  // Configure Roundrobin Scheduling
	
	OSTaskCreate(	 (OS_TCB     *)&AppTaskStartTCB,         // Create AppStarter task which is kind of idle task and creates other tasks
                 (CPU_CHAR   *)"APP STARTER TASK",
                 (OS_TASK_PTR ) AppTaskStartTASK,
                 (void       *) 0,
                 (OS_PRIO     ) 1,
                 (CPU_STK    *)&AppTaskStartSTK[0],
                 (CPU_STK     ) 0u,
                 (CPU_STK_SIZE) 128u,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);
	
	OSStart(&os_err); 																		// OS start
}



void AppTaskStartTASK(void *p_arg){      //AppStarter Task

	OSTaskCreate(	 (OS_TCB     *)&Adc0Seq3TCB,           // Create all other task
                 (CPU_CHAR   *)"ADC 0 SEQ 3 TASK",
                 (OS_TASK_PTR ) Adc0Seq3TASK,
                 (void       *) 0,
                 (OS_PRIO     ) 1,
                 (CPU_STK    *)&Adc0Seq3STK[0],
                 (CPU_STK     ) 0u,
                 (CPU_STK_SIZE) 128u,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);
						
	OSTaskCreate(	 (OS_TCB     *)&PWMDutySetTCB, 
                 (CPU_CHAR   *)"INVERTER PWM & SPWM",
                 (OS_TASK_PTR ) PWMDutySetTASK,
                 (void       *) 0,
                 (OS_PRIO     ) 1,
                 (CPU_STK    *)&PWMDutySetSTK[0],
                 (CPU_STK     ) 0u,
                 (CPU_STK_SIZE) 128u,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);
								 
	OSTaskCreate(	 (OS_TCB     *)&InverterPllTCB, 
                 (CPU_CHAR   *)"INVERTER PLL",
                 (OS_TASK_PTR ) InverterPllTASK,
                 (void       *) 0,
                 (OS_PRIO     ) 1,
                 (CPU_STK    *)&InverterPllSTK[0],
                 (CPU_STK     ) 0u,
                 (CPU_STK_SIZE) 128u,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);
								 
	OSTaskCreate(	 (OS_TCB     *)&DCConverterMpptTCB, 
                 (CPU_CHAR   *)"MPPT",
                 (OS_TASK_PTR ) DCConverterMpptTASK,
                 (void       *) 0,
                 (OS_PRIO     ) 1,
                 (CPU_STK    *)&DCConverterMpptSTK[0],
                 (CPU_STK     ) 0u,
                 (CPU_STK_SIZE) 128u,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);
								 
	
	while(DEF_TRUE){
		if (AppStarterTaskCOUNT >= 4294967295){
				AppStarterTaskCOUNT = 0;
		}
		AppStarterTaskCOUNT++;
		OSSchedRoundRobinYield(&os_err);
	}

}

void Adc0Seq3TASK(void *p_arg){
	
	ADC_Initialize();
	
	while(DEF_TRUE){
		
	PllInput = ADC_ReadData();																					// Get PLL ADC Input
		
	OSSchedRoundRobinYield(&os_err);																		// Yield the Adc0Seq3 TASK
	}
}

void PWMDutySetTASK(void *p_arg){

	//PWM_Initialize();																										// 80 MHZ
	
	while(DEF_TRUE){
		
		if((PLLOutput>0)){
			PWM0->_0_CMPA =	((PWM0->_0_LOAD-1) - ((PWM0->_0_LOAD-1)*(PLLOutput))); 	// Set gen0 cmp to load - load *(x/2)
			PWM0->_0_CMPB = 0;																	// Set gen0 cmpb = 4095
			PWM0->_1_CMPA = (PWM0->_0_LOAD)-1;																							// Set gen1 cmpa = 0
			PWM0->_1_CMPB = (PWM0->_0_LOAD)-1;
			PWM0->ENUPD |= 0x0;
		}
		if((PLLOutput<0)){
			PWM0->_0_CMPA =	(PWM0->_0_LOAD)-1;																							// Set gen0 cmpa = 0
			PWM0->_0_CMPB = (PWM0->_0_LOAD)-1;	
			PWM0->_1_CMPA = ((PWM0->_1_LOAD-1) + ((PWM0->_1_LOAD-1)*(PLLOutput)));	// Set gen1 cmpa = load + load *(x/2)  as x will be negative
			PWM0->_1_CMPB = 0;				// Set gen0 cmpb = 0
			PWM0->ENUPD |= 0x0;
		}
		
		OSSchedRoundRobinYield(&os_err);																	// Yield the PWMDutySetTASK task
	}
}

void InverterPllTASK(void *p_arg){
	
	
	
	while(DEF_TRUE){
		
		XOR=(ZeroCrossDetector(PllInput))^(ZeroCrossDetector(PreX)); 	// XOR gives the phase difference between the input and the output 
		LPF = LowPassFilter(PreXOR,PreLPF);														// Pass it throught the Low pass filter (1st Order)
		W=GAIN*LPF;																										// multiply with gain to obtain omega 
		H=W*dt;																												// step size decides how smooth is the harmonic oscillator wave should be
		A0=H*H/4;																									// 2 nd order Harmonic oscillator equations
		A1=(1-A0)/(1+A0);
		A2=H/(1+A0);
			
		X=(A1*PreX)+(A2*PreY);																				// Cosine Instance output harmonic oscillator
		Y=PreY-((H/2)*PreX)-((H/2)*X);																// Sine Instance output from harmonic oscillator
		PLLOutput  = Y;
		//PLLOutputNeg  = (-Y+1)/2;
		
		PreY=Y;																												// Store the previous values
    PreX=X;
    PreLPF=LPF;
    PreXOR=XOR;
		
		OSSchedRoundRobinYield(&os_err);															// Yield the InverterPLL task
	}
}

void DCConverterMpptTASK(void *p_arg){
	
	while(DEF_TRUE){
				
		OSSchedRoundRobinYield(&os_err);														// Yield the DCConverterMppt task
	}
}

void ADC_Initialize(void){

	SYSCTL->RCGCGPIO |= 0x10;      				// Activated the clock for port E (bit 4 of RCGCGPIO)
	while((SYSCTL->PRGPIO&0x10) == 0){};  // Wait for the clock to stablize
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

unsigned long ADC_ReadData(void){  
	
	unsigned long result;
  ADC0->PSSI = 0x0008;            			// Initiate SS3
  while((ADC0->RIS&0x08)==0){};   			// Wait for conversion done
  result = ADC0->SSFIFO3&0xFFF;   			// Read result unsigned long -32-bit
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
			SYSCTL->RCC	=	0x00100000	|									//	3)	use	PWM	divider				
		((SYSCTL->RCC	&	(~0x000E0000))	+	//				clear	PWM	divider	field				
		0x00000000);
	SYSCTL->RCGCPWM |= 0x00000001;  			// Activate PWM0
	while((SYSCTL->PRPWM&0x01) == 0){};		// Wait for the clock to stablize PWM module 0
		PWM0->_0_CTL =  
	// PWM module gets system clock i.e PWMDIV is not used.
	PWM0->_0_CTL	=	0;									// PWM0 gen0 is disabled and configured to default state if enabled. ( i.e count down mode)
	PWM0->_1_CTL	=	0x0;									// PWM0 gen1 is disabled and configured to default state if enabled. ( i.e count down mode)
	PWM0->_0_GENA = 0xC8;									// Set PB6 low when counter==load and set high when counter == cmpa (gen0)
	PWM0->_0_GENB = 0xC08;								// Set PB7 low when counter==load and set high when counter == cmpb (gen0)
	PWM0->_1_GENA = 0xC8;									// Set PB4 low when counter==load and set high when counter == cmpa (gen1)
	PWM0->_1_GENB = 0xC08;								// Set PB5 low when counter==load and set high when counter == cmpb (gen1)
	PWM0->INVERT = 0xF;
	PWM0->_0_LOAD = 0xFFF;								// Set load value == 4095 for gen0
	PWM0->_1_LOAD = 0xFFF;								// Set load value == 4095 for gen1
	PWM0->_0_CMPA =	0x7FF;								// Set gen0 cmpa = 2047
	PWM0->_0_CMPB = 0x19A;								// Set gen0 cmpb = 2047
	PWM0->_1_CMPA = 0xFFC;								// Set gen1 cmpa = 2047
	PWM0->_1_CMPB = 0xA4;									// Set gen1 cmpb = 2047
	PWM0->_0_CTL	|= 0x01;								// PWM0 gen0 is enabled
	PWM0->_1_CTL	|= 0x01;								// PWM0 gen1 is enabled
	PWM0->ENABLE  |= 0X0F;								// Drives the gen0,gen1 outputs to pins. (pwm0A->m0pwm0, pwm0B->m0pwm1, pwm1A->m0pwm2,pwm1B->m0pwm2) 
}

_Bool ZeroCrossDetector(unsigned long x) {   //Zero cross detector

	if(x>=0x7FF){   													// Converts to 1 if above 2047 else 0
		return 1;
	}
	else {
	return 0;
		
	}
}

float LowPassFilter(bool x, float y){ // retruns the Low pass filter output.
	return y+(0.001*(x-y));     				//this is an equation implemented
}
 