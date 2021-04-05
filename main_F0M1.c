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
#define RX_LENGTH    16
// length of UART RX buffer
sbit LED = P1^6; // LED = 1 means ON

struct COMMANDES commandes; //On declare une structure
//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------
void SYSCLK_Init (void);
void PORT_Init (void);
void UART0_Init (void);
void UART0_ISR (void);
void HQ_CM (void);
void CM_HQ (void);
void Send_char (char);
void Send_string(char*);
//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------
bit TX_Ready;
// ‘1’ means okay to TX
char *TX_ptr;
// pointer to string to transmit
bit RX_Ready;
// ‘1’ means RX string received
char idata RX_Buf[RX_LENGTH];
// receive string storage buffer

char idata CMD[RX_LENGTH]; // Buffer de sauvegarde de la commande
char idata PARAM[RX_LENGTH]; // Buffer de sauvegarde des param
char idata M_CMD[RX_LENGTH];
char idata test[RX_LENGTH];




char idata *p_test;
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
	// wait for TX ready
	while (1){
      while (RX_Ready == 0) ; // wait for string
      while (!TX_Ready) ; // wait for transmitter to be available
			HQ_CM();
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
		int cmd_param=0;
		int index = 0;
		int s_cmd = 2;
		char idata *p_RX;
		
		p_RX = RX_Buf;
		
		for(p_RX=RX_Buf;p_RX < RX_Buf + RX_LENGTH;p_RX++) // On itère à travers notre chaine de char
			{	
				switch(cmd_param)
					{
					case 0:
						CMD[index] = *p_RX; // On passe char par char à l'array CMD
						index++;
					
						if(*(p_RX + 1) == 0x20) // Si le char suivant est un espace on passe à la partie param
							{
								cmd_param = 1;
								//CMD[index]='\r';
								index = 0;
								p_RX++;
							}
							
						if(*(p_RX + 1) == 0x0D) // Si le char suivant est une fin de chaine on a terminé
							{
								cmd_param = 2;
								//CMD[index] = '\0';
							}
						break;	
					case 1:
						if(*p_RX != 0x20)
							{
								PARAM[index] = *p_RX;
								index++;
								
								if(*(p_RX + 1) == 0x0D) // Si le char suivant est un retour chariot on a terminé
								{
									//PARAM[index]='\0';
								}
							}
							break;
					}
				}
			
				
			
				
			M_CMD[0] = 'D'; // Cmd Début d'épreuve
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=1;
				}
				
			
				
			M_CMD[0] = 'E'; // Cmd Fin d'épreuve
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=2;
				}
			
				
				
			M_CMD[0] = 'Q'; // Cmd Arrêt d'urgence
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=3;
				}
			

				
			M_CMD[0] = 'A'; // Cmd Avancer
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=4;
				}	
				
				
				
			M_CMD[0] = 'B'; // Cmd Reculer
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=5;
				}

				
				
			M_CMD[0] = 'S'; // Cmd Stop (pour fin A et B)
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=6;
				}
				
				

			M_CMD[0] = 'T'; // Cmd Réglage Vitesse
			M_CMD[1] = 'V';	
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=7;
				}
				
				
			
			M_CMD[0] = 'R'; // Cmd Rotation Droite
			M_CMD[1] = 'D';	
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=8;
				}	
				
			

			M_CMD[0] = 'R'; // Cmd Rotation Gauche
			M_CMD[1] = 'G';	
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=9;
				}	

				
				
			M_CMD[0] = 'R'; // Cmd Rotation complète(sens horaire / anti-horaire)
			M_CMD[1] = 'C';	
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=10;
				}
			
				
				
			M_CMD[0] = 'R'; // Cmd Rotation partielle (sens horaire / anti-horaire)
			M_CMD[1] = 'A';	
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=11;
				}
				
			
			
			M_CMD[0] = 'G'; // Cmd Déplacement par coord
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=12;
				}
				
				
			M_CMD[0] = 'A'; // Cmd Acquisition son 
			M_CMD[1] = 'S';	
			M_CMD[2] = 'S';
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=13;
				}	
				
				
			
			M_CMD[0] = 'M'; // Cmd Mesure Courant // CMD PAS IMPLENTEE
			M_CMD[1] = 'I';	
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=14;
				}



			M_CMD[0] = 'M'; // Cmd Mesure Courant // CMD PAS IMPLENTEE
			M_CMD[1] = 'E';	
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=15;
				}	


			
			M_CMD[0] = 'I'; // Cmd initialisation pos robot coord
			M_CMD[1] = 'P';	
			M_CMD[2] = 'O';
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=16;
				}	


				
			M_CMD[0] = 'P'; // Cmd Position base roulante 
			M_CMD[1] = 'O';	
			M_CMD[2] = 'S';
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=17;
				}
				
			
				
				
				
			M_CMD[0] = 'M'; // Cmd Detection d'obstacle unique 
			M_CMD[1] = 'O';	
			M_CMD[2] = 'U';
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=18;
				}
				
				
				
			M_CMD[0] = 'M'; // Cmd Detection d'obstacle par balayage 
			M_CMD[1] = 'O';	
			M_CMD[2] = 'B';
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=19;
				}
				
				
				
			M_CMD[0] = 'M'; // Cmd Detection d'obstacle le plus proche par balayage 
			M_CMD[1] = 'O';	
			M_CMD[2] = 'S';
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=20;
				}	
				
				
				
			M_CMD[0] = 'S'; // Cmd Génération de signaux sonores 
			M_CMD[1] = 'D';	
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=21;
				}
				
				
				
			M_CMD[0] = 'L'; // Cmd Allumage pointeur lumineux
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=22;
				}	
				
				
				
			M_CMD[0] = 'L'; // Cmd Extinction pointeur lumineux
			M_CMD[1] = 'S';	
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=23;
				}
				
				
				
			M_CMD[0] = 'C'; // Cmd Pilotage servomoteur
			M_CMD[1] = 'S';	
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=24;
				}
				
				
			M_CMD[0] = 'P'; // Cmd Prise de photographie
			M_CMD[1] = 'P';	
			M_CMD[2] = 'H';
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=25;
				}	
				
				
			
			M_CMD[0] = 'S'; // Cmd Arrêt prise de photographie
			M_CMD[1] = 'P';	
			M_CMD[2] = 'H';
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=26;
				}			



			M_CMD[0] = 'A'; // Cmd Auxiliaires
			M_CMD[1] = 'U';	
			M_CMD[2] = 'X';
			if(strcmp(CMD,M_CMD)==0)
				{
					s_cmd=27;
				}				
				
	
				
				
				
			
				
				
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
		}
	
	
//-----------------------------------------------------------------------------
// CM_HQ Sous-routine de retour d'Information
//-----------------------------------------------------------------------------
void CM_HQ(void)
	{
	}
//-----------------------------------------------------------------------------
// send_char et Send_char Sous-routine pour envoyer un char et string
//-----------------------------------------------------------------------------
//void send_char (char* message)
//	{
//		char temp[24];
//		sprintf(temp, "%s\r\n", message);
//		TX_Ready = 0; // claim transmitter
//    TX_ptr = temp; // pointe vers le message à transmettre
//		TI0 = 1; // start transmit
//	}
	
	
	
	
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
// Interrupt Handlers
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// UART0_ISR
//-----------------------------------------------------------------------------
//
// Interrupt Service Routine for UART0:
// Transmit function is implemented as a NULL-terminated string transmitter
// that uses the global variable <TX_ptr> and the global semaphore <TX_Ready>.
// Example usage:
//while (TX_Ready == 0);
// wait for transmitter to be available
//    TX_Ready = 0; 
// claim transmitter
//    TX_ptr = <pointer to string to transmit>;
//    TI0 = 1;
// initiate transmit
//
// Receive function is implemented as a CR-terminated string receiver
// that uses the global buffer <RX_Buf> and global indicator <RX_Ready>.
// Once the message is received, <RX_Ready> is set to ‘1’.  Characters
// received while <RX_Ready> is ‘1’ are ignored.
//




// ON NE PASSE PLUS PAR UN INTERRUPT  


//void UART0_ISR (void) interrupt 4 using 3
//	{
//	static unsigned char RX_index = 0;
//  // receive buffer index   
//	unsigned char the_char;
//	if (RI0 == 1) // Si on reçoit un message
//		{ // handle receive function 
//			RI0 = 0; // clear RX complete indicator 
//			if (RX_Ready != 1) 
//				{ // check to see if message pending 
//					the_char = SBUF0;
//					if (the_char != '\r') 
//						{ // On check la fin du message  
//				
//							RX_Buf[RX_index] = the_char; // store the character  
//					
//				// increment buffer pointer and wrap if necessary  
//							if (RX_index < (RX_LENGTH - 2)) 
//								{
//									RX_index++;
//								}
//							else 
//								{
//									RX_index = 0;
//						// if length exceeded, 
//									RX_Ready = 1;
//							// post message complete, and  
//							// <CR>-terminate string
//									RX_Buf[RX_index-1] = '\r';
//								}
//					}
//						else
//							{
//								RX_Buf[RX_index] = '\r';   
//				// <CR>-terminate message 
//								RX_Ready = 1; 
//				// post message ready
//								RX_index = 0;
//				// reset RX message index
//							}
//						
//					}
//				else
//					{
//					// ignore character -- previous message has not been processed  
//					}
//				}
//		else if (TI0 == 1) // Si on envoie un message 
//			{   
//						// handle transmit function 
//				TI0 = 0;
//						// clear TX complete indicator 
//				the_char = *TX_ptr; 
//						// read next character in string
//				if (the_char != '\r')
//					{ 
//						SBUF0 = the_char;  
//							// transmit it  
//						TX_ptr++; 
//							// get ready for next character 
//					}
//					else
//						{
//								// character is <CR>
//							TX_Ready = 1;
//								// indicate ready for next TX 
//						}  
//			}
//	}