	PRESERVE8
	THUMB
	
	AREA CODE, CODE, READONLY
		
	IMPORT OS_CPU_PendSVHandler
	IMPORT OS_CPU_SysTickHandler
		
	EXPORT PendSV_Handler
PendSV_Handler
	B OS_CPU_PendSVHandler
	B .
	
	EXPORT SysTick_Handler
SysTick_Handler
	B OS_CPU_SysTickHandler
	B .
	
	END