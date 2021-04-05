//-----------------------------------------------------------------------------
// UART0_Int1.c
//-----------------------------------------------------------------------------
// Copyright 2001 Cygnal Integrated Products, Inc.
//// AUTH: BW// DATE: 28 AUG 01
//// This program configures UART0 to operate in interrupt mode, showing an
// example of a string transmitter and a string receiver.  These strings are
// assumed to be NULL-terminated.
//// Assumes an 22.1184MHz crystal is attached between XTAL1 and XTAL2.
// // The system clock frequency is stored in a global constant SYSCLK.  The
// target UART baud rate is stored in a global constant BAUDRATE.//
// Target: C8051F02x
// Tool chain: KEIL C51 6.03 / KEIL EVAL C51//
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <c8051f020.h> // SFR declarations
#include <stdio.h>
#include <string.h>
#include <FO_M1__Structures_COMMANDES_INFORMATIONS_CentraleDeCommande.h>

//-----------------------------------------------------------------------------
// 16-bit SFR Definitions for ‘F02x
//-----------------------------------------------------------------------------
sfr16 DP       = 0x82;                 
// data pointer
sfr16 TMR3RL   = 0x92;
// Timer3 reload value
sfr16 TMR3     = 0x94;
// Timer3 counter
sfr16 ADC0     = 0xbe;
// ADC0 data
sfr16 ADC0GT   = 0xc4;
// ADC0 greater than window
sfr16 ADC0LT   = 0xc6;
// ADC0 less than window
sfr16 RCAP2    = 0xca;
// Timer2 capture/reload
sfr16 T2       = 0xcc;
// Timer2
sfr16 RCAP4    = 0xe4;
// Timer4 capture/reload
sfr16 T4       = 0xf4;
// Timer4
sfr16 DAC0     = 0xd2;
// DAC0 data
sfr16 DAC1     = 0xd5;
// DAC1 data

//-----------------------------------------------------------------------------
// Global CONSTANTS
//-----------------------------------------------------------------------------
#define SYSCLK       22118400          
// SYSCLK frequency in Hz
#define BAUDRATE     19200
// Baud rate of UART in bps

sbit LED = P1^6; // LED = 1 means ON

struct COMMANDES commandes; //On declare une structure
//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------
void SYSCLK_Init (void);
void PORT_Init (void);
void UART0_Init (void);



void HQ_CM (void);
void CM_HQ (void);

void Send_char (char);
void Send_string(char*);
char Recup_char(void);

char separ_cmd(char*, char*);
void analyse_cmd(void);

void Reset_buff_ptr(void);


//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------
bit TX_Ready;
// ‘1’ means okay to TX
char *TX_ptr;
// pointer to string to transmit
bit RX_Ready;
// ‘1’ means RX string received

char RX_Buf[50]="";
char* RX_ptr=&RX_Buf[0];
// receive string storage buffer

char CMD[15]=""; // Buffer de sauvegarde de la commande
char* ptr_CMD = &CMD[0];

char PARAM_1[10]=""; // Buffer de sauvegarde des param 1
char* ptr_PARAM_1 = &PARAM_1[0];

char PARAM_2[10]=""; // Buffer de sauvegarde des param 2
char* ptr_PARAM_2 = &PARAM_2[0];

char PARAM_3[10]=""; // Buffer de sauvegarde des param 3
char* ptr_PARAM_3 = &PARAM_3[0];

char PARAM_4[10]=""; // Buffer de sauvegarde des param 4
char* ptr_PARAM_4 = &PARAM_4[0];




char test[5];

int nb_cmd;
int s_cmd;
int commande_valide;

char *p_test;
//-----------------------------------------------------------------------------
// MAIN Routine
//-----------------------------------------------------------------------------
void main (void) {

	WDTCN = 0xde; // disable watchdog timer
	WDTCN = 0xad;
	
	SYSCLK_Init(); // initialize oscillator
	PORT_Init (); // initialize crossbar and GPIO   
	UART0_Init (); // initialize UART0   
	
	Send_string("Tapez une cmd ");
	
	while (1){
      if(RI0 == 1)
				{
					HQ_CM();
				}
      TX_Ready = 0; // claim transmitter
      TX_ptr = test; // set TX buffer pointer to point to
			// received message
			TI0 = 1; // start transmit
      while (!TX_Ready); // wait for transmission to complete
      TX_Ready = 0;
      TX_ptr = "\r\n>"; // send CR+LF+>
			TI0 = 1; // start transmit  
			while (!TX_Ready) ; // wait for transmission to complete
      RX_Ready = 0; // free the receiver   
		}
}


//-----------------------------------------------------------------------------
// Initialization Subroutines
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
// PORT_Init
//-----------------------------------------------------------------------------
//
// Configure the Crossbar and GPIO ports
//
	void PORT_Init (void){
		XBR0    |= 0x04;
		// Enable UART0
		XBR2    |= 0x40;
		// Enable crossbar and weak pull-ups  
		P0MDOUT |= 0x01;
		// enable TX0 as a push-pull output  
		P1MDOUT |= 0x40;
		// enable LED as push-pull output
		}
	//-----------------------------------------------------------------------------
	// UART0_Init
	//-----------------------------------------------------------------------------
		//
		// Configure the UART0 using Timer1, for <baudrate> and 8-N-1.
		//
		void UART0_Init (void){
			SCON0  = 0x50; // SCON0: mode 1, 8-bit UART, enable RX
			TMOD   = 0x20; // TMOD: timer 1, mode 2, 8-bit reload 
			TH1    = -(SYSCLK/BAUDRATE/16); // set Timer1 reload value for baudrate
			TR1    = 1; // start Timer1
			CKCON |= 0x10; // Timer1 uses SYSCLK as time base
			PCON  |= 0x80; // SMOD00 = 1
			ES0    = 1; // enable UART0 interrupts
			TX_Ready = 1; // indicate TX ready for transmit 
			RX_Ready = 0; // indicate RX string not ready
			TX_ptr = NULL;
}
		

//-----------------------------------------------------------------------------
// HQ_CM Sous-routine d'écriture de Commande 
//-----------------------------------------------------------------------------
void HQ_CM(void)
	{
		char char_unique;
		
		char_unique = Recup_char(); // On récupère le caractère reçu.
		
		*RX_ptr = char_unique; // On stocke ds le buffer tout les caractères.
		*(RX_ptr + 1) = '\0';
		RX_ptr++;
		
		Send_char(char_unique); // On le renvoie pour savoir ce qu'on a tapé
		
		
		if (char_unique == '\r')
		{
			Send_char('\n'); // retour à la ligne
			
			Reset_buff_ptr(); // On initialise nos buffers à vide pour être certain de connaitre leur contenu
			
			// On commence à séparer le contenu de RX_Buf en sous buffers appropriés.
			separ_cmd(RX_ptr , ptr_CMD );
			
			if(separ_cmd(RX_ptr , ptr_PARAM_1)!=-1)
			{
				nb_cmd=0;
			}
			
			if(separ_cmd(RX_ptr , ptr_PARAM_2)!=-1)
			{
				nb_cmd=1;
			}
			
			if(separ_cmd(RX_ptr , ptr_PARAM_3)!=-1)
			{
				nb_cmd=2;
			}
			
			if(separ_cmd(RX_ptr , ptr_PARAM_4)!=-1)
			{
				nb_cmd=3;
			}
			else nb_cmd=4;
			

			
			
			analyse_cmd();
			
			
			switch(s_cmd)
			{     // Int or Enum type !
			
				
			case 0: 
				test[0] = 'N';
				test[1] = '\r';
				break;
			
			
			case 1:
				test[0] = 'D';
				test[1] = '\r';
				break;
			
			case 2:
				test[0] = 'E';
				test[1] = '\r';
				break;
			
			case 3:
				test[0] = 'Q';
				test[1] = '\r';
				break;
			
			case 4:
				test[0] = 'T';
				test[1] = 'V';
				test[2] = '\r';
				break;
			
			case 5:
				test[0] = 'A';
				test[1] = '\r';
				break;
			
			case 6:
				test[0] = 'B';
				test[1] = '\r';
				break;
			
			case 7:
				test[0] = 'S';
				test[1] = '\r';
				break;
			
			case 8:
				test[0] = 'R';
				test[1] = 'D';
				test[2] = '\r';
				break;
			
			case 9:
				test[0] = 'R';
				test[1] = 'G';
				test[2] = '\r';
				break;
			
			case 10:
				test[0] = 'R';
				test[1] = 'C';
				test[2] = '\r';
				break;
			
			case 11:
				test[0] = 'R';
				test[1] = 'A';
				test[2] = '\r';
				break;
			
			case 12:
				test[0] = 'G';
				test[1] = '\r';
				break;
			
			case 13:
				test[0] = 'A';
				test[1] = 'S';
				test[2] = 'S';
				test[3] = '\r';
				break;
			
			case 14:
				test[0] = 'M';
				test[1] = 'I';
				test[2] = '\r';
				break;
			
			case 15:
				test[0] = 'M';
				test[1] = 'E';
				test[2] = '\r';
				break;
			
			case 16:
				test[0] = 'I';
				test[1] = 'P';
				test[2] = 'O';
				test[3] = '\r';
				break;
			
			case 17:
				test[0] = 'P';
				test[1] = 'O';
				test[2] = 'S';
				test[3] = '\r';
				break;
			
			case 18:
				test[0] = 'M';
				test[1] = 'O';
				test[2] = 'U';
				test[3] = '\r';
				break;
			
			case 19:
				test[0] = 'M';
				test[1] = 'O';
				test[2] = 'B';
				test[3] = '\r';
				break;
			
			case 20:
				test[0] = 'M';
				test[1] = 'O';
				test[2] = 'S';
				test[3] = '\r';
				break;
			
			case 21:
				test[0] = 'S';
				test[1] = 'D';
				test[2] = '\r';
				break;
			
			case 22:
				test[0] = 'L';
				test[1] = '\r';
				break;
			
			case 23:
				test[0] = 'L';
				test[1] = 'S';
				test[2] = '\r';
				break;
			
			case 24:
				test[0] = 'C';
				test[1] = 'S';
				test[2] = '\r';
				break;
			
			case 25:
				test[0] = 'P';
				test[1] = 'P';
				test[2] = 'H';
				test[3] = '\r';
				break;
			
			case 26:
				test[0] = 'S';
				test[1] = 'P';
				test[2] = 'H';
				test[3] = '\r';
				break;
			
			case 27:
				test[0] = 'A';
				test[1] = 'U';
				test[2] = 'X';
				test[3] = '\r';
			
			default:
				
				test[0] = 'D';
				test[1] = '\r';
			
				break;
				
			}
			
			RX_ptr = &RX_Buf[0]; // On re place le ptr au début. Pour être capable de relancer un cycle.
		}
	}
	
	
//-----------------------------------------------------------------------------
// CM_HQ Sous-routine de retour d'Information
//-----------------------------------------------------------------------------
void CM_HQ(void)
	{
	}
//-----------------------------------------------------------------------------
// send_char et Send_string Sous-routines pour envoyer un char et string
//-----------------------------------------------------------------------------

void Send_char(char c)
{
	
	//Desactive reception
	
	REN0 = 0;
	SBUF0 = c;
	
	//Attente fin de transmission
	while(!TI0){}
		
	//Remise à 0 du flag d'envoi une fois qu'on est sur que le caractere a été transmis
	TI0 = 0;
	REN0 = 1;
}

void Send_string(char* mot)
{
	
	//Tant qu'on n'a pas fini de lire toute la chaine
		while (*mot != '\0'){
			
			//Check si on est en train de lire l'avant-dernier caractere (le tout dernier caractere est l'appuie de la touche ENTER)
			Send_char(*mot);
			
			mot++;
		}
		
		Send_char('\r');
		Send_char('\n'); //Retour à la ligne
}



//-----------------------------------------------------------------------------
// Recup_char sous-routine de récupération des char 
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


//-----------------------------------------------------------------------------
// Sous-routine d'analyse de la chaîne reçu
//-----------------------------------------------------------------------------

char separ_cmd(char* ptr_lecture, char* ptr_buffer){

// La fonction returne -1 si il n'y a rien ( ou plus rien à lire) dans le buffer de lecture donné.

	
    //On passe les espaces.
    while(*ptr_lecture == ' '){
        ptr_lecture++;
        RX_ptr++;
    }

    // cas chaine vide ou avec plus rien à lire
    if(*ptr_lecture == '\r' || *ptr_lecture == '\0'){
        return -1;
    }

    //On itère dans notre chaine de char
    while(*ptr_lecture != '\r' && *ptr_lecture != '\0' && *ptr_lecture != ' '){
        //Copie du char dans le buffer
        *ptr_buffer = *ptr_lecture;
        *(ptr_buffer+1) = '\0';
        ptr_lecture++;
        RX_ptr++;
        ptr_buffer++;
    }
		return 0;
  }

	
//-----------------------------------------------------------------------------
// Sous routine de Reset_buff_ptr
//-----------------------------------------------------------------------------

void Reset_buff_ptr(){
	
// On initialise nos buffers à vide pour être certain de connaitre leur contenu 
			RX_ptr = &RX_Buf[0]; // On re place le ptr au début.
			
			strcpy(CMD, "");
			ptr_CMD = &CMD[0];
			
			strcpy(PARAM_1, "");
			ptr_PARAM_1 = &PARAM_1[0];
			
			strcpy(PARAM_2, "");
			ptr_PARAM_2 = &PARAM_2[0];
			
			strcpy(PARAM_3, "");
			ptr_PARAM_3 = &PARAM_3[0];
			
			strcpy(PARAM_4, "");
			ptr_PARAM_4 = &PARAM_4[0];

  }
	
//-----------------------------------------------------------------------------
// analyse_cmd Sous-routine d'analyse de commande
//-----------------------------------------------------------------------------
void analyse_cmd(void)
	{
		if(strcmp(CMD, "D") == 0)
			{
			s_cmd = 1;
			}
	else if(strcmp(CMD, "E") == 0)
		{
			s_cmd = 2;
		}
	else if(strcmp(CMD, "Q") == 0)
	{
		s_cmd = 3;
	}
	else if(strcmp(CMD, "TV") == 0)
		{
		s_cmd = 4;
		}
	else if(strcmp(CMD, "A") == 0)
		{
		s_cmd = 5;
		}
	else if(strcmp(CMD, "B") == 0)
		{
		s_cmd = 6;
		}
	else if(strcmp(CMD, "S") == 0)
		{
		s_cmd = 7;
		}
	else if(strcmp(CMD, "RD") == 0)
		{
		s_cmd = 8;
		}
	else if(strcmp(CMD, "RG") == 0)
		{
		s_cmd = 9;
		}	
	else if(strcmp(CMD, "RC") == 0)
		{
		s_cmd = 10;
		}
	else if(strcmp(CMD, "RA") == 0)
		{
		s_cmd = 11;
	}
	else if(strcmp(CMD, "G") == 0)
		{
		s_cmd = 12;
	}
	else if(strcmp(CMD, "ASS") == 0)
		{
		s_cmd = 13;
	}
	else if(strcmp(CMD, "MI") == 0)
		{
		s_cmd = 14;
	}
	else if(strcmp(CMD, "ME") == 0)
		{
		s_cmd = 15;
	}
	else if(strcmp(CMD, "IPO") == 0)
		{
		s_cmd = 16;
	}
	else if(strcmp(CMD, "POS") == 0)
		{
		s_cmd = 17;
	}
	else if(strcmp(CMD, "MOU") == 0)
		{
		s_cmd = 18;
	}
	else if(strcmp(CMD, "MOB") == 0)
		{
		s_cmd = 19;
	}
	else if(strcmp(CMD, "MOS") == 0)
		{
		s_cmd = 20;
	}
	else if(strcmp(CMD, "SD") == 0)
		{
		s_cmd = 21;
	}
	else if(strcmp(CMD, "L") == 0)
		{
		s_cmd = 22;
	}
	else if(strcmp(CMD, "LS") == 0)
		{
		s_cmd = 23;
	}
	else if(strcmp(CMD, "CS") == 0)
		{
		s_cmd = 24;
	}
		else if(strcmp(CMD, "PPH") == 0)
		{
		s_cmd = 25;
	}
		else if(strcmp(CMD, "SPH") == 0)
		{
		s_cmd = 26;
	}
		else if(strcmp(CMD, "AUX") == 0)
		{
		s_cmd = 27;
	}

	// Cas commande inconnue
	else {commande_valide = 0;}
	
	//Affichage retour selon si la commande est connue ou non
	switch(commande_valide){
		case 0:
			//Si pas valide : accusé non reception :  CR + LF + #
			Send_char(0x0D);
			Send_char(0x0A);
			Send_char(0x23);
			break;
		
		//valide
		default: // Si valide : accusé reception :  CR + LF + >
			Send_char(0x0D);
			Send_char(0x0A);
			Send_char(0x3E);
			break;
	}
}
//-----------------------------------------------------------------------------
// Interrupt Handlers
//-----------------------------------------------------------------------------
