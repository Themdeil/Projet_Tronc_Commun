//-----------------------------------------------------------------------------
// F0-M01.c
//
//Gestion des trames de commandes et d'informations
//Réception sur l'UART des commandes et stockage dans un tableau binaire
//Lecture dans un tableau des informations et envoie par l'UART
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//	Pour les fonctions selon la commande : faire un return avec 
//	si oui ou non la commande est valide

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#include <C8051F020.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c8051F020_SFR16.h"
#include "MASTER_Config_Globale.h"
#include "FO_M1__Structures_COMMANDES_INFORMATIONS_CentraleDeCommande.h"

// ========== Prototypes de Fonctions ==========
	//Config
void Config_UART0(void);
void Config_interrupt(void);
void Config_Timer(void);

	//Communication via UART
void F0_M1_lecture_commande(void);
void Send_char(char c);
void Send_string(char*);
char Get_char(void);

	//Traitement des commandes
void Buffer_commande(char);

void get_Clef_Valeur(char* mot, char* bufferClef, char* bufferVal);

void Transfert_commande(void);
void Reinitialise_buffers(void);

//Cas des commandes
char commande_valide;
void Commande_D(void);
void Commande_E(void);
void Commande_Q(void);

void Commande_A(void);
void Commande_B(void);
void Commande_S(void);
void Commande_RD(void);
void Commande_RG(void);
void Commande_RC(void);
void Commande_RA(void);
void Commande_G(void);
void Commande_TV(void);

void Commande_ASS(void);

void Commande_MOB(void);

void Commande_L(void);
void Commande_LS(void);

void Commande_CS(void);

void Commande_MI(void);
void Commande_ME(void);

void Commande_IPO(void);
void Commande_POS(void);

void Commande_PPH(void);
void Commande_SPH(void);
	//Interrupt

// ========== Variables utiles ==========
char bit_reception_UART;
	
struct COMMANDES STRUCT_COMMANDE
= { //Valeurs par defaut
	Epreuve_non,
	Mouvement_non,20, 0, 0, 0,20,
	ACQ_non, 0,
	DCT_non, 0, 
	Lumiere_non, 0, 0, 0, 0,
	Servo_non, 0,
	Energie_non,
	Position_non, 0, 0, 0,
	Photo_non, 0, 0
};


//Contient la clef/valeur des arguments complexes
char buffer_Clef[15];
char buffer_Valeur[15];

char UART_COMMANDE[60] = "";
char* ptr_UART = &UART_COMMANDE[0];
char COMMANDE[10] = "";
char* ptr_COMMANDE = &COMMANDE[0];
char ARGUMENT_1[10] = "";
char* ptr_ARGUMENT_1 = &ARGUMENT_1[0];
char ARGUMENT_2[10] = "";
char* ptr_ARGUMENT_2 = &ARGUMENT_2[0];
char ARGUMENT_3[10] = "";
char* ptr_ARGUMENT_3 = &ARGUMENT_3[0];
char ARGUMENT_4[10] = "";
char* ptr_ARGUMENT_4 = &ARGUMENT_4[0];
char NOMBRE_ARGUMENTS = 0;

char EXTRACTION_COMMANDE();
char EXTRACTION(char* ptr_lecture, char* ptr_buffer);
char isComplexe(char* ARG);


// ===================== ENVOI INFORMATION ===================

void F0_M1_envoi_information(struct INFORMATIONS STRUCT_INFORMATIONS);
char MSG_CENTRALE[50];
char CONCAT_MSG_CENTRALE[50];








//-----------------------------------------------------------------------------
// ==================== MAIN Routine ====================
//-----------------------------------------------------------------------------

void main (void) {
	// Appel des configurations globales
	Init_Device();  
	Config_Timer();
	Config_UART0();
	
	//Init variables
	Send_string("SYSTEME OK !");

	while (1){
		F0_M1_lecture_commande();
	}
}



void F0_M1_envoi_information(struct INFORMATIONS STRUCT_INFORMATIONS){
    //
    if(STRUCT_INFORMATIONS.Etat_Invite == Invite_oui){
        Send_string(STRUCT_INFORMATIONS.MSG_Invit);
    }
    //
    if(STRUCT_INFORMATIONS.Etat_BUT_Mouvement == BUT_Atteint_oui){
        Send_string("Coordonnees atteintes");
    }
    //
    if(STRUCT_INFORMATIONS.Etat_BUT_Servo == BUT_Servo_H){
        Send_string("Servo H en place");
    }
    else if(STRUCT_INFORMATIONS.Etat_BUT_Servo == BUT_Servo_V){
        Send_string("Servo V en place");
    }
    //
    if(STRUCT_INFORMATIONS.Etat_DCT_Obst == BUT_Atteint_oui){
        Send_string("Coordonnees atteintes");
    }
    //
    if(STRUCT_INFORMATIONS.Etat_RESULT_Courant == BUT_Atteint_oui){
        Send_string("Coordonnees atteintes");
    }
    //
    if(STRUCT_INFORMATIONS.Etat_RESULT_Energie == BUT_Atteint_oui){
        Send_string("Coordonnees atteintes");
    }
    //
    if(STRUCT_INFORMATIONS.Etat_RESULT_Position == BUT_Atteint_oui){
        Send_string("Coordonnees atteintes");
    }
    //
    if(STRUCT_INFORMATIONS.Etat_Aux == Aux_oui){
        Send_string(MSG_Aux);
    }
}





char EXTRACTION(char* ptr_lecture, char* ptr_buffer){
//0 : pas de suite
//-1 : vide
//1 : non vide et une suite
    //Cas ou il y a des espaces avant la chaine
    while(*ptr_lecture == ' '){
        ptr_lecture++;
        ptr_UART++;
    }

    //chaine vide
    if(*ptr_lecture == '\r' || *ptr_lecture == '\0'){
        return -1;
    }

    //Tant qu'on n'est pas a la fin de la commchaineande
    while(*ptr_lecture != '\r' && *ptr_lecture != '\0' && *ptr_lecture != ' '){
        //On ecrit le caractere de le buffer
        *ptr_buffer = *ptr_lecture;
        *(ptr_buffer+1) = '\0';
        ptr_lecture++;
        ptr_UART++;
        ptr_buffer++;
    }

    //Fin de chaine sans suite
    if(*ptr_lecture == '\r' || *ptr_lecture == '\0'){
        return 0;
    }
    
    //Il y a une suite (espace)
    return 1;
}

char EXTRACTION_COMMANDE(){
    char continuer;
    strcpy(ARGUMENT_1, "");
    ptr_ARGUMENT_1 = &ARGUMENT_1[0];
    strcpy(ARGUMENT_2, "");
    ptr_ARGUMENT_2 = &ARGUMENT_2[0];
    strcpy(ARGUMENT_3, "");
    ptr_ARGUMENT_3 = &ARGUMENT_3[0];
    strcpy(COMMANDE, "");
    ptr_COMMANDE = &COMMANDE[0];
    ptr_UART = &UART_COMMANDE[0];

    continuer = EXTRACTION(ptr_UART, COMMANDE);
    if (continuer != 1){
        NOMBRE_ARGUMENTS = 0;
        return continuer;
    }
    continuer = EXTRACTION(ptr_UART, ARGUMENT_1);
    if (continuer != 1){
        NOMBRE_ARGUMENTS = 1;
        return continuer;
    }

    continuer = EXTRACTION(ptr_UART, ARGUMENT_2);
    if (continuer != 1){
        NOMBRE_ARGUMENTS = 2;
        return continuer;
    }

    continuer = EXTRACTION(ptr_UART, ARGUMENT_3);
    if (continuer != 1){
        NOMBRE_ARGUMENTS = 3;
        return continuer;
    }

    continuer = EXTRACTION(ptr_UART, ARGUMENT_4);
    if (continuer != 1){
        NOMBRE_ARGUMENTS = 4;
        return continuer;
    }

    return 0;
}

char isComplexe(char* ARG){
    //Check s'il y a 'clef' puis ':' puis 'valeur'
    //1 : pas complexe
    //0 : complexe

    //pas de clef
    if(*ARG == ':' || *ARG == '\0'){
        return 1;
    }

    //Tant qu'on regarde la clef
    while(*ARG != ':' || *ARG != '\0'){
        ARG++;
    }

    //Si on regarde ':', on commence a lire la valeur
    if(*ARG == ':'){
        ARG++;
    }

    //Pas de ':' ou valeur vide
    if(*ARG == '\0'){
        return 1;
    }

    return 0;
}

//-----------------------------------------------------------------------------
//  Fonctions de configuration des divers périphériques et interruptions
//-----------------------------------------------------------------------------
void Config_interrupt()
	{
}

void Config_UART0(void)
	{
	//SMOD0 dans PCON.7 à 0 pour garder la baud rate/2
	//SM00 SM10 : 01 (mode 1)
	//SM20 : 0 (valeur du bit de STOP ignorée)
	//REN0 : 0 (reception desactivée)
	//TB80 RB80 : 00 (valeur du bit lors de la Transmission/Reception dans le mode 2 ou 3)
	//TI0 RI0 : 00 (flag lors d'une fin de transmission/reception)
	SCON0 = (1<<6);
}

void Config_Timer()
	{
	
	//TIMER 2 (POUR UART)
	
	//TF2 EXF2 = 00 (flags interrupt)
	//RCLK0 TCLK0 : 11 (mode 2 baud rate generator receive et transmit)
	//EXEN2 : 0 (T2EX ignored)
	//TR2 : 0 (TIMER2 disabled)
	//C/T2 : 0 (SYSCLK used)
	//CP/RL2 : 0 (ignored in mode 2)
	RCAP2 = 0xFFDC; //Baud-rate de 19200
	T2CON = (3<<4);
	TR2 = 1; //start timer
}

//-----------------------------------------------------------------------------
// Fonctions utiles
//-----------------------------------------------------------------------------
void Reinitialise_buffers(void){
	// ==========================================
	//
	// But : Remettre toutes les variables dans l'etat initial
	// Pour retrouver la situation d'origine (comme au demarrage)
	//
	// ===========================================
    ptr_UART = &UART_COMMANDE[0];
    ptr_COMMANDE = &COMMANDE[0];
    ptr_ARGUMENT_1 = &ARGUMENT_1[0];
    ptr_ARGUMENT_2 = &ARGUMENT_2[0];
    ptr_ARGUMENT_3 = &ARGUMENT_3[0];
    NOMBRE_ARGUMENTS = 0;
}

void F0_M1_lecture_commande()
	// ==========================================
	//	Fonction principale de lecture des commandes
	// A Utiliser dans M6 avec une interruption (surement)
	// But : Lire la commande, et la stocker si elle est bonne dans
	// 			la structure de donnees prevues STRUCT_COMMANDE
	//
	// ===========================================
	{
			if (RI0 == 1){
			
			//Recup du caractere
			bit_reception_UART = Get_char();
			
			//Stockage de la chaine correspondant a la commande + arguments
			Buffer_commande(bit_reception_UART);
				
			//Envoi du caractere entré pour avoir une lisibilité 
				//sur la commande que l'utilisateur ecrit
			//A voir si c'est utile puisqu'on ecrit sur python et pas putty
			Send_char(bit_reception_UART);
			
			//On check si on a fini la commande (si on appuie sur la touche ENTER ou pas)
			if (bit_reception_UART == '\r'){
				//Retour à la ligne esthetique
				Send_char('\n');
			
            //mise dans les buffers de la commande et des arguments
            EXTRACTION_COMMANDE();

			//Traitement de la commande
				Transfert_commande();
				
			//Reinitialise les variables
				Reinitialise_buffers();
			}
		}
}

void get_Clef_Valeur(char* mot, char* bufferClef, char* bufferVal)
{

    if(*mot == '\0'){
        *bufferClef = '\0';
        *bufferVal = '\0';
    }
    else{
        //On prend tous les caracteres jusqu'au ':' exclus dans clef
        while(*mot != ':')
        {
                *bufferClef = *mot;
                bufferClef++;
        mot++;
        }
        //Ici on passe les ':'
        mot++;
    
        //Puis on lit tout ce qui suit les ':' jusqu'a la fin de la chaine
        while(*mot != '\0')
        {
            *bufferVal = *mot;
            bufferVal++;
            mot++;
        }
    }
}

char Get_char(void)
{
		
	char retour;

	//Desactive la reception
	RI0 = 0;
	REN0 = 0;
	
	//Recup le caractere
	retour = SBUF0;
	
	//Reactive la reception
	REN0 = 1;
	
	return retour;
}

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
	// ==========================================
	//Envoi la chaine de caractere et fait un retour a la ligne
	// ==========================================
{

	
	//Tant qu'on n'a pas fini de lire toute la chaine
		while (*mot != '\0'){
			Send_char(*mot);
			mot++;
		}
		Send_char('\r');//Retour chariot
		Send_char('\n'); //Retour à la ligne
}

void Buffer_commande(char c)
// ===========================================
// Enregistre le caractere dans le buffer de commande
// Traite le cas de fin suppression de caractere (touche SUPPR)
// ===========================================
{
	if(c == 0x08)
	{
		*ptr_UART = '\0';
		ptr_UART--;
	}
	else
	{
	*ptr_UART = c;
	*(ptr_UART + 1) = '\0';
	ptr_UART++;
	}
}

void Transfert_commande(void)
// ===========================================
//
// Reconnaissance de la commande entree et appelle la
//fonction correspondante selon le cas
// Traite le cas d'un code commande inexistant
// 
// Renvoie > dans le terminal si la commande est validée
// et # si commande invalide (code commande ou argument)	
//
// ===========================================

{
	//On part du principe que la commande est bonne
	//On change le flag si on est dans une situation ou la commande n'est pas valide
	commande_valide = 1;
	
	//Tous les cas possibles
	if(strcmp(COMMANDE, "D") == 0){Commande_D();}
	else if(strcmp(COMMANDE, "E") == 0){Commande_E();}
	else if(strcmp(COMMANDE, "Q") == 0){Commande_Q();}
	else if(strcmp(COMMANDE, "A") == 0){Commande_A();}
	else if(strcmp(COMMANDE, "B") == 0){Commande_B();}
	else if(strcmp(COMMANDE, "S") == 0){Commande_S();}
	else if(strcmp(COMMANDE, "RD") == 0){Commande_RD();}
	else if(strcmp(COMMANDE, "RG") == 0){Commande_RG();}
	else if(strcmp(COMMANDE, "RC") == 0){Commande_RC();}
	else if(strcmp(COMMANDE, "RA") == 0){Commande_RA();}
	else if(strcmp(COMMANDE, "G") == 0){Commande_G();}
	else if(strcmp(COMMANDE, "TV") == 0){Commande_TV();}
	else if(strcmp(COMMANDE, "ASS") == 0){Commande_ASS();}
	else if(strcmp(COMMANDE, "MOB") == 0){Commande_MOB();}
	else if(strcmp(COMMANDE, "L") == 0){Commande_L();}
	else if(strcmp(COMMANDE, "LS") == 0){Commande_LS();}
	else if(strcmp(COMMANDE, "CS") == 0){Commande_CS();}
	else if(strcmp(COMMANDE, "MI") == 0){Commande_MI();}
	else if(strcmp(COMMANDE, "ME") == 0){Commande_ME();}
	else if(strcmp(COMMANDE, "IPO") == 0){Commande_IPO();}
	else if(strcmp(COMMANDE, "POS") == 0){Commande_POS();}
	else if(strcmp(COMMANDE, "PPH") == 0){Commande_PPH();}
	else if(strcmp(COMMANDE, "SPH") == 0){Commande_SPH();}

	// Cas commande inconnue
	else {commande_valide = 0;}
	
	//Affichage retour selon si la commande est connue ou non
	switch(commande_valide){
		case 0:
			//Pas valide
			Send_char(0x0D);
			Send_char(0x0A);
			Send_char(0x23);
			break;
		
		//valide
		default:
			Send_char(0x0D);
			Send_char(0x0A);
			Send_char(0x3E);
			break;
	}
}

//-----------------------------------------------------------------------------
// Fonctions selon la commande
//-----------------------------------------------------------------------------

//====================== ETAT EPREUVE ======================
void Commande_D(void)
{
	//Pas d'argument
	if(NOMBRE_ARGUMENTS == 0)
	{
		STRUCT_COMMANDE.Etat_Epreuve = 1;
	}
	else //Un argument : le n° de l'epreuve
	{
		if(0<atoi(ARGUMENT_1) && 9>atoi(ARGUMENT_1))
		{
			STRUCT_COMMANDE.Etat_Epreuve = atoi(ARGUMENT_1);	
		}
		//Sinon commande invalide
		else
		{
			commande_valide = 0;
		}
	}
}
void Commande_E(void)
{
	STRUCT_COMMANDE.Etat_Epreuve = 9;	
}
void Commande_Q(void)
{
	STRUCT_COMMANDE.Etat_Epreuve = 10;	
}

//====================== ETAT MOUVEMENT ======================

void Commande_A(void)
{
		//Pas d'argument, vitesse par defaut (par TV)
	if(NOMBRE_ARGUMENTS == 0)
	{
		STRUCT_COMMANDE.Vitesse = STRUCT_COMMANDE.Vitesse_TV;
		STRUCT_COMMANDE.Etat_Mouvement = Avancer;
	}
	else
	{
		if(5>atoi(ARGUMENT_1) && 101>atoi(ARGUMENT_1))
		{
			STRUCT_COMMANDE.Vitesse = atoi(ARGUMENT_1);
			STRUCT_COMMANDE.Etat_Mouvement = Avancer;
		}
		else
		{
			commande_valide = 0;
		}
	}
}
void Commande_B(void)
{
	//Pas d'argument, vitesse par defaut (par TV)
	if(NOMBRE_ARGUMENTS == 0)
	{
		STRUCT_COMMANDE.Vitesse = STRUCT_COMMANDE.Vitesse_TV;
		STRUCT_COMMANDE.Etat_Mouvement = Reculer;
	}
	else
	{
		if(5>atoi(ARGUMENT_1) && 101>atoi(ARGUMENT_1))
		{
			STRUCT_COMMANDE.Vitesse = atoi(ARGUMENT_1);
			STRUCT_COMMANDE.Etat_Mouvement = Reculer;
		}
		else
		{
			commande_valide = 0;
		}
	}
}
void Commande_S(void)
{
	STRUCT_COMMANDE.Etat_Mouvement = Stopper;
}
void Commande_RD(void)
{
	STRUCT_COMMANDE.Etat_Mouvement = Rot_90D;
}
void Commande_RG(void)
{
	STRUCT_COMMANDE.Etat_Mouvement = Rot_90G;
}
void Commande_RC(void)
{
	//Pas d'arg
	if(NOMBRE_ARGUMENTS == 0)
	{
		commande_valide = 0;
	}
	//Un arg
	else
	{
		if(strcmp(ARGUMENT_1, "D") == 0)
		{
			STRUCT_COMMANDE.Etat_Mouvement = Rot_180D;

		}
		else if(strcmp(ARGUMENT_1, "G") == 0)
		{
			STRUCT_COMMANDE.Etat_Mouvement = Rot_180G;
		}
		else{
			commande_valide = 0;
		}
	}
}
void Commande_RA(void)
{
	//Si pas d'arg, valeur par defaut
	if(NOMBRE_ARGUMENTS == 0)
	{
		STRUCT_COMMANDE.Etat_Mouvement = Rot_AngD;
		STRUCT_COMMANDE.Angle = 90;
	}
	
	
	else
	{
		if(isComplexe(ARGUMENT_1) != 0 )
		{
			commande_valide = 0;
		}
		//Argument valide
		else
		{
			get_Clef_Valeur(ARGUMENT_1, buffer_Clef, buffer_Valeur);
			//Si droite et ecrit correctement
			if(strcmp(buffer_Clef, "D") == 0 &&
					atoi(buffer_Valeur) > 0 &&
					atoi(buffer_Valeur) < 181)
				{
					STRUCT_COMMANDE.Etat_Mouvement = Rot_AngD;
					STRUCT_COMMANDE.Angle = atoi(buffer_Valeur);
			}
				//Si gauche et ecrit correctement
			else if(strcmp(buffer_Clef, "G") == 0 &&
					atoi(buffer_Valeur) > 0 &&
					atoi(buffer_Valeur) < 181)
				{
					STRUCT_COMMANDE.Etat_Mouvement = Rot_AngG;
					STRUCT_COMMANDE.Angle = atoi(buffer_Valeur);
			}
				//Sinon probleme dans la syntaxe
			else{
				commande_valide = 0;
			}
		}
	}
}
void Commande_G(void)
{
	//S'il y a trop ou pas assez d'argument
	if(NOMBRE_ARGUMENTS != 3)
		{
			commande_valide = 0;
		}
        //Si l'un des arguments n'est pas sous la forme d'arg complexe
	else if(isComplexe(ARGUMENT_1) != 0 ||
            isComplexe(ARGUMENT_2) != 0 ||
            isComplexe(ARGUMENT_3) != 0){
            commande_valide = 0;
        }
	else {
		//Sinon il faut verifier que tous les arguments sont valides
		//On va check les arguments un a un et faire le changement au fur et a mesure
		//Si un argument est invalide on rechange tout pour "pas de commande"

		//Premier argument
		get_Clef_Valeur(ARGUMENT_1, buffer_Clef, buffer_Valeur);

		if((strcmp(buffer_Clef, "X") == 0) &&
		((-100 < atoi(buffer_Valeur)) && (100 > atoi(buffer_Valeur)))){
			STRUCT_COMMANDES.Etat_Mouvement = Depl_Coord;
			STRUCT_COMMANDES.Coord_X = atoi(buffer_Valeur);
		}

        else if((strcmp(buffer_Clef, "Y") == 0) &&
		((-100 < atoi(buffer_Valeur)) && (100 > atoi(buffer_Valeur))) &&
		commande_valide != 0){
			STRUCT_COMMANDES.Coord_Y = atoi(buffer_Valeur);
		}

        else if((strcmp(buffer_Clef, "A") == 0) &&
		((-181 < atoi(buffer_Valeur)) && (181 > atoi(buffer_Valeur))) &&
		commande_valide != 0){
			STRUCT_COMMANDES.Angle = atoi(buffer_Valeur);
		}

		else{
				commande_valide = 0;
		}
		
		//Deuxieme argument si le premier est fait
		get_Clef_Valeur(ARGUMENT_2, buffer_Clef, buffer_Valeur);

		if((strcmp(buffer_Clef, "Y") == 0) &&
		((-100 < atoi(buffer_Valeur)) && (100 > atoi(buffer_Valeur)))){
			STRUCT_COMMANDES.Coord_X = atoi(buffer_Valeur);
		}

        else if((strcmp(buffer_Clef, "X") == 0) &&
		((-100 < atoi(buffer_Valeur)) && (100 > atoi(buffer_Valeur))) &&
		commande_valide != 0){
			STRUCT_COMMANDES.Coord_Y = atoi(buffer_Valeur);
		}

        else if((strcmp(buffer_Clef, "A") == 0) &&
		((-181 < atoi(buffer_Valeur)) && (181 > atoi(buffer_Valeur))) &&
		commande_valide != 0){
			STRUCT_COMMANDES.Angle = atoi(buffer_Valeur);
		}

		else{
			commande_valide = 0;
		}
		
		//Troisieme argument
		get_Clef_Valeur(ARGUMENT_3, buffer_Clef, buffer_Valeur);

		if((strcmp(buffer_Clef, "A") == 0) &&
		((-181 < atoi(buffer_Valeur)) && (181 > atoi(buffer_Valeur))) &&
		commande_valide != 0){
			STRUCT_COMMANDES.Angle = atoi(buffer_Valeur);
		}

        else if((strcmp(buffer_Clef, "Y") == 0) &&
		((-100 < atoi(buffer_Valeur)) && (100 > atoi(buffer_Valeur)))){
			STRUCT_COMMANDES.Coord_X = atoi(buffer_Valeur);
		}

        else if((strcmp(buffer_Clef, "X") == 0) &&
		((-100 < atoi(buffer_Valeur)) && (100 > atoi(buffer_Valeur))) &&
		commande_valide != 0){
			STRUCT_COMMANDES.Coord_Y = atoi(buffer_Valeur);
		}

		else{
			commande_valide = 0;
        }
		//Si un des arguments est invalide, on remet tout a 0
		if(commande_valide == 0){
			STRUCT_COMMANDES.Etat_Mouvement = Mouvement_non;
			STRUCT_COMMANDES.Coord_X = 0;
			STRUCT_COMMANDES.Coord_Y = 0;
			STRUCT_COMMANDES.Angle = 0;
		}
	}
}
void Commande_TV(void)
{
	//Si on a un argument c'est bon
	if(NOMBRE_ARGUMENTS != 0)
	{
		//Verifie la validite de la valeur de vitesse
		if(5>atoi(ARGUMENT_1) && 101>atoi(ARGUMENT_1))
		{
			STRUCT_COMMANDE.Vitesse = atoi(ARGUMENT_1);
			STRUCT_COMMANDE.Vitesse_TV = atoi(ARGUMENT_1);
		}
		else
		{
			commande_valide = 0;
		}
	}
	
	else
	{
		commande_valide = 0;
	}
}

//====================== ETAT ACQ SON ======================
	
void Commande_ASS(void)
{
	//Pas d'argument
	if(NOMBRE_ARGUMENTS == 0)
	{
		commande_valide = 0;
	}
	
	//Arg doit etre en 1 et 99 inclus
	if(atoi(ARGUMENT_1) > 0 && atoi(ARGUMENT_1) < 99){
		STRUCT_COMMANDE.ACQ_Duree = atoi(ARGUMENT_1);
		STRUCT_COMMANDE.Etat_ACQ_Son = ACQ_oui;
	}
	else{
		commande_valide = 0;
	}
	
}

//====================== ETAT DTC OBS ======================

void Commande_MOB(void)
{
    switch (NOMBRE_ARGUMENTS)
    {
    //Valeurs par defaut
    case 0:
        STRUCT_COMMANDE.Etat_DCT_Obst = oui_360;
		STRUCT_COMMANDE.DCT_Obst_Resolution = 30;
        break;

    //Un arg, a voir si c'est D ou la resolution angulaire
    case 1:
        //Cas ou c'est D
        if(strcmp(ARGUMENT_1, "D") == 0){
		    STRUCT_COMMANDE.Etat_DCT_Obst = oui_180;
        }
        //Cas ou c'est la resolution angulaire, mais il faut que 
        //l'arg soit valide
        else if(isComplexe(ARGUMENT_1) == 0){
            get_Clef_Valeur(ARGUMENT_1, buffer_Clef, buffer_Valeur);
            if(strcmp(buffer_Clef, "A") == 0 &&
                atoi(buffer_Valeur) > 4 &&
                atoi(buffer_Valeur) < 46){
                STRUCT_COMMANDE.DCT_Obst_Resolution = atoi(buffer_Valeur);
            }
        }
        //Sinon l'arg est invalide et la commande idem
        else{
            commande_valide = 0;
        }
        break;

    //Deux arguments
    case 2:
        if(strcmp(ARGUMENT_1, "D") == 0){
		    STRUCT_COMMANDE.Etat_DCT_Obst = oui_180;
        }
 
        //arg valide ?
        if(isComplexe(ARGUMENT_2) == 0){
            get_Clef_Valeur(ARGUMENT_2, buffer_Clef, buffer_Valeur);
            if(strcmp(buffer_Clef, "A") == 0 &&
                atoi(buffer_Valeur) > 4 &&
                atoi(buffer_Valeur) < 46){
                STRUCT_COMMANDE.DCT_Obst_Resolution = atoi(buffer_Valeur);
            }
        }
        //Sinon l'arg est invalide et la commande idem
        else{
            commande_valide = 0;
        }
        break;
    
    //Eventuellement si on met plus de 2 arguments
    default:
        commande_valide = 0;
        break;
    }
}

//====================== ETAT LUMIERE ======================

void Commande_L(void)
{
    //Valeurs par defaut
    STRUCT_COMMANDE.Etat_Lumiere = Allumer;
    STRUCT_COMMANDE.Lumiere_Intensite = 100;
    STRUCT_COMMANDE.Lumiere_Duree = 99;
    STRUCT_COMMANDE.Lumire_Extinction = 0;
    STRUCT_COMMANDE.Lumiere_Nbre = 1;

    if(NOMBRE_ARGUMENTS != 0){
        get_Clef_Valeur(ARGUMENT_1, buffer_Clef, buffer_Valeur);
        if(strcmp(buffer_Clef, "I") == 0 &&
            0 < atoi(buffer_Valeur) &&
            101 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumiere_Intensite = atoi(buffer_Valeur);
        }
        else if(strcmp(buffer_Clef, "D") == 0 &&
            0 < atoi(buffer_Valeur) &&
            100 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumiere_Duree = atoi(buffer_Valeur);
        }
        else if(strcmp(buffer_Clef, "E") == 0 &&
            0 <= atoi(buffer_Valeur) &&
            100 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumire_Extinction = atoi(buffer_Valeur);
        }
        else if (strcmp(buffer_Clef, "N") == 0 &&
            0 < atoi(buffer_Valeur) &&
            100 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumiere_Nbre = atoi(buffer_Valeur);
        }
        else{
            commande_valide = 0;
        }

        get_Clef_Valeur(ARGUMENT_2, buffer_Clef, buffer_Valeur);
        if(strcmp(buffer_Clef, "I") == 0 &&
            0 < atoi(buffer_Valeur) &&
            101 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumiere_Intensite = atoi(buffer_Valeur);
        }
        else if(strcmp(buffer_Clef, "D") == 0 &&
            0 < atoi(buffer_Valeur) &&
            100 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumiere_Duree = atoi(buffer_Valeur);
        }
        else if(strcmp(buffer_Clef, "E") == 0 &&
            0 <= atoi(buffer_Valeur) &&
            100 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumire_Extinction = atoi(buffer_Valeur);
        }
        else if (strcmp(buffer_Clef, "N") == 0 &&
            0 < atoi(buffer_Valeur) &&
            100 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumiere_Nbre = atoi(buffer_Valeur);
        }
        else{
            commande_valide = 0;
        }

        get_Clef_Valeur(ARGUMENT_3, buffer_Clef, buffer_Valeur);
        if(strcmp(buffer_Clef, "I") == 0 &&
            0 < atoi(buffer_Valeur) &&
            101 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumiere_Intensite = atoi(buffer_Valeur);
        }
        else if(strcmp(buffer_Clef, "D") == 0 &&
            0 < atoi(buffer_Valeur) &&
            100 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumiere_Duree = atoi(buffer_Valeur);
        }
        else if(strcmp(buffer_Clef, "E") == 0 &&
            0 <= atoi(buffer_Valeur) &&
            100 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumire_Extinction = atoi(buffer_Valeur);
        }
        else if (strcmp(buffer_Clef, "N") == 0 &&
            0 < atoi(buffer_Valeur) &&
            100 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumiere_Nbre = atoi(buffer_Valeur);
        }
        else{
            commande_valide = 0;
        }

        get_Clef_Valeur(ARGUMENT_4, buffer_Clef, buffer_Valeur);
        if(strcmp(buffer_Clef, "I") == 0 &&
            0 < atoi(buffer_Valeur) &&
            101 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumiere_Intensite = atoi(buffer_Valeur);
        }
        else if(strcmp(buffer_Clef, "D") == 0 &&
            0 < atoi(buffer_Valeur) &&
            100 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumiere_Duree = atoi(buffer_Valeur);
        }
        else if(strcmp(buffer_Clef, "E") == 0 &&
            0 <= atoi(buffer_Valeur) &&
            100 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumire_Extinction = atoi(buffer_Valeur);
        }
        else if (strcmp(buffer_Clef, "N") == 0 &&
            0 < atoi(buffer_Valeur) &&
            100 > atoi(buffer_Valeur)){
                STRUCT_COMMANDE.Lumiere_Nbre = atoi(buffer_Valeur);
        }
        else{
            commande_valide = 0;
        }
    }

    if(commande_valide == 0){
        STRUCT_COMMANDE.Etat_Lumiere = Lumiere_non;
    }

}
void Commande_LS(void)
{
    STRUCT_COMMANDE.Etat_Lumiere = Eteindre;
}

//====================== ETAT SERVO ======================
	
void Commande_CS(void)
{
    switch(NOMBRE_ARGUMENTS){
        case 0:
            STRUCT_COMMANDE.Etat_Servo = Servo_H;
            STRUCT_COMMANDE.Servo_Angle = 0;
            break;
        case 1:
            if(isComplexe(ARGUMENT_1) == 0){
                get_Clef_Valeur(ARGUMENT_1, buffer_Clef, buffer_Valeur);
                if(strcmp(buffer_Clef, "A") == 0 &&
                    -100 < atoi(buffer_Valeur) &&
                    100 > atoi(buffer_Valeur)){
                        STRUCT_COMMANDE.Servo_Angle = atoi(buffer_Valeur);
                }
                else{
                    commande_valide = 0;
                }
            }
            else if (strcmp(ARGUMENT_1, "H") == 0){
                STRUCT_COMMANDE.Etat_Servo = Servo_H;
            }
            else if (strcmp(ARGUMENT_1, "V") == 0){
                STRUCT_COMMANDE.Etat_Servo = Servo_V;
            }
            else{
                commande_valide = 0;
            }
            break;

        case 2:
            if(isComplexe(ARGUMENT_2) == 0){
                get_Clef_Valeur(ARGUMENT_2, buffer_Clef, buffer_Valeur);
                if(strcmp(buffer_Clef, "A") == 0 &&
                    -100 < atoi(buffer_Valeur) &&
                    100 > atoi(buffer_Valeur)){
                        STRUCT_COMMANDE.Servo_Angle = atoi(buffer_Valeur);
                }
                else{
                    commande_valide = 0;
                }
            }
            else{
                commande_valide = 0;
            }
            if (strcmp(ARGUMENT_1, "H") == 0){
                STRUCT_COMMANDE.Etat_Servo = Servo_H;
            }
            else if (strcmp(ARGUMENT_1, "V") == 0){
                STRUCT_COMMANDE.Etat_Servo = Servo_V;
            }
            else{
                commande_valide = 0;
            }
            break;

            default:
                commande_valide = 0;
                break;
    }

    if(commande_valide == 0){
        STRUCT_COMMANDE.Etat_Servo = Servo_non;
        STRUCT_COMMANDE.Servo_Angle = 0;
    }
}
	
//====================== ETAT ENERGIE ======================
	
void Commande_MI(void)
{
	STRUCT_COMMANDE.Etat_Energie = Mesure_I;
}
void Commande_ME(void)
{
	STRUCT_COMMANDE.Etat_Energie = Mesure_E;
}

//====================== ETAT POSITION ======================

void Commande_IPO(void)
{
	//S'il y a trop ou pas assez d'argument
	if(NOMBRE_ARGUMENTS != 3)
		{
			commande_valide = 0;
		}
        //Si l'un des arguments n'est pas sous la forme d'arg complexe
	else if(isComplexe(ARGUMENT_1) != 0 ||
            isComplexe(ARGUMENT_2) != 0 ||
            isComplexe(ARGUMENT_3) != 0){
            commande_valide = 0;
        }
	else {
		//Sinon il faut verifier que tous les arguments sont valides
		//On va check les arguments un a un et faire le changement au fur et a mesure
		//Si un argument est invalide on rechange tout pour "pas de commande"

		//Premier argument
		get_Clef_Valeur(ARGUMENT_1, buffer_Clef, buffer_Valeur);

		if((strcmp(buffer_Clef, "X") == 0) &&
		((-100 < atoi(buffer_Valeur)) && (100 > atoi(buffer_Valeur)))){
			STRUCT_COMMANDES.Etat_Position = Init_Position;
			STRUCT_COMMANDES.Pos_Coord_X = atoi(buffer_Valeur);
		}

        else if((strcmp(buffer_Clef, "Y") == 0) &&
		((-100 < atoi(buffer_Valeur)) && (100 > atoi(buffer_Valeur))) &&
		commande_valide != 0){
			STRUCT_COMMANDES.Pos_Coord_Y = atoi(buffer_Valeur);
		}

        else if((strcmp(buffer_Clef, "A") == 0) &&
		((-181 < atoi(buffer_Valeur)) && (181 > atoi(buffer_Valeur))) &&
		commande_valide != 0){
			STRUCT_COMMANDES.Pos_Angle = atoi(buffer_Valeur);
		}

		else{
				commande_valide = 0;
		}
		
		//Deuxieme argument si le premier est fait
		get_Clef_Valeur(ARGUMENT_2, buffer_Clef, buffer_Valeur);

		if((strcmp(buffer_Clef, "Y") == 0) &&
		((-100 < atoi(buffer_Valeur)) && (100 > atoi(buffer_Valeur)))){
			STRUCT_COMMANDES.Pos_Coord_X = atoi(buffer_Valeur);
		}

        else if((strcmp(buffer_Clef, "X") == 0) &&
		((-100 < atoi(buffer_Valeur)) && (100 > atoi(buffer_Valeur))) &&
		commande_valide != 0){
			STRUCT_COMMANDES.Pos_Coord_Y = atoi(buffer_Valeur);
		}

        else if((strcmp(buffer_Clef, "A") == 0) &&
		((-181 < atoi(buffer_Valeur)) && (181 > atoi(buffer_Valeur))) &&
		commande_valide != 0){
			STRUCT_COMMANDES.Pos_Angle = atoi(buffer_Valeur);
		}

		else{
			commande_valide = 0;
		}
		
		//Troisieme argument
		get_Clef_Valeur(ARGUMENT_3, buffer_Clef, buffer_Valeur);

		if((strcmp(buffer_Clef, "A") == 0) &&
		((-181 < atoi(buffer_Valeur)) && (181 > atoi(buffer_Valeur))) &&
		commande_valide != 0){
			STRUCT_COMMANDES.Pos_Angle = atoi(buffer_Valeur);
		}

        else if((strcmp(buffer_Clef, "Y") == 0) &&
		((-100 < atoi(buffer_Valeur)) && (100 > atoi(buffer_Valeur)))){
			STRUCT_COMMANDES.Pos_Coord_X = atoi(buffer_Valeur);
		}

        else if((strcmp(buffer_Clef, "X") == 0) &&
		((-100 < atoi(buffer_Valeur)) && (100 > atoi(buffer_Valeur))) &&
		commande_valide != 0){
			STRUCT_COMMANDES.Pos_Coord_Y = atoi(buffer_Valeur);
		}

		else{
			commande_valide = 0;
        }
    }
    //Si un des arguments est invalide, on remet tout a 0
    if(commande_valide == 0){
        STRUCT_COMMANDES.Etat_Mouvement = Mouvement_non;
        STRUCT_COMMANDES.Pos_Coord_X = 0;
        STRUCT_COMMANDES.Pos_Coord_Y = 0;
        STRUCT_COMMANDES.Pos_Angle = 0;
    }
}
void Commande_POS(void)
{
	STRUCT_COMMANDES.Etat_Position = Demande_Position;
}

//====================== ETAT PHOTO ======================

void Commande_PPH(void)
{
    //Par defaut
    STRUCT_COMMANDES.Etat_Photo = Photo_non;
    STRUCT_COMMANDES.Photo_Duree = 1;
    STRUCT_COMMANDES.Photo_Nbre = 1;

    //Check le type de photo
    if(strcmp(ARGUMENT_1, "O") == 0){
        STRUCT_COMMANDES.Etat_Photo = Photo_1;
    }

    else if(strcmp(ARGUMENT_1, "C") == 0){
        STRUCT_COMMANDES.Etat_Photo = Photo_continue;
    }

    else if(strcmp(ARGUMENT_1, "S") == 0){
        STRUCT_COMMANDES.Etat_Photo = Photo_Multiple;
    }

    else{
        commande_valide = 0
    }

    //E:Duree
    if (isComplexe(ARGUMENT_2) == 0){
        get_Clef_Valeur(ARGUMENT_2, buffer_Clef, buffer_Valeur);
        if(strcmp(buffer_Clef, "E") == 0 &&
            0 <= atoi(buffer_Valeur) &&
            100 > atoi(buffer_Valeur)){
                STRUCT_COMMANDES.Photo_Duree = atoi(buffer_Valeur);
            }
        else{
            commande_valide = 0
        }
    }
    else{
        commande_valide = 0
    }

    //N:nombre
    if (isComplexe(ARGUMENT_3) == 0){
        get_Clef_Valeur(ARGUMENT_3, buffer_Clef, buffer_Valeur);
        if(strcmp(buffer_Clef, "N") == 0 &&
            0 < atoi(buffer_Valeur) &&
            256 > atoi(buffer_Valeur)){
                STRUCT_COMMANDES.Photo_Nbre = atoi(buffer_Valeur);
        }
        else{
        commande_valide = 0
        }
    }
    else{
        commande_valide = 0
    }

    if(commande_valide == 0){
        STRUCT_COMMANDES.Etat_Photo = Photo_non;
        STRUCT_COMMANDES.Photo_Duree = 1;
        STRUCT_COMMANDES.Photo_Nbre = 1;
    }
}
void Commande_SPH(void)
{
    STRUCT_COMMANDES.Etat_Photo = Photo_stop;
}
	
	
	
