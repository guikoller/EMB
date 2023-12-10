#include "LPC17xx.h"
#include <stdio.h>

#define RELOAD_VALUE  9999999
#define ZeitDauer_Sek  4

extern void UART_PutChar(int ,int );
extern int UART_GetChar(int );

extern char *UART_TX_String;

int New_INT0_ButtonState, Old_INT0_ButtonState;

void UART_Start_Char_Transmit(int PortNum, int NumberChar);

int INT0_RisingEdgeDetected = 0;
int INT0_FallingEdgeDetected = 0;
//int INT0_PhaseValue =0;
int INT0_PulsWidthCounter;
//int DebounceThreshold = 2;
volatile int Launch_LCD_Output =0;
volatile int LCD_RefreshTick = 0;

int New_UART_Data_Received = 0;
extern int Selected_UART_PORT;
//static unsigned char *LCD_Output_String;
int volatile RX_MaxNumberChar;
volatile unsigned char *LCD_Output_String;


int U0_RX_NumberOfChar, U0_rx_sw_buffer_index;
int U1_RX_NumberOfChar, U1_rx_sw_buffer_index;
int U2_RX_NumberOfChar, U2_rx_sw_buffer_index;
int U3_RX_NumberOfChar, U3_rx_sw_buffer_index;

int U0_tx_sw_buffer_index, U0_TX_NumberOfChar;
int U1_tx_sw_buffer_index, U1_TX_NumberOfChar;
int U2_tx_sw_buffer_index, U2_TX_NumberOfChar;
int U3_tx_sw_buffer_index, U3_TX_NumberOfChar;

/*unsigned char LCD_U0_ReceivedBytes[500];
unsigned char LCD_U1_ReceivedBytes[500];
unsigned char LCD_U2_ReceivedBytes[500];
unsigned char LCD_U3_ReceivedBytes[500];*/

unsigned char UART0_ReceivedBytes[500];
unsigned char UART1_ReceivedBytes[500];
unsigned char UART2_ReceivedBytes[500];
unsigned char UART3_ReceivedBytes[500];

//unsigned char UART_TX_String[500];

int U0_TX_Running =0;
int U1_TX_Running =0;
int U2_TX_Running =0;
int U3_TX_Running =0;


int AutoBaud_DLL, AutoBaud_DLM;


void Initialize_SysTick(void);
void init_NVIC(void);
void init_Timer0_for_UART(void);

/*****************************************************************************
** Function name:		Initialize_SysTick
**
** Descriptions:		Initialization of the System Timer SysTick
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
void Initialize_SysTick(void){
	// Configure SysTick to trigger every millisecond using the CPU Clock
	SysTick->CTRL = 0;											// Disable the SysTick Module
	/* A clock frequency of 100 MHz is assumed  */
	SysTick->LOAD = RELOAD_VALUE;						// Set the Reload Register for 100ms interrupts
	NVIC_SetPriority(SysTick_IRQn, 1);			// Set the interrupt priority to least urgency
	SysTick->VAL = 0;												// Clear the Current Value register
	SysTick->CTRL = (1<<0)|(1<<1)|(1<<2);		// Enable SysTick, Enable SysTick Exceptions, Use CPU Clock
}



/*****************************************************************************
** Function name:		SysTick_Handler
**
** Descriptions:		IRQ Handler for the System Timer
**                      Used to sample the INT0 signal
**                      in order to launch a transmission of characters
**                      if the button INT0 has been pressed
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
void SysTick_Handler(void)
{
	New_INT0_ButtonState =  ( (1 << 10) & LPC_GPIO2->FIOPIN );
	if (New_INT0_ButtonState){
		//INT0_PhaseValue = 1;
		if (Old_INT0_ButtonState != New_INT0_ButtonState)
			INT0_RisingEdgeDetected = 1;
		else
			INT0_RisingEdgeDetected = 0;
	}
	else {
		//INT0_PhaseValue = 0;
		if (Old_INT0_ButtonState != New_INT0_ButtonState){
			INT0_FallingEdgeDetected = 1;
			INT0_PulsWidthCounter = 0;
		}
		else {
			INT0_FallingEdgeDetected = 0;
			INT0_PulsWidthCounter++;
		}
	}
	Old_INT0_ButtonState = New_INT0_ButtonState;
}



/*****************************************************************************
** Function name:		UART0_IRQHandler
**
** Descriptions:		UART0 interrupt service routine
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
void UART0_IRQHandler(void)
{
	char  IIR_Content, LSR_Content, Dummy;

	//Extract Interrupt Identification Bits = UnIER[3:1]
	IIR_Content = (LPC_UART0->IIR) & ((1<<3)|(1<<2)|(1<<1));

	//Read U0LSR and consequently RLS interrupt is automatically cleared.
	LSR_Content = LPC_UART0->LSR;

	switch(IIR_Content){

		case 0x06: /* Receive Line Status (RLS)*/
			//At least one RX error OE(Overrun Error) or PE(Parity Error) or
			// FE(Frame Error) or BI(Break Interrupt) has occurred
			// Put your code here if RLS error handling required
			if (LSR_Content & 1)
			Dummy = LPC_UART0->LSR; //Remove erroneous byte, if any
			break;

		case 0x04: /* Receive Data Available (RDA)  or*/
		case 0x0C: /*Character Time-Out Indicator (CTI) */
			while (LPC_UART0->LSR & 0x01){
				Dummy = UART_GetChar(0);
				if  ((Dummy == 0x0D) || (Dummy == 0x0A )){  //Carriage return (CR) or New line (NL)
					//Carriage return or New Line considered to be last character in the
					// string given in a terminal
					U0_RX_NumberOfChar = U0_rx_sw_buffer_index;
					U0_rx_sw_buffer_index = 0;
					New_UART_Data_Received = 1;
					//Prepare the pointer for LCD output
					//LCD_U0_ReceivedBytes = &UART0_ReceivedBytes[0];
				}
				else
					UART0_ReceivedBytes[U0_rx_sw_buffer_index++] = Dummy;
			}
			break;

		case 0x02:
			if (U0_TX_Running){
				while (( LPC_UART0->LSR & 0x20) && (U0_tx_sw_buffer_index < (U0_TX_NumberOfChar-2))) {
					Dummy = UART_TX_String[U0_tx_sw_buffer_index++];
					UART_PutChar(0, Dummy);
				}
			}
			if 	(U0_tx_sw_buffer_index == U0_TX_NumberOfChar){
					U0_TX_Running = 0;
					LPC_UART0->IER &= ~(1<<1);
			}
			break;

		default: Dummy++;

	}

	if ( IIR_Content & 0x100){
		AutoBaud_DLL = LPC_UART0->DLL;
		AutoBaud_DLM = LPC_UART0->DLM;
		LPC_UART0->ACR = (1<<0)|(1<<2);
	}
	else
		if ( IIR_Content & 0x200)
			LPC_UART0->ACR = (1<<2);
}




/*****************************************************************************
** Function name:		UART1_IRQHanlder
**
** Descriptions:		UART1 interrupt service routine
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
void UART1_IRQHandler(void)
{
	char  IIR_Content, LSR_Content, Dummy;

	//Extract Interrupt Identification Bits = UnIER[3:1]
	IIR_Content = (LPC_UART1->IIR) & ((1<<3)|(1<<2)|(1<<1));

	//Read U0LSR and consequently RLS interrupt is automatically cleared.
	LSR_Content = LPC_UART1->LSR;

	switch(IIR_Content){

		case 0x06: /* Receive Line Status (RLS)*/
			//At least one RX error OE(Overrun Error) or PE(Parity Error) or
			// FE(Frame Error) or BI(Break Interrupt) has occurred
			// Put your code here if RLS error handling required
			if (LSR_Content & 1)
			Dummy = LPC_UART1->LSR; //Remove erroneous byte, if any
			break;

		case 0x04: /* Receive Data Available (RDA)  or*/
		case 0x0C: /*Character Time-Out Indicator (CTI) */
			while (LPC_UART1->LSR & 0x01){
				Dummy = UART_GetChar(1);
				if  ((Dummy == 0x0D) || (Dummy == 0x0A )){  //Carriage return (CR) or New line (NL)
					//Carriage return or New Line considered to be last character in the
					// string given in a terminal
					U1_RX_NumberOfChar = U1_rx_sw_buffer_index;
					U1_rx_sw_buffer_index = 0;
					New_UART_Data_Received = 1;
					//Prepare the pointer for LCD output
					//LCD_U1_ReceivedBytes = &UART1_ReceivedBytes[0];
				}
				else
					UART1_ReceivedBytes[U1_rx_sw_buffer_index++] = Dummy;
			}
			break;

		case 0x02:
			if (U1_TX_Running){
				while (( LPC_UART1->LSR & 0x20) && (U1_tx_sw_buffer_index < (U1_TX_NumberOfChar-2))) {
					Dummy = UART_TX_String[U0_tx_sw_buffer_index++];
					UART_PutChar(1, Dummy);
				}
			}
			if 	(U1_tx_sw_buffer_index == U1_TX_NumberOfChar){
				U1_TX_Running = 0;
				LPC_UART1->IER &= ~(1<<1);
			}
			break;

		default: Dummy++;

	}

	if ( IIR_Content & 0x100){
		AutoBaud_DLL = LPC_UART1->DLL;
		AutoBaud_DLM = LPC_UART1->DLM;
		LPC_UART1->ACR = (1<<0)|(1<<2);
	}
	else
		if ( IIR_Content & 0x200)
					LPC_UART1->ACR = (1<<2);
}



/*****************************************************************************
** Function name:		UART2_IRQHandler
**
** Descriptions:		UART2 interrupt service routine
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
void UART2_IRQHandler(void)
{
	char  IIR_Content, LSR_Content, Dummy;

	//Extract Interrupt Identification Bits = UnIER[3:1]
	IIR_Content = (LPC_UART2->IIR) & ((1<<3)|(1<<2)|(1<<1));

	//Read U0LSR and consequently RLS interrupt is automatically cleared.
	LSR_Content = LPC_UART2->LSR;

	switch(IIR_Content){

		case 0x06: /* Receive Line Status (RLS)*/
			//At least one RX error OE(Overrun Error) or PE(Parity Error) or
			// FE(Frame Error) or BI(Break Interrupt) has occurred
			// Put your code here if RLS error handling required
			if (LSR_Content & 1)
				Dummy = LPC_UART2->LSR; //Remove erroneous byte, if any
			break;

		case 0x04: /* Receive Data Available (RDA)  or*/
		case 0x0C: /*Character Time-Out Indicator (CTI) */
			while (LPC_UART2->LSR & 0x01){
				Dummy = UART_GetChar(2);
				if  ((Dummy == 0x0D) || (Dummy == 0x0A )){  //Carriage return (CR) or New line (NL)
					//Carriage return or New Line considered to be last character in the
					// string given in a terminal
			    U2_RX_NumberOfChar = U2_rx_sw_buffer_index;
					U2_rx_sw_buffer_index = 0;
					New_UART_Data_Received = 1;
					//Prepare the pointer for LCD output
					//LCD_U2_ReceivedBytes = &UART2_ReceivedBytes[0];
				}
				else
					UART2_ReceivedBytes[U2_rx_sw_buffer_index++] = Dummy;
			}
		break;

		case 0x02:
			if (U2_TX_Running){
				while (( LPC_UART2->LSR & 0x20) && (U2_tx_sw_buffer_index < (U2_TX_NumberOfChar-2))) {
					Dummy = UART_TX_String[U2_tx_sw_buffer_index++];
					UART_PutChar(2, Dummy);
				}
			}
			if 	(U2_tx_sw_buffer_index == U2_TX_NumberOfChar){
				U2_TX_Running = 0;
				LPC_UART2->IER &= ~(1<<1);
			}
			break;

		default: Dummy++;

	}

	if ( IIR_Content & 0x100){
		AutoBaud_DLL = LPC_UART2->DLL;
		AutoBaud_DLM = LPC_UART2->DLM;
		LPC_UART2->ACR = (1<<0)|(1<<2);
	}
	else
		if ( IIR_Content & 0x200)
			LPC_UART2->ACR = (1<<2);
}



/*****************************************************************************
** Function name:		UART3_IRQHandler
**
** Descriptions:		UART3 interrupt service routine
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
void UART3_IRQHandler(void)
{
	char  IIR_Content, LSR_Content, Dummy;

	//Extract Interrupt Identification Bits = UnIER[3:1]
	IIR_Content = (LPC_UART3->IIR) & ((1<<3)|(1<<2)|(1<<1));

	//Read U0LSR and consequently RLS interrupt is automatically cleared.
	LSR_Content = LPC_UART3->LSR;

	switch(IIR_Content){

		case 0x06: /* Receive Line Status (RLS)*/
			//At least one RX error OE(Overrun Error) or PE(Parity Error) or
			// FE(Frame Error) or BI(Break Interrupt) has occurred
			// Put your code here if RLS error handling required
			if (LSR_Content & 1)
				Dummy = LPC_UART3->LSR; //Remove erroneous byte, if any
			break;

		case 0x04: /* Receive Data Available (RDA)  or*/
		case 0x0C: /*Character Time-Out Indicator (CTI) */
			while (LPC_UART3->LSR & 0x01){
				Dummy = UART_GetChar(3);
				if  ((Dummy == 0x0D) || (Dummy == 0x0A )){  //Carriage return (CR) or New line (NL)
					//Carriage return or New Line considered to be last character in the
				    // string given in a terminal
			    U3_RX_NumberOfChar = U3_rx_sw_buffer_index;
					U3_rx_sw_buffer_index = 0;
					New_UART_Data_Received = 1;
					//Prepare the pointer for LCD output
					//LCD_U3_ReceivedBytes = &UART3_ReceivedBytes[0];
				}
				else
					UART3_ReceivedBytes[U3_rx_sw_buffer_index++] = Dummy;
			}
			break;

		case 0x02:
			if (U3_TX_Running){
				while (( LPC_UART3->LSR & 0x20) && (U0_tx_sw_buffer_index < (U3_TX_NumberOfChar-2))) {
					Dummy = UART_TX_String[U3_tx_sw_buffer_index++];
					UART_PutChar(3, Dummy);
				}
			}
			if	(U3_tx_sw_buffer_index == U3_TX_NumberOfChar){
					U0_TX_Running = 0;
					LPC_UART3->IER &= ~(1<<1);
			}
			break;

		default: Dummy++;

	}

	if ( IIR_Content & 0x100){
		AutoBaud_DLL = LPC_UART3->DLL;
		AutoBaud_DLM = LPC_UART3->DLM;
		LPC_UART3->ACR = (1<<0)|(1<<2);
	}
	else
		if ( IIR_Content & 0x200)
			LPC_UART3->ACR = (1<<2);
}






/*****************************************************************************
** Function name:		UART_Start_Char_Transmit
**
** Descriptions: This function sends the first 2 characters per UART and
**               enables UART TX interrupt afterwards
**
** parameters:			PortNum = 0 (for UART0) or 1 (for UART1)
**                                2 (for UART2) or 3 (for UART3)
**                  TX_String = String to be sent
**                  NumberChar
** Returned value:		None
**
*****************************************************************************/
void UART_Start_Char_Transmit(int PortNum, int NumberChar){
	volatile unsigned  int *UxLSR;
	volatile unsigned int *UxIER;

	if (PortNum == 0) //UART0 Selected
		{
			UxLSR = (volatile unsigned int*)(&(LPC_UART0->LSR));
			UxIER = &(LPC_UART0->IER);
			if (NumberChar > 2){
				U0_TX_Running  = 1;
				U0_TX_NumberOfChar = NumberChar;
				U0_tx_sw_buffer_index = 0;
			}
		}
		else if (PortNum == 1) //UART1 Selected
		{
			UxLSR = (volatile unsigned int*) &(LPC_UART1->LSR);
			UxIER = &(LPC_UART1->IER);
			if (NumberChar > 2){
				U1_TX_Running  = 1;
				U1_TX_NumberOfChar = NumberChar;
				U1_tx_sw_buffer_index = 2;
			}
		}
	else if (PortNum == 2) //UART2 Selected
		{
			UxLSR = (volatile unsigned int*) &(LPC_UART2->LSR);
			UxIER = &(LPC_UART2->IER);
			if (NumberChar > 2){
				U2_TX_Running  = 1;
				U2_TX_NumberOfChar = NumberChar;
				U2_tx_sw_buffer_index = 2;
		    }
		}
	else  //UART3 Selected
		{
			UxLSR = (volatile unsigned int*) &(LPC_UART3->LSR);
			UxIER = &(LPC_UART3->IER);
			if (NumberChar > 2){
				U3_TX_Running  = 1;
				U3_TX_NumberOfChar = NumberChar;
				U3_tx_sw_buffer_index = 2;
			}
		}

	if ( *UxLSR & 0x20 ){
		if ( NumberChar >= 2) {
			UART_PutChar(PortNum, *UART_TX_String++); // Send first char without interrupt
			UART_PutChar(PortNum, *UART_TX_String); // Send second char without interrupt
			if ( NumberChar > 2){
				*UxIER  |= 0x02;
				NumberChar = NumberChar - 2;
				UART_TX_String++;
			}
		}
		else
			if ( NumberChar == 1){
				UART_PutChar(PortNum, *UART_TX_String);
				*UART_TX_String = 0;
		}
	}

}

/**********************************************************************
 *   TIMER0 Interrupt used sequencing data outputs to GLCD Display Module
 *   in Lab2 of the lecture EMB
 *   Functions: TIMER0_IRQHandler
 *   LDK / HS Mannheim
 *   History
 *   2015.05.14  ver 0.01
 *********************************************************************/
void TIMER0_IRQHandler (void){

	if (New_UART_Data_Received){
		if (Selected_UART_PORT==0)  {
			//LCD_Output_String = LCD_U1_ReceivedBytes;
			LCD_Output_String = UART0_ReceivedBytes;
			RX_MaxNumberChar =  U0_RX_NumberOfChar;
		}
		else if (Selected_UART_PORT==1)  {
			//LCD_Output_String = LCD_U1_ReceivedBytes;
			LCD_Output_String = UART1_ReceivedBytes;
			RX_MaxNumberChar =  U1_RX_NumberOfChar;
		}
		else if (Selected_UART_PORT==2)  {
			//LCD_Output_String = LCD_U2_ReceivedBytes;
			LCD_Output_String = UART2_ReceivedBytes;
			RX_MaxNumberChar =  U2_RX_NumberOfChar;
		}
		else {
			//LCD_Output_String = LCD_U3_ReceivedBytes;
			LCD_Output_String = UART3_ReceivedBytes;
			RX_MaxNumberChar =  U3_RX_NumberOfChar;
		}
		New_UART_Data_Received = 0;
		Launch_LCD_Output = 1;
	}
	LCD_RefreshTick = 1;
	LPC_TIM0->IR = (1 << 0);  //Clear interrupt
}


void  init_Timer0_for_UART(void) {
	// Put your code here:
	//  :
}

void init_NVIC(void){
	// Put your code here:
	//  :
}
