#include <LPC17xx.h>
#include "GLCD.h"
#include <stdio.h>
#include "Mytext.h"

extern int INT0_RisingEdgeDetected;
extern int INT0_PulsWidthCounter;
extern volatile unsigned char *LCD_Output_String;

extern void UART_Start_Char_Transmit(int PortNum, int NumberChar);
extern void UART_init(int, int, int, int);
extern void Initialize_SysTick(void);
extern void Initialize_SysTick(void);
extern void init_NVIC(void);
extern void  init_Timer0_for_UART(void);

char *UART_TX_String;


int DebounceThreshold = 2;
extern volatile int Launch_LCD_Output, LCD_RefreshTick;
extern volatile int RX_MaxNumberChar;
unsigned volatile int LCD_Out_NumberLines, ScreenCounter, LineCounter;
unsigned volatile int LCD_Number_ScreenTurns;
unsigned int CharCounter, LCD_line, EndCharIndex;
unsigned char Zeile[21];



int Selected_UART_PORT, UDL, UxFDR_LCR, FIFO_mode, IntEnableValue;

int main (void) {

	int NumberChar,i;
//	unsigned int LCD_Out_NumberLines, ScreenCounter, LineCounter;
//	volatile unsigned int LCD_Number_ScreenTurns;
//	unsigned int CharCounter, LCD_line, EndCharIndex;
//	unsigned char Zeile[20];

	Selected_UART_PORT = 0;
	UDL =163;
	UxFDR_LCR = 0x1003; // 8 Bits, 1 stop bit, no parity, break transmission disabled
	FIFO_mode = 0xC3;	// FIFO enabled, (TX FIFO cleared, RX FIFI cleared), RX FIFO trigger level = 14
	IntEnableValue = 0x01; //RX Interrupt enabled

	UART_init (Selected_UART_PORT , UDL, UxFDR_LCR, ((IntEnableValue << 8) | FIFO_mode));

	/******************************************************************************
	      AUTO-BAUD  Measurement
	*******************************************************************************/
	// UART0
	//LPC_UART0->ACR = (1<<0)|(1<<2);	//UART0  Auto-Baud start and Restart //Mode 0
	//LPC_UART1->ACR = (1<<0)|(1<<2);	//UART1  Auto-Baud start and Restart //Mode 0
	//LPC_UART2->ACR = (1<<0)|(1<<2);	//UART2  Auto-Baud start and Restart //Mode 0
	//LPC_UART3->ACR = (1<<0)|(1<<2);	//UART3  Auto-Baud start and Restart //Mode 0

	LPC_PINCON->PINSEL4 &= ~(3 << 20);			//Clear the bits to be updated
																					//PORT2.10 (INT0 Pin) as GPIO

	Initialize_SysTick( );
	init_NVIC();
	init_Timer0_for_UART();

	LPC_SC->PCONP |= (1 << 22);

	Selected_UART_PORT = 0;

	GLCD_Init();																			/* Initialize the GLCD */
	GLCD_Clear(White);																/* Clear the GLCD */
	GLCD_SetBackColor(Blue);													/* Set the Back Color */
	GLCD_SetTextColor(White);													/* Set the Text Color */

	//GLCD_SetTextColor(Black);												/* Set the Text Color */
	GLCD_DisplayString(0, 0, "EMB Versuch 2: UART ");	/* print string to LCD */
	GLCD_DisplayString(1, 0, "UARTx has received: ");	/* print string to LCD */

	GLCD_SetBackColor(White);													/* Set the Back Color */
	GLCD_SetTextColor(Blue);													/* Set the Text Color */



	while (1)  {
		//Data to be sent over UARTx if button INT0 has been pressed
		if (INT0_RisingEdgeDetected==1){
		 if ( INT0_PulsWidthCounter  > DebounceThreshold ) {
				UART_TX_String = MyText;
				NumberChar = 0;
				while(*UART_TX_String++) NumberChar++;
				UART_TX_String = 	MyText;
				if ( NumberChar > 0)
					UART_Start_Char_Transmit(Selected_UART_PORT, NumberChar);
			}
		}
		INT0_RisingEdgeDetected = 0;

		// Shall Data on Display be updated?
		if (Launch_LCD_Output){
			LCD_RefreshTick = 0;
			LCD_Out_NumberLines = (int) ((RX_MaxNumberChar + (20 -1)) /20); // Assuming 20 Chars per Line
			LCD_Number_ScreenTurns = (int) ((LCD_Out_NumberLines + (6 -1))/ 6);//Assuming 6 lines for data per screen

			//Clear Lines 2 to 9
			for (i=2;i < 9; i++){
					GLCD_ClearLn(i);
			}

			for (ScreenCounter = 0; ScreenCounter < LCD_Number_ScreenTurns; ScreenCounter++){
				for (LineCounter= 6*ScreenCounter; ((LineCounter < (6*(ScreenCounter+1))) &&
						(LineCounter < LCD_Out_NumberLines )) ; LineCounter++){
					if (( LineCounter < (LCD_Out_NumberLines - 1)) && ( LCD_Out_NumberLines > 1)){ //Not yet on last line
						for (CharCounter = 0; CharCounter < 20; CharCounter++){
							Zeile[CharCounter] = *(LCD_Output_String + LineCounter*20 + CharCounter);
						}
					}
					else {
						EndCharIndex = RX_MaxNumberChar - 20*(LCD_Out_NumberLines - 1);
						for (CharCounter = 0; CharCounter < EndCharIndex; CharCounter++){
							Zeile[CharCounter] = *(LCD_Output_String + LineCounter*20 + CharCounter);
						}
						if ( CharCounter == EndCharIndex ){
							for (CharCounter = EndCharIndex; CharCounter < 20; CharCounter++)
								Zeile[CharCounter] = 0;
						}
					}
					LCD_line = (LineCounter%6) + 3;
					GLCD_DisplayString(LCD_line, 0, Zeile);
				}
			  if ( ScreenCounter < (LCD_Number_ScreenTurns-1)){
					while(LCD_RefreshTick ==0);
					// Clear lines 2 to 9 to prepare new screen
					for (i=2;i < 9; i++){
						GLCD_ClearLn(i);
						LCD_RefreshTick =0;
					}
				}
			}
			Launch_LCD_Output = 0;
		}
	}
}
