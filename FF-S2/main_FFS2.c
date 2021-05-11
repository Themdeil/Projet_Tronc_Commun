//-----------------------------------------------------------------------------
// main_FFS2.c
//-----------------------------------------------------------------------------
//// AUTH: Hadrien Barret// DATE: 28/04/21

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <c8051f020.h> // SFR declarations

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


//-----------------------------------------------------------------------------
// Global CONSTANTS
//----------------------------------------------------------------------------

#define SYSCLK       22118400          
// SYSCLK frequency in Hz
#define BAUDRATE     115200
// Baud rate of UART in bps

//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------

void SYSCLK_Init (void);

void UART0_Init (void);

void PORT_Init (void);

char Lecture_Guidage (void);

char Recup_char (void);

//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------

char val_guidage;


//-----------------------------------------------------------------------------
// MAIN Routine
//-----------------------------------------------------------------------------

void main (void) {

	WDTCN = 0xde; // disable watchdog timer
	WDTCN = 0xad;
	
	SYSCLK_Init(); // initialize oscillator
	
	PORT_Init(); // initialize IO for UART

	UART0_Init(); // initialize UART0   
	
	while(1)
		{
			if(RI0 == 1)
				{
				val_guidage = Recup_char(); // On récupère la valeur reçue.
				}
				
		
		
		
		}
	
}


//-----------------------------------------------------------------------------
// Sous routines d'initialisation du système de réception
//-----------------------------------------------------------------------------
// SYSCLK_Init
//-----------------------------------------------------------------------------//
// This routine initializes the system clock to use an 22.1184MHz crystal
// as its clock source.
//
void SYSCLK_Init (void){
	int i;
	// delay counter
	OSCXCN = 0x67;
	// start external oscillator with
	// 22.1184MHz crystal
	for (i=0; i < 256; i++) ;
	// wait for XTLVLD to stabilize
	while (!(OSCXCN & 0x80)) ;
	// Wait for crystal osc. to settle
	OSCICN = 0x88;
	// select external oscillator as SYSCLK
	// source and enable missing clock
	// detector
	}

	
	//-----------------------------------------------------------------------------
// UART0_Init  Configure the UART0 using Timer1, for <baudrate> and 8-N-1.
//-----------------------------------------------------------------------------
void UART0_Init (void){
			SCON0  = 0x50; // SCON0: mode 1, 8-bit UART, enable RX
			TMOD   = 0x20; // TMOD: timer 1, mode 2, 8-bit reload 
			TH1    = -(SYSCLK/BAUDRATE/16); // set Timer1 reload value for baudrate
			TR1    = 1; // start Timer1
			CKCON |= 0x10; // Timer1 uses SYSCLK as time base
			PCON  |= 0x80; // SMOD00 = 1
}

//-----------------------------------------------------------------------------
// PORT_Init Configure the Crossbar and GPIO ports
//-----------------------------------------------------------------------------
void PORT_Init (void){
		XBR0    |= 0x04;
		// Enable UART0
		XBR2    |= 0x64;
		// Enable crossbar and weak pull-ups  
		P0MDOUT |= 0x01;
		// enable TX0 as a push-pull output  
		
		}

		
//-----------------------------------------------------------------------------
// Lecture_Guidage // fonction qui retourne la dernière val de guidage 
//-----------------------------------------------------------------------------		
char Lecture_Guidage(void)
	{
		
	return val_guidage;
	}	

//-----------------------------------------------------------------------------
// Recup_char // fonction qui recupere la val envoyée par la balise  
//-----------------------------------------------------------------------------	
char Recup_char(void)
{
		
	char c;

	//Desactive la reception
	RI0 = 0;
	REN0 = 0;
	
	//Recup le caractere
	c = SBUF0;
	
	//Reactive la reception
	REN0 = 1;
	
	return c;
}
