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
#include <stdlib.h>

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

struct COMMANDES commandes; //On declare une structure commande
struct INFORMATIONS informations; //On declare une structure information
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
void decoup_clef_val(char*, char*, char*);

void Reset_buff_ptr(void);
void Struct_init(void);

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



char buf_clef[15];
char buf_val[15];

char test[5];

int nb_cmd;
int s_cmd;
int commande_valide;


//-----------------------------------------------------------------------------
// MAIN Routine
//-----------------------------------------------------------------------------
void main (void) {

	WDTCN = 0xde; // disable watchdog timer
	WDTCN = 0xad;
	
	SYSCLK_Init(); // initialize oscillator
	PORT_Init (); // initialize crossbar and GPIO   
	UART0_Init (); // initialize UART0   
	
	Struct_init(); //Init des valeurs de la struct
	
	Send_string("Tapez une cmd ");

	while (1){
      if(RI0 == 1)
				{
					HQ_CM();
				}
			CM_HQ();	
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
// UART0_Init  Configure the UART0 using Timer1, for <baudrate> and 8-N-1.
//-----------------------------------------------------------------------------
void UART0_Init (void){
			SCON0  = 0x50; // SCON0: mode 1, 8-bit UART, enable RX
			TMOD   = 0x20; // TMOD: timer 1, mode 2, 8-bit reload 
			TH1    = -(SYSCLK/BAUDRATE/16); // set Timer1 reload value for baudrate
			TR1    = 1; // start Timer1
			CKCON |= 0x10; // Timer1 uses SYSCLK as time base
			PCON  |= 0x80; // SMOD00 = 1
			//ES0    = 1; // enable UART0 interrupts
			//TX_Ready = 1; // indicate TX ready for transmit 
			//RX_Ready = 0; // indicate RX string not ready
			//TX_ptr = NULL;
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
				Send_string("Cas 0");
				break;
			
			
			case 1:
				Send_string("D");
				if(nb_cmd == 0)
					{
						commandes.Etat_Epreuve = 1;
					}
				else //Un argument : le n° de l'epreuve
					{
						if(0<atoi(PARAM_1) && 9>atoi(PARAM_1))
							{
								commandes.Etat_Epreuve = atoi(PARAM_1);	
							}
		//Sinon commande invalide
				else
						{
							commande_valide = 0;
						}
					}
				break;
			
			case 2:
				Send_string("E");
			commandes.Etat_Epreuve = 9;
				break;
			
			case 3:
				Send_string("Q");
			commandes.Etat_Epreuve = 10;
				break;
			
			case 4:
				Send_string("TV");
				if(nb_cmd == 1)
					{
						if(5>atoi(PARAM_1) && 101>atoi(PARAM_1))
							{
								commandes.V_defaut = atoi(PARAM_1);
							}
					}
				else
						{
							commande_valide = 0;
						}
				break;
			
			case 5:
				Send_string("A");
				if(nb_cmd == 0)
					{
						commandes.Vitesse = commandes.V_defaut;
						commandes.Etat_Mouvement = Avancer;
						
					}
		else
		{
			if(5>atoi(PARAM_1) && 101>atoi(PARAM_1))
			{
				commandes.Vitesse = atoi(PARAM_1);
				commandes.Etat_Mouvement = Avancer;
			}
			else
			{
				commande_valide = 0;
			}
		}
				break;
			
			case 6:
				Send_string("B");
				if(nb_cmd == 0)
		{
			commandes.Vitesse = commandes.V_defaut;
			commandes.Etat_Mouvement = Reculer;
		}
		else
		{
			if(5>atoi(PARAM_1) && 101>atoi(PARAM_1))
			{
				commandes.Vitesse = atoi(PARAM_1);
				commandes.Etat_Mouvement = Reculer;
			}
			else
			{
				commande_valide = 0;
			}
		}
				break;
			
			case 7:
				Send_string("S");
				commandes.Etat_Mouvement = Stopper;
				break;
			
			case 8:
				Send_string("RD");
				commandes.Etat_Mouvement = Rot_90D;
				break;
			
			case 9:
				Send_string("RG");
				commandes.Etat_Mouvement = Rot_90G;
				
				break;
			
			case 10:
				Send_string("RC");
				if(nb_cmd == 0)
					{
						commande_valide = 0;
					}
	
					else
						{
							if(strcmp(PARAM_1, "D") == 0)
								{
									commandes.Etat_Mouvement = Rot_180D;

								}
							else if(strcmp(PARAM_1, "G") == 0)
									{
										commandes.Etat_Mouvement = Rot_180G;
									}
							else{
										commande_valide = 0;
									}
						}
		
				break;
			
			case 11:
				Send_string("RA");
			
				if(nb_cmd == 0)
					{
						commandes.Etat_Mouvement = Rot_AngD;
						commandes.Angle = 90;
					}
	
	
				else
					{	
						decoup_clef_val(PARAM_1, buf_clef, buf_val);
			//Si droite et ecrit correctement
						if(strcmp(buf_clef, "D") == 0 &&
							atoi(buf_val) > 0 &&
							atoi(buf_val) < 181)
						{
							commandes.Etat_Mouvement = Rot_AngD;
							commandes.Angle = atoi(buf_val);
			}
			else if(strcmp(buf_clef, "G") == 0 && atoi(buf_val) > 0 && atoi(buf_val) < 181)
				{
					commandes.Etat_Mouvement = Rot_AngG;
					commandes.Angle = atoi(buf_val);
				}
			else{
				commande_valide = 0;
			}
		}
				break;
			
			case 12:
				Send_string("G");
			
				if(nb_cmd != 3)
					{
						commande_valide = 0;
					}
				else 
					{
					
						decoup_clef_val(PARAM_1, buf_clef, buf_val);

						if((strcmp(buf_clef, "X") == 0) && ((-100 < atoi(buf_val)) && (100 > atoi(buf_val))))
							{
								commandes.Etat_Mouvement = Depl_Coord;
								commandes.Coord_X = atoi(buf_val);
							}

						else if((strcmp(buf_clef, "Y") == 0) && ((-100 < atoi(buf_val)) && (100 > atoi(buf_val))) && commande_valide != 0)
							{
								commandes.Coord_Y = atoi(buf_val);
							}

						else if((strcmp(buf_clef, "A") == 0) && ((-181 < atoi(buf_val)) && (181 > atoi(buf_val))) && commande_valide != 0)
							{
								commandes.Angle = atoi(buf_val);
							}

						else
							{
								commande_valide = 0;
							}
		
	
						decoup_clef_val(PARAM_2, buf_clef, buf_val);

						if((strcmp(buf_clef, "Y") == 0) && ((-100 < atoi(buf_val)) && (100 > atoi(buf_val))))
							{
								commandes.Coord_X = atoi(buf_val);
							}

						else if((strcmp(buf_clef, "X") == 0) && ((-100 < atoi(buf_val)) && (100 > atoi(buf_val))) && commande_valide != 0)
							{
								commandes.Coord_Y = atoi(buf_val);
							}

						else if((strcmp(buf_clef, "A") == 0) && ((-181 < atoi(buf_val)) && (181 > atoi(buf_val))) && commande_valide != 0)
							{
								commandes.Angle = atoi(buf_val);
							}

						else
							{
								commande_valide = 0;
							}
		
						
						decoup_clef_val(PARAM_3, buf_clef, buf_val);

						if((strcmp(buf_clef, "A") == 0) && ((-181 < atoi(buf_val)) && (181 > atoi(buf_val))) && commande_valide != 0)
						{
							commandes.Angle = atoi(buf_val);
						}

						else if((strcmp(buf_clef, "Y") == 0) && ((-100 < atoi(buf_val)) && (100 > atoi(buf_val))))
							{
								commandes.Coord_X = atoi(buf_val);
							}

						else if((strcmp(buf_clef, "X") == 0) && ((-100 < atoi(buf_val)) && (100 > atoi(buf_val))) && commande_valide != 0)
							{
								commandes.Coord_Y = atoi(buf_val);
							}

						else
							{
								commande_valide = 0;
							}
		//Si un des arguments est invalide, on remet tout a 0
						if(commande_valide == 0)
							{
								commandes.Etat_Mouvement = Mouvement_non;
								commandes.Coord_X = 0;
								commandes.Coord_Y = 0;
								commandes.Angle = 0;
							}
					}
				break;
			
			case 13:
				Send_string("ASS");
				if(nb_cmd == 0)
					{
						commande_valide = 0;
					}
	
				if(atoi(PARAM_1) > 0 && atoi(PARAM_1) < 99)
					{
						commandes.ACQ_Duree = atoi(PARAM_1);
						commandes.Etat_ACQ_Son = ACQ_oui;
					}
				else
					{
						commande_valide = 0;
					}
				break;
			
			case 14: // Mesure conso courant pas encore implémenté
				Send_string("MI");
				commandes.Etat_Energie = Mesure_I;
				break;
			
			case 15: // Mesure conso energie pas encore implémenté
				Send_string("ME");
				commandes.Etat_Energie = Mesure_E;
				break;
			
			case 16:
				Send_string("IPO");
			
				if(nb_cmd != 3)
					{
						commande_valide = 0;
					}
				else 
					{
						decoup_clef_val(PARAM_1, buf_clef, buf_val);
						if((strcmp(buf_clef, "X") == 0) && ((-100 < atoi(buf_val)) && (100 > atoi(buf_val))))
							{
								commandes.Etat_Position = Init_Position;
								commandes.Pos_Coord_X = atoi(buf_val);
							}

        else if((strcmp(buf_clef, "Y") == 0) && ((-100 < atoi(buf_val)) && (100 > atoi(buf_val))) && commande_valide != 0)
				{
					commandes.Pos_Coord_Y = atoi(buf_val);
				}

        else if((strcmp(buf_clef, "A") == 0) && ((-181 < atoi(buf_val)) && (181 > atoi(buf_val))) && commande_valide != 0)
				{
					commandes.Pos_Angle = atoi(buf_val);
				}

				else
					{
						commande_valide = 0;
					}
		
		//Deuxieme argument si le premier est fait
		decoup_clef_val(PARAM_2, buf_clef, buf_val);

		if((strcmp(buf_clef, "Y") == 0) && ((-100 < atoi(buf_val)) && (100 > atoi(buf_val))))
			{
				commandes.Pos_Coord_X = atoi(buf_val);
			}

        else if((strcmp(buf_clef, "X") == 0) && ((-100 < atoi(buf_val)) && (100 > atoi(buf_val))) && commande_valide != 0)
				{
					commandes.Pos_Coord_Y = atoi(buf_val);
				}

        else if((strcmp(buf_clef, "A") == 0) && ((-181 < atoi(buf_val)) && (181 > atoi(buf_val))) && commande_valide != 0)
				{
					commandes.Pos_Angle = atoi(buf_val);
				}

		else
			{
				commande_valide = 0;
			}
		
		//Troisieme argument
		decoup_clef_val(PARAM_3, buf_clef, buf_val);

		if((strcmp(buf_clef, "A") == 0) && ((-181 < atoi(buf_val)) && (181 > atoi(buf_val))) && commande_valide != 0)
			{
				commandes.Pos_Angle = atoi(buf_val);
			}

    else if((strcmp(buf_clef, "Y") == 0) && ((-100 < atoi(buf_val)) && (100 > atoi(buf_val))))
			{
				commandes.Pos_Coord_X = atoi(buf_val);
			}

    else if((strcmp(buf_clef, "X") == 0) && ((-100 < atoi(buf_val)) && (100 > atoi(buf_val))) && commande_valide != 0)
		{
			commandes.Pos_Coord_Y = atoi(buf_val);
		}

		else
			{
				commande_valide = 0;
      }
    }
    
    if(commande_valide == 0)
			{
        commandes.Etat_Mouvement = Mouvement_non;
        commandes.Pos_Coord_X = 0;
        commandes.Pos_Coord_Y = 0;
        commandes.Pos_Angle = 0;
			}
			
				break;
			
			case 17:
				Send_string("POS");
				commandes.Etat_Position = Demande_Position;
				break;
			
			case 18:
				test[0] = 'M';
				test[1] = 'O';
				test[2] = 'U';
				test[3] = '\r';
				break;
			
			case 19:
				Send_string("MOB");
			
				switch (nb_cmd)
					{
					
						case 0:
							commandes.Etat_DCT_Obst = oui_360;
							commandes.DCT_Obst_Resolution = 30;
							break;

				
						case 1:
						
							if(strcmp(PARAM_1, "D") == 0)
								{
									commandes.Etat_DCT_Obst = oui_180;
								}
 
							else
								{
									decoup_clef_val(PARAM_1, buf_clef, buf_val);
									if(strcmp(buf_clef, "A") == 0 && atoi(buf_val) > 4 && atoi(buf_val) < 46)
										{
											commandes.DCT_Obst_Resolution = atoi(buf_val);
										}
								}
							break;

   
						case 2:
							if(strcmp(PARAM_1, "D") == 0)
								{
									commandes.Etat_DCT_Obst = oui_180;
								}
 

							
									decoup_clef_val(PARAM_2, buf_clef, buf_val);
									if(strcmp(buf_clef, "A") == 0 && atoi(buf_val) > 4 && atoi(buf_val) < 46)
										{
											commandes.DCT_Obst_Resolution = atoi(buf_val);
										}
												
       
									else
										{
											commande_valide = 0;
										}
							break;
 
						default:
							commande_valide = 0;
							break;
					}
				break;
			
			case 20: // FONCTION NON PROGRAMMEE POUR LE MOMENT  
				test[0] = 'M';
				test[1] = 'O';
				test[2] = 'S';
				test[3] = '\r';
				break;
			
			case 21: // FONCTION NON PROGRAMMEE POUR LE MOMENT
				test[0] = 'S';
				test[1] = 'D';
				test[2] = '\r';
				break;
			
			case 22:
				Send_string("L");
			
				commandes.Etat_Lumiere = Allumer;
				commandes.Lumiere_Intensite = 100;
				commandes.Lumiere_Duree = 99;
				commandes.Lumire_Extinction = 0;
				commandes.Lumiere_Nbre = 1;

				if(nb_cmd != 0)
					{
						decoup_clef_val(PARAM_1, buf_clef, buf_val);
						if(strcmp(buf_clef, "I") == 0 && 0 < atoi(buf_val) && 101 > atoi(buf_val))
							{
								commandes.Lumiere_Intensite = atoi(buf_val);
							}
						else if(strcmp(buf_clef, "D") == 0 && 0 < atoi(buf_val) && 100 > atoi(buf_val))
						{
                commandes.Lumiere_Duree = atoi(buf_val);
						}
						else if(strcmp(buf_clef, "E") == 0 && 0 <= atoi(buf_val) && 100 > atoi(buf_val))
						{
                commandes.Lumire_Extinction = atoi(buf_val);
						}
						else if (strcmp(buf_clef, "N") == 0 &&
            0 < atoi(buf_val) &&
            100 > atoi(buf_val)){
                commandes.Lumiere_Nbre = atoi(buf_val);
        }
        else{
            commande_valide = 0;
        }

        decoup_clef_val(PARAM_2, buf_clef, buf_val);
        if(strcmp(buf_clef, "I") == 0 &&
            0 < atoi(buf_val) &&
            101 > atoi(buf_val)){
                commandes.Lumiere_Intensite = atoi(buf_val);
        }
        else if(strcmp(buf_clef, "D") == 0 &&
            0 < atoi(buf_val) &&
            100 > atoi(buf_val)){
                commandes.Lumiere_Duree = atoi(buf_val);
        }
        else if(strcmp(buf_clef, "E") == 0 &&
            0 <= atoi(buf_val) &&
            100 > atoi(buf_val)){
                commandes.Lumire_Extinction = atoi(buf_val);
        }
        else if (strcmp(buf_clef, "N") == 0 &&
            0 < atoi(buf_val) &&
            100 > atoi(buf_val)){
                commandes.Lumiere_Nbre = atoi(buf_val);
        }
        else{
            commande_valide = 0;
        }

        decoup_clef_val(PARAM_3, buf_clef, buf_val);
        if(strcmp(buf_clef, "I") == 0 &&
            0 < atoi(buf_val) &&
            101 > atoi(buf_val)){
                commandes.Lumiere_Intensite = atoi(buf_val);
        }
        else if(strcmp(buf_clef, "D") == 0 &&
            0 < atoi(buf_val) &&
            100 > atoi(buf_val)){
                commandes.Lumiere_Duree = atoi(buf_val);
        }
        else if(strcmp(buf_clef, "E") == 0 &&
            0 <= atoi(buf_val) &&
            100 > atoi(buf_val)){
                commandes.Lumire_Extinction = atoi(buf_val);
        }
        else if (strcmp(buf_clef, "N") == 0 &&
            0 < atoi(buf_val) &&
            100 > atoi(buf_val)){
                commandes.Lumiere_Nbre = atoi(buf_val);
        }
        else{
            commande_valide = 0;
        }

        decoup_clef_val(PARAM_4, buf_clef, buf_val);
        if(strcmp(buf_clef, "I") == 0 &&
            0 < atoi(buf_val) &&
            101 > atoi(buf_val)){
                commandes.Lumiere_Intensite = atoi(buf_val);
        }
        else if(strcmp(buf_clef, "D") == 0 &&
            0 < atoi(buf_val) &&
            100 > atoi(buf_val)){
                commandes.Lumiere_Duree = atoi(buf_val);
        }
        else if(strcmp(buf_clef, "E") == 0 &&
            0 <= atoi(buf_val) &&
            100 > atoi(buf_val)){
                commandes.Lumire_Extinction = atoi(buf_val);
        }
        else if (strcmp(buf_clef, "N") == 0 &&
            0 < atoi(buf_val) &&
            100 > atoi(buf_val)){
                commandes.Lumiere_Nbre = atoi(buf_val);
        }
        else{
            commande_valide = 0;
        }
    }

    if(commande_valide == 0){
        commandes.Etat_Lumiere = Lumiere_non;
    }
				
				break;
			
			case 23:
				Send_string("LS");
				commandes.Etat_Lumiere = Eteindre;
				break;
			
			case 24:
				Send_string("CS");
			
				switch(nb_cmd)
			{
        case 0:
            commandes.Etat_Servo = Servo_H;
            commandes.Servo_Angle = 0;
            break;
        case 1:
            
                decoup_clef_val(PARAM_1, buf_clef, buf_val);
                if(strcmp(buf_clef, "A") == 0 && -100 < atoi(buf_val) && 100 > atoi(buf_val))
								{
									commandes.Servo_Angle = atoi(buf_val);
                }
                else
									{
                    commande_valide = 0;
									}
            
								if (strcmp(PARAM_1, "H") == 0)
									{
										commandes.Etat_Servo = Servo_H;
									}
            else if (strcmp(PARAM_1, "V") == 0)
							{
								commandes.Etat_Servo = Servo_V;
							}
            else
							{
								commande_valide = 0;
							}
            break;

        case 2:
            
                decoup_clef_val(PARAM_2, buf_clef, buf_val);
                if(strcmp(buf_clef, "A") == 0 && -100 < atoi(buf_val) && 100 > atoi(buf_val))
									{
										commandes.Servo_Angle = atoi(buf_val);
									}
                else
									{
                    commande_valide = 0;
									}
            
            if (strcmp(PARAM_1, "H") == 0)
							{
								commandes.Etat_Servo = Servo_H;
							}
            else if (strcmp(PARAM_1, "V") == 0)
							{
								commandes.Etat_Servo = Servo_V;
							}
            else
							{
								commande_valide = 0;
							}
            break;

            default:
                commande_valide = 0;
                break;
    }

    if(commande_valide == 0)
			{
        commandes.Etat_Servo = Servo_non;
        commandes.Servo_Angle = 0;
			}
				break;
			
			case 25:
				Send_string("PPH");
			
				commandes.Etat_Photo = Photo_non;
				commandes.Photo_Duree = 1;
				commandes.Photo_Nbre = 1;

					if(strcmp(PARAM_1, "O") == 0)
						{
							commandes.Etat_Photo = Photo_1;
						}

					else if(strcmp(PARAM_1, "C") == 0)
						{
							commandes.Etat_Photo = Photo_continue;
						}

					else if(strcmp(PARAM_1, "S") == 0)
						{
							commandes.Etat_Photo = Photo_Multiple;
						}

					else
						{
							commande_valide = 0;
						}
					decoup_clef_val(PARAM_2, buf_clef, buf_val);
					if(strcmp(buf_clef, "E") == 0 && 0 <= atoi(buf_val) && 100 > atoi(buf_val))
								{
									commandes.Photo_Duree = atoi(buf_val);
								}
					else
						{
							commande_valide = 0;
						}

			
					decoup_clef_val(PARAM_3, buf_clef, buf_val);
					if(strcmp(buf_clef, "N") == 0 && 0 < atoi(buf_val) && 256 > atoi(buf_val))
					{
                commandes.Photo_Nbre = atoi(buf_val);
					}
					else
						{
							commande_valide = 0;
						}
				
	

    if(commande_valide == 0)
			{
        commandes.Etat_Photo = Photo_non;
        commandes.Photo_Duree = 1;
        commandes.Photo_Nbre = 1;
			}
				
				break;
			
			case 26:
				Send_string("SPH");
				commandes.Etat_Photo = Photo_stop;
				break;
			
			case 27:
				test[0] = 'A';
				test[1] = 'U';
				test[2] = 'X';
				test[3] = '\r';
			
			default:
				Send_char(0x0D);
				Send_char(0x0A);
				Send_char(0x23);
				break;
				
			}
			Reset_buff_ptr();
			RX_ptr = &RX_Buf[0]; // On re place le ptr au début. Pour être capable de relancer un cycle.
		}
	}
	
	
//-----------------------------------------------------------------------------
// CM_HQ Sous-routine de retour d'Information
//-----------------------------------------------------------------------------
void CM_HQ(void)
	{
		if(informations.Etat_Invite == Invite_oui){
        Send_string(informations.MSG_Invit);
    }
    //
    if(informations.Etat_BUT_Mouvement == BUT_Atteint_oui){
        Send_string("Coordonnees atteintes");
    }
    //
    if(informations.Etat_BUT_Servo == BUT_Servo_H){
        Send_string("Servo H en place");
    }
    else if(informations.Etat_BUT_Servo == BUT_Servo_V){
        Send_string("Servo V en place");
    }
    //
    if(informations.Etat_DCT_Obst == BUT_Atteint_oui){
        Send_string("Coordonnees atteintes");
    }
    //
    if(informations.Etat_RESULT_Courant == BUT_Atteint_oui){
        Send_string("Coordonnees atteintes");
    }
    //

    if(informations.Etat_RESULT_Energie == BUT_Atteint_oui){
        Send_string("Coordonnees atteintes");
    }
    //
    if(informations.Etat_RESULT_Position == BUT_Atteint_oui){
        Send_string("Coordonnees atteintes");
    }
    //
    if(informations.Etat_Aux == Aux_oui){
        Send_string(informations.MSG_Aux);
    }
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
// Sous-routine  decoup_clef_val de découpage clef valeur des param complexes.
//-----------------------------------------------------------------------------
void decoup_clef_val(char* param, char* buf_clef, char* buf_val)
{
	if(*param == '\0'){
        *buf_clef = '\0';
        *buf_val = '\0';
    }
    else{
        //On prend tous les caracteres jusqu'au ':' exclus dans clef
        while(*param != ':')
        {
                *buf_clef = *param;
                buf_clef++;
        param++;
        }
        //Ici on passe les ':'
        param++;
    
        //Puis on lit tout ce qui suit les ':' jusqu'a la fin de la chaine
        while(*param != '\0')
        {
            *buf_val = *param;
            buf_val++;
            param++;
        }
    }
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
// analyse_cmd Sous-routine d'analyse de commande
//-----------------------------------------------------------------------------
void analyse_cmd(void)
	{
		commande_valide = 1;
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
		else 
			{
				s_cmd = 28;
				commande_valide = 0;
			}
	// Cas commande inconnue
	//else {commande_valide = 0;}
	
	//Affichage retour selon si la commande est connue ou non
	switch(commande_valide){
		case 0:
			
			//Si pas valide : accusé non reception :  CR + LF + #
		
			//Send_char(0x0D);
			//Send_char(0x0A);
			//Send_char(0x23);
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
// Struct_init Sous-routine d'analyse de commande
//-----------------------------------------------------------------------------
void Struct_init(void)
{
	commandes.Etat_Epreuve = Epreuve_non;
	commandes.Etat_Mouvement = Mouvement_non;
	
	commandes.Etat_ACQ_Son = ACQ_non;
	commandes.Etat_DCT_Obst = DCT_non;
	commandes.Etat_Lumiere = Lumiere_non;
	commandes.Etat_Servo = Servo_non;
	commandes.Etat_Energie = Energie_non;
	commandes.Etat_Position = Position_non;
	commandes.Etat_Photo = Photo_non;
	
	commandes.V_defaut = 20;
	
	
}
//-----------------------------------------------------------------------------
// Interrupt Handlers
//-----------------------------------------------------------------------------
