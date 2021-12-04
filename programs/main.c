/* ###################################################################
**     Filename    : main.c
**     Project     : Lab6_Part2
**     Processor   : MK64FN1M0VLL12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2019-11-03, 17:50, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
 ** @file main.c
 ** @version 01.01
 ** @brief
 **         Main module.
 **         This module contains user's application code.
 */
/*!
 **  @addtogroup main_module main module documentation
 **  @{
 */
/* MODULE main */


/* Including needed modules to compile this module/procedure */
#include "Cpu.h"
#include "Events.h"
#include "Pins1.h"
#include "FX1.h"
#include "GI2C1.h"
#include "WAIT1.h"
#include "CI2C1.h"
#include "CsIO1.h"
#include "IO1.h"
#include "MCUC1.h"
#include "SM1.h"
/* Including shared modules, which are used for whole project */
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "PDD_Includes.h"
#include "Init_Config.h"
//#include "fsl_device_registers.h"
#include <stdint.h>

void Dispenser();
void Valve_Start();

void software_delay(unsigned long delay)
{
while (delay > 0) delay--;
}

int maximum(int arr[]){
int max=arr[1];
if(arr[0]>arr[1]){max=arr[0];}
if(arr[2]>max){max=arr[2];}
return max;
}

unsigned short ADC_read16b(void);

/* User includes (#include below this line is not maintained by Processor Expert) */

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */

unsigned char write[512];
unsigned int i = 0;
unsigned int temp = 1;

enum Liquid_States {Liquid_Start, Liquid_Setup, Set_Open, Set_Wait, Set_Close, done} Liquid_State;
enum Dispenser {Start, Base, Delay, Activate} states;

int percent[3]={50,25,25};
unsigned int ratios[3] = {0,0,0};
unsigned int counter = 0;
unsigned int flag = 0;
unsigned int Isum=0;
unsigned short c=0,o=0,p=0,j=0,k=0,l=0;
unsigned int decoder0[8] = { 0x04, 0x06, 0x02, 0x0A, 0x08, 0x09, 0x01, 0x05};
unsigned int decoder1[8] = { 0x400, 0x408, 0x008, 0x808, 0x800, 0x804, 0x004, 0x404};
unsigned int decoder2[8] = { 0x400, 0x600, 0x200, 0xA00, 0x800, 0x900, 0x100, 0x500};
uint32_t Input = 0x00, ROT_DIR1 = 0, ROT_DIR2 = 0, counter1 = 0;
unsigned long delay = 7000;


int main(void){
	PE_low_level_init();

	SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK; /* Enable Port C Clock Gate Control*/
	SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK; /* Enable Port B Clock Gate Control*/

	PORTC_GPCLR = 0x00F0F0100; /* Configures Pins 0-3 and 8-11 on Port C to be GPIO */
	PORTB_GPCLR = 0x00C0C0100; /* Configures Pins 0-3 and 8-11 on Port B to be GPIO */

	GPIOC_PDDR = 0x000000F0F; /* Configures Pins 0-3 and 8-11 on Port C to be Output */
	GPIOB_PDDR = 0x000000C0C; /* Configures Pins 0-3 and 8-11 on Port B to be Output */





	/* Write your local variable definition here */

	/*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/

	/*** End of Processor Expert internal initialization.                    ***/
	/* Write your code here */
	uint8_t who;

	int len;
	LDD_TDeviceData *SM1_DeviceData;
	SM1_DeviceData = SM1_Init(NULL);


	FX1_Init();

	SIM_SCGC6 |= SIM_SCGC6_ADC0_MASK; // 0x8000000u; Enable ADC0 Clock
	ADC0_CFG1 = 0x0C; // 16bits ADC; Bus Clock
	ADC0_SC1A = 0x1F; // Disable the module, ADCH = 11111


	for(;;) {
		if (FX1_WhoAmI(&who)!=ERR_OK) {
			return ERR_FAILED;
		}
		if(flag == 0) {
			Dispenser();
			for(unsigned int j = 0; j < 300000; j++); //delay
			len = sprintf(write, "%d\n", i);
			SM1_SendBlock(SM1_DeviceData, &write, len);
		}
		if(flag == 1){
			Valve_Start();
		}
		//printf("%d\n", ADC_read16b());


	}


	/* For example: for(;;) { } */

	/*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

unsigned short ADC_read16b(void) {
	ADC0_SC1A = 0x00; //Write to SC1A to start conversion from ADC_0
	while(ADC0_SC2 & ADC_SC2_ADACT_MASK); // Conversion in progress
	while(!(ADC0_SC1A & ADC_SC1_COCO_MASK)); // Until conversion complete
	return ADC0_RA;

}

void Dispenser() {

	switch(states) {//Transitions

		case Start:
		i = 0;
		ratios[0] = 0;
		ratios[1] = 0;
		ratios[2] = 0;
		counter   = 0;
		states = Base;
		break;

		case Base:
		if(ADC_read16b() > 60000) {//Press
				ratios[counter] = i;
				i = 'a'; //Press value
				states = Delay; //Rising edge
		}
		else if(ADC_read16b() > 45000) {//Right
			if(i < 9) {
				i = i + 1;
			}
			states = Base;
		}
		else if(ADC_read16b() < 10000) {//Left
			if(i > 0) {
				i = i - 1;
			}
			states = Base;
		}
		//for(unsigned int j = 0; j < 50000; j++); //delay
		break;

		case Delay:
		//for(unsigned int j = 0; j < 300000; j++); //delay
		if(ADC_read16b() < 60000) {
			i = 0;
			if(counter != 3){
				states = Base;
				counter = (counter + 1);
			}
			printf("%d ", ratios[0]);
			printf("%d ", ratios[1]);
			printf("%d\n", ratios[2]);
		}
		if(counter == 3) {
			flag = 1;
			counter = 0;
			Isum=ratios[0]+ratios[1]+ratios[2];
			percent[0] = ratios[0]*100/(Isum);
			percent[1] = ratios[1]*100/(Isum);
			percent[2] = ratios[2]*100/(Isum);

			states = Start;
		}
		break;

		case Activate:
			states = Base;
		break;
	}

	switch(states) {//States
		case Start:

		break;

		case Base:

		break;

		case Delay:
		//for(unsigned int j = 0; j < 300000; j++); //delay
			i = 0;
		break;

		case Activate:

		break;

	}

}

void Valve_Start(){
	//action that happens for all
	switch(Liquid_State){//transitions
	case Liquid_Start:
	j=0;
	k=0;
	l=0;
	counter1=0;
	Liquid_State=Liquid_Setup;
	if (ratios[0]==0 && ratios[1]==0 && ratios[2]==0){
		Liquid_State=Liquid_Start;
		flag=0;
	}
	break;
	case Liquid_Setup:

	Liquid_State=Set_Open;

	break;
	case Set_Open:
	if(j<=1024){
		if(percent[0]>0){c=(c+1)%8;}
		if(percent[1]>0){
		if(o>0){o=(o-1)%8;}
		else{o=7;}
		}
		if(percent[2]>0){p=(p+1)%8;}
		j=j+1;
		Liquid_State=Set_Open;
	}
	else{
		j=0;
		k=0;
		l=0;
		Liquid_State=Set_Wait;
	}

	break;
	case Set_Wait:
	counter1+=1;

	if(counter1>40*percent[0] && j<=1024 && percent[0]>0){
		if(c>0){c=(c-1)%8;}
		else{c=7;}
		j=j+1;
	}
	if(counter1>40*percent[1] && k<=1024 && percent[1]>0){
		o=(o+1)%8;
		k=k+1;
	}
	if(counter1>40*percent[2] && l<=1024 && percent[2]>0){
		if(p>0){p=(p-1)%8;}
		else{p=7;}
		l=l+1;
	}
	Liquid_State=Set_Wait;
	if(counter1>8000+maximum(percent)*20){
		j=0;
		k=0;
		l=0;
		counter1=0;
		flag=0;
		Liquid_State=Liquid_Start;
	}
	break;
	case Set_Close:

	break;
	case done:

	Liquid_State=Liquid_Setup;
	break;
	default:
	Liquid_State=Liquid_Start;
	break;
	}//transitions

	switch(Liquid_State){//state actions
	case Liquid_Start:
	break;
	case Liquid_Setup:
	case Set_Open:
	case Set_Wait:
	case Set_Close:
	GPIOC_PDOR = (decoder2[c] | decoder0[p]);
	GPIOB_PDOR = decoder1[o];
	software_delay(delay);
	case done:
	break;

	}
	}

/* END main */
/*!
 ** @}
 */
/*
 ** ###################################################################
 **
 **     This file was created by Processor Expert 10.4 [05.11]
 **     for the Freescale Kinetis series of microcontrollers.
 **
 ** ###################################################################
 */
