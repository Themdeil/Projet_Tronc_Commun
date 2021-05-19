//-----------------------------------------------------------------------------
// main_FF-MS1_Master.c
//-----------------------------------------------------------------------------
//// AUTH: Hadrien Barret// DATE: 11/05/21

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <c8051f020.h> // SFR declarations
#include <c8051f020_SFR16.h>





//-----------------------------------------------------------------------------
// Global CONSTANTS and Ports definition
//-----------------------------------------------------------------------------

sbit FF_MS1_SELECT_SLAVE = P2^0;

//-----------------------------------------------------------------------------
// Fonctions prototypes
//-----------------------------------------------------------------------------

void SYSCLK_Init (void);
void WD_disable (void);
void Envoyer_MSG(char c);
void MASTER_SPI0_Configuration(void);
void INT_TRANSMISSION_SPI0(void);
void MASTER_Pins_Configuration(void);


//-----------------------------------------------------------------------------
// MAIN Routine
//-----------------------------------------------------------------------------
void main (void) {

	WD_disable(); // Désactive Watchdog	
	SYSCLK_Init(); // initialize oscillator
  MASTER_SPI0_Configuration();
	MASTER_Pins_Configuration();
	
	while (1){
		int i;
		for(i=0; i<500;i++){}
		Envoyer_MSG(0xFE);
     	
		}
}

//-----------------------------------------------------------------------------
// Sous routine d'init de la clk système
//-----------------------------------------------------------------------------

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
// Sous-routine de desable du watchdog 
//-----------------------------------------------------------------------------
	
void WD_disable (void){
	WDTCN = 0xde; // disable watchdog timer
	WDTCN = 0xad;
	}	

//-----------------------------------------------------------------------------
// Sous-routine de config de la liasion SPI0 pour la carte Master 
//-----------------------------------------------------------------------------
	
void MASTER_SPI0_Configuration(void){
	
	FF_MS1_SELECT_SLAVE = 1;
	
	//Enabled/Disabled : SPIEN, SPI0CN.0
	//Mode master : MSTEN, SPI0CN.1 = 1
	SPI0CN |= (3<<0);
	
	//SPI0CFG[2:0] = 111 pour 8 bits de donnees
	//SPI0CFG[5:3] : savoir quel est le dernier bit qui a ete transfere
	SPI0CFG |= (7<<0);
	
	//Vitesse de transfert = SYSCLK/(2*(SPI0CKR+1))
	SPI0CKR = 0x09; 
}

//-----------------------------------------------------------------------------
// Sous-routines de config des ports pour la carte Master
//-----------------------------------------------------------------------------
void MASTER_Pins_Configuration(void){
	//SPI0
	XBR0 |= (1<<1);
	
	//Crossbar on
	XBR2 |= (1<<6);
	
	//Push-pull
	P0MDOUT |= (1<<0) + (1<<1) + (1<<3); //MISO, NSS MASTER
	P2MDOUT |= (1<<0);
	
}

//-----------------------------------------------------------------------------
// Sous-routines de communication
//-----------------------------------------------------------------------------

void Envoyer_MSG(char c){
	//On dit au slave d'ecouter
	FF_MS1_SELECT_SLAVE = 0;
	
	//Ecriture et debut transfert du code
	SPI0DAT = c;
	SPIF = 0;
	//Attente de la fin de la transmission
	while(SPIF != 1){}
	SPIF = 0;
	//Desactive l'ecoute du slave
	FF_MS1_SELECT_SLAVE = 1;
}


//-----------------------------------------------------------------------------
// Sous-routines des fonctions d'interruption
//-----------------------------------------------------------------------------

void INT_TRANSMISSION_SPI0(void) interrupt 6 {
	SPIF = 0;	
}