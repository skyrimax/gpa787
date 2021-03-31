#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

/* Définition des fonctions de controleur
 * 
 * consigne_h : consigne de hauteur à atteindre
 * consigne_t : consigne de température à atteindre
 * 
 * hd : hauteur de l'eau dans le réservoir à cette période d'échantillonnage
 * td : température de l'eau dans le réservoir à cette période d'échantillonnage
 * 
 * uh : pointeur de l'emplacement mémoire où écrire la commande du robinet d'eau chaude
 * oc : pointeur de l'emplacement mémoire où écrire la commande du robinet d'eau froide
 * 
 * params : structure de paramètres externes nécessaire à la fonction
 */
typedef void (*fonct_controleur)(double consigne_h, double consigne_t, double hauteur, double temperature, double *uh, double *uc, void* params);

/* Définition des fonction de sortie des données de la simulation
 * 
 * i : numéro d'échantillon
 * 
 * consigne_h : consigne de hauteur à atteindre
 * consigne_t : consigne de température à atteindre
 * 
 * uH : pointeur de l'emplacement mémoire où écrire la commande du robinet d'eau chaude
 * uC : pointeur de l'emplacement mémoire où écrire la commande du robinet d'eau froide
 * 
 * hautM : hauteur de l'eau dans le réservoir à cette période d'échantillonnage
 * tempM : température de l'eau dans le réservoir à cette période d'échantillonnage
 * 
 * size_tab : nb d'échantillons conservé en mémoire, correspond à la taille des tableaux
 * 
 * tab_consigne_h : tableau conservant la conssigne de hauteur à atteindre
 * tab_consigne_t : tableau conservant la conssigne de température à atteindre
 * 
 * tab_uH : tableau conservant la commande du robinet d'eau chaude
 * tab_uC : tableau conservant la commande du robinet d'eau froide
 * 
 * tab_hautM : tableau conservant la hauteur de l'eau dans le réservoir
 * tab_tempM : tableau conservant la température de l'eau dans le réservoir
 * 
 * params : paramètres externes à la fonction
 */
typedef void (*fonct_data_output)(size_t i, double consigne_h, double consigne_t, double uH, double uC, double hautM, double tempM, double hauteur, double temperature,
                size_t size_tabs, double tab_consigne_h[], double tab_consigne_t[], double tab_uH[], double tab_uC[], double tab_hautM[], double tab_tempM[],
                double* tab_hauteur, double* tab_temperature, void* params);

// Structure d'argument pour sélectionner la commande
typedef struct
{
    // Pin reliée au boutton à enfoncer
    uint8_t pin;

    // Valeur de commande à assigner
    double consigne_a_assigner;

    // Emplacement mémoire de la commande
    double* consigne;
}args_commande;

/* Fonction initialisant la structure d'argument pour la sélection de commande
 * 
 * args : pointeur vers la structure d'argument à initialiser
 * 
 * pin : pin reliée au boutton à enfoncer
 * 
 * consigne_a_assigner : valeur de commande à assigner
 * 
 * consigne : emplacement mémoire de la commande
 * 
 * retourne toujours EXIT_SUCCESS
 */
int init_param_commande(args_commande* args, uint8_t pin, double consigne_a_assigner, double* consigne)
{
    args->pin = pin;
    args->consigne_a_assigner = consigne_a_assigner;
    args->consigne = consigne;

    return EXIT_SUCCESS;
}

// Structure d'argument pour le simulateur
typedef struct
{
    // Fréquence d'échantillonnage
    double fs;

    // Consignes
    double consigne_hauteur;
    double consigne_temp;

    double uH; // commande du robinet d'eau chaude
    double uC; // commande du robinet d'eau froide

    double tempM; // lecture du capteur de température
    double hautM; // lecture du capteur de hauteur

    double temperature; // température de l'eau dans le réservoir
    double hauteur; // heauteur de l'eau dans le réservoir

    // Taille des tableau d'historique
    size_t size_tabs;

    // Historiques des consigne de hauteur et de température
    double* tab_consigne_h;
    double* tab_consigne_t;

    // Historique des commandes des robinets d'eau chaude et froiude
    double* tab_uH;
    double* tab_uC;

    // Historique des valeur lues par les capteur de hauteur et de température
    double* tab_hautM;
    double* tab_tempM;

    // Historique des valeurs échantillonnées de hauteur et température
    double* tab_temperature;
    double* tab_hauteur;

    // Fonction du contrôleur
    fonct_controleur controleur;

    // Arguments du controleur
    void* controleur_params;

    // Capacité courante du tableau de pointeur de fonction
    size_t capacity;

    // Nombre de fonction d'output data
    size_t nb_data_output_fonct;

    // tableau de fonctions d'output data
    fonct_data_output* data_output;

    // Tableau d'argument externe pour le data output
    void** data_output_params;
}args_simulation;

/* Fonction initialisant la structure de paramêtre pour la simulation
 * 
 * fs : fréquence d'échantillonnade la la simulation
 * 
 * taille_histo : nombre d'échantillons à garder dans l'historique, détermine la taille des tableaux
 * 
 * controleur : fonction de forme fonct_controleur utilisé pour le controle du système
 * 
 * params_controleur : structure de paramètres pour le controleur, void* pour pouvoir recevoir n'importe quelle structure
 * 
 * Les fonction de callback sont ajouté avec la fonction add_data_output_fonct
 * 
 * Retourne s'il y a eu un problème lors de l'initialisation
 */
int init_param_simulation(args_simulation *args, double fs, size_t taille_histo, fonct_controleur controleur, void* controleur_params)
{
    // Initialiser les tableaux de data outputs à NULL pour détecter les erreurs d'assignation
    args->data_output = NULL;
    args->data_output_params = NULL;

    // Initialisser les tableau d'historiques à NULL pour détecter les erreurs d'assignation
    args->tab_consigne_h = NULL;
    args->tab_consigne_t = NULL;

    args->tab_uH = NULL;
    args->tab_uC = NULL;

    args->tab_hautM = NULL;
    args->tab_tempM = NULL;

    args->tab_hauteur = NULL;
    args->tab_temperature = NULL;

    // Capacité initiale de 4 éléments
    args->capacity = 4;

    // Aucune fonction de data output à l'initialisation
    args->nb_data_output_fonct = 0;

    // Assigner la taille des tableaux d'historique
    args->size_tabs = taille_histo;

    // Allocation de la mémoire pour les fonction de data output
    args->data_output = (fonct_data_output*) malloc(args->capacity * sizeof(fonct_data_output));
    args->data_output_params = (void**) malloc(args->capacity * sizeof(void*));

    // Allocation de la mémoire pour les tableau d'historique
    args->tab_consigne_h = (double*) malloc(args->size_tabs * sizeof(double));
    args->tab_consigne_t = (double*) malloc(args->size_tabs * sizeof(double));
    args->tab_uH = (double*) malloc(args->size_tabs * sizeof(double));
    args->tab_uC = (double*) malloc(args->size_tabs * sizeof(double));
    args->tab_hautM = (double*) malloc(args->size_tabs * sizeof(double));
    args->tab_tempM = (double*) malloc(args->size_tabs * sizeof(double));
    args->tab_hauteur = (double*) malloc(args->size_tabs * sizeof(double));
    args->tab_temperature = (double*) malloc(args->size_tabs * sizeof(double));

    // S'assurer que la mémoire à bien été allouée
    if(!args->data_output || !args->data_output_params || !args->tab_consigne_h ||
        !args->tab_consigne_t || !args->tab_uH || !args->tab_uC ||
        !args->tab_hautM || !args->tab_tempM || !args->tab_hauteur ||
        !args->tab_temperature) {
        free(args->tab_consigne_h);
        free(args->tab_consigne_t);
        free(args->tab_uH);
        free(args->tab_uC);
        free(args->tab_hautM);
        free(args->tab_tempM);
        free(args->tab_hauteur);
        free(args->tab_temperature);

        return EXIT_FAILURE;
    }

    // Assignation de la fréquence d'échantillonnage
    args->fs = fs;

    // Initialisation des commandes et consignes
    args->consigne_hauteur = 0;
    args->consigne_temp = 0;
    args->uH = 0;
    args->uC = 0;

    // Ajouter le controleur à la simulation
    args->controleur = controleur;
    args->controleur_params = controleur_params;

    return EXIT_SUCCESS;
}

/* Fonction pour ajouter une fonction de data output à la simulation
 * 
 * args : pointeur de la structure à laquelle ajouter la fonction
 * 
 * data_output : fonction de data output à ajouter
 * 
 * data_output_params : structure de paramètres externes nécessaires à la fonction
 * 
 * retourne s,'il y a en une erreure
 */
int add_data_output_fonct(args_simulation* args, fonct_data_output data_output, void* data_output_params)
{
    // Buffer du tableau pour étendre la capacité au besoins
    fonct_data_output* buffer_data_output = NULL;
    void** buffer_data_output_params = NULL;

    // Si le tableau est complètement rempli
    if(args->nb_data_output_fonct >= args->capacity) {
        // Doubler la capacité 
        args->capacity *= 2;

        // Allouer des espaces mémoire 2 fois plus grand
        buffer_data_output = (fonct_data_output*) malloc(args->capacity * sizeof(fonct_data_output));
        buffer_data_output_params = (void**) malloc(args->capacity * sizeof(void**));

        // S'assurer que les 2 nouveau espace sont bien alloués
        if(!buffer_data_output || !buffer_data_output_params) {
            // Libérer les 2 nouveau espaces mémoires au cas ou un des 2 aurait été alloué
            free(buffer_data_output);
            free(buffer_data_output_params);

            return EXIT_FAILURE;
        }

        // Déplacer les données vers les nouveaux espaces mémoire
        for(int i = 0; i < args->nb_data_output_fonct; ++i) {
            buffer_data_output[i] = args->data_output[i];
            buffer_data_output_params[i] = args->data_output_params[i];
        }

        // libérer les anciens espaces mémoire
        free(args->data_output);
        free(args->data_output_params);

        // assigner les nouveaux espaces mémoires dans la structure
        args->data_output = buffer_data_output;
        args->data_output_params = buffer_data_output_params;
    }

    args->data_output[args->nb_data_output_fonct] = data_output;
    args->data_output_params[args->nb_data_output_fonct] = data_output_params;

    ++args->nb_data_output_fonct;

    return EXIT_SUCCESS;
}

/* Fonction pour désallouer la mémoire occupée dans la structure d'argument pour la simulation
 * 
 * args : pointeur vers la structure d'argument à détruire
 */
void destroy_param_simulation(args_simulation *args)
{
    free(args->data_output);
    free(args->data_output_params);
    free(args->tab_consigne_h);
    free(args->tab_consigne_t);
    free(args->tab_uH);
    free(args->tab_uC);
    free(args->tab_hautM);
    free(args->tab_tempM);
    free(args->tab_hauteur);
    free(args->tab_temperature);
}

/* Strtucture de donnée contenant les coefficient du contrôleur PID
 * 
 * P : coefficient du gain proportionnel
 * 
 * I : coefficient du gain intégral
 * 
 * D : coefficient du gain dérivé
 * 
 * Ts: période d'échantillonnage
 * 
 * e0 : erreur à l'instant k
 * 
 * e1 : erreur à l'instant k - 1
 * 
 * e2 : erreur à l'instant k - 2
 * 
 * u0 : commande envoyé précédemment
 */
typedef struct
{
    double min_commande;
    double max_commande;
    double P;
    double I;
    double D;
    double Ts;
    double e0;
    double e1;
    double e2;
    double u0;
}args_PID;

/* Fonction initialisant la structure d'argument d'un controleur PID
 * 
 * args : pointeur vers la structure d'argument à initialiser
 * 
 * P : coefficient du gain proportionnel
 * 
 * I : coefficient du gain intégral
 * 
 * D : coefficient du gain dérivé
 * 
 * Ts: période d'échantillonnage
 * 
 * retourne toujours EXIT_SUCCESS
 */
int init_param_PID(args_PID *args, double min_commande, double max_commande, double P, double I, double D, double Ts)
{
    args->min_commande = min_commande;
    args->max_commande = max_commande;

    args->P = P;
    args->I = I;
    args->D = D;
    args->Ts = Ts;

    args->e0 = 0;
    args->e1 = 0;
    args->e2 = 0;
    args->u0 = 0;

    return EXIT_SUCCESS;
}

/* Structure de donnée pour le controleur MIMO PID
 * 
 * params_PID_1 : structure de données pour le controleur PID de la 1re boucle
 * 
 * params_PID_2 : structure de données pour le controleur PID de la 2e boucle
 */
typedef struct
{
    args_PID params_PID_1;
    args_PID params_PID_2;
}args_MIMO_PID;

/* Fonction initialisant la structure d'arguements pour un controleur MIMO PID
 * 
 * args : pointeur vers la structure d'argument à initialiser
 * 
 * P1 : coefficient du gain proportionnel pour le controleur de la boucle 1
 * 
 * I1 : coefficient du gain intégral pour le controleur de la boucle 1
 * 
 * D1 : coefficient du gain dérivé pour le controleur de la boucle 1
 * 
 * Ts1: période d'échantillonnage pour le controleur de la boucle 1
 * 
 * P2 : coefficient du gain proportionnel pour le controleur de la boucle 2
 * 
 * I2 : coefficient du gain intégral pour le controleur de la boucle 2
 * 
 * D2 : coefficient du gain dérivé pour le controleur de la boucle 2
 * 
 * Ts2: période d'échantillonnage pour le controleur de la boucle 2
 * 
 * retourne toujours EXIT_SUCCESS
 */
int init_param_MIMO_PID(args_MIMO_PID *args, double min_commande1, double max_commande1, double P1, double I1, double D1, double Ts1, double min_commande2, double max_commande2, double P2, double I2, double D2, double Ts2)
{
    init_param_PID(&args->params_PID_1, min_commande1, max_commande1, P1, I1, D1, Ts1);
    init_param_PID(&args->params_PID_2, min_commande2, max_commande2, P2, I2, D2, Ts2);

    return EXIT_SUCCESS;
}

/* Fonction de base d'un controleur de type PID
 * consigne : Valeur à atteindre
 * 
 * echantillon : valeur échantillonnée
 * 
 * param : structure de donnée contenant les gains ainsi que les erreures k-1 et k-2
 * 
 * retourne la valeur de la commande à envoyer au système
 */
double PID(double consigne, double echantillon, args_PID* params)
{
    params->e0 = consigne - echantillon;
    
    double u1 = params->P*(params->e0 - params->e1) +
                params->I*params->Ts*(params->e0 + params->e1)/2 +
                params->D*(params->e0 - 2*params->e1 + params->e2) + params->u0;

    u1 = (u1 < params->min_commande) ? params->min_commande : u1;
    u1 = (u1 > params->max_commande) ? params->max_commande : u1;

    params->e2 = params->e1;
    params->e1 = params->e0;

    params->u0 = u1;

    return u1;
}

/* Fonction controleur en boucle ouverte
 * 
 * Pour les non-initiés, ça veut dire pas de controleur
 * 
 * Suit la forme de fonct_controleur pour les arguments et le retour
 * 
 * Ce controleur le nécéssite pas d'arguments externes, on peut donc simplement passer NULL
 */
void boucle_ouverte(double consigne_h, double consigne_t, double hauteur, double temperature, double *uh, double *uc, void* params)
{
    // Ce controleur ne fait rien, dah c'est une boucle ouverte
} 

/* Fonction controleur ON/OFF
 * 
 * Ouvre les 2 robinets à 100 si l'échantillon est plus petit que la commande
 * Ferme complêtement les robinets sinon
 * 
 * Suit la forme de fonct_controleur pour les arguments et le retour
 * 
 * Ce controleur le nécéssite pas d'arguments externes, on peut donc simplement passer NULL
 */
void on_off_hauteur(double consigne_h, double consigne_t, double hauteur, double temperature, double *uh, double *uc, void* params)
{
    if(hauteur < consigne_h) {
        *uh = 5.0;
        *uc = 5.0;
    }
    else {
        *uh = 0.0;
        *uc = 0.0;
    }
}

/* Fonction controleur PID pour l'asservissement de la hauteur de l'eau
 * 
 * Calcule la consigne à envoyer au deux robinets avec un controleur PID
 * 
 * Suit la forme de fonct_controleur pour les arguments et le retour
 * 
 * Ce controleur nécessite une structure de donnée contenant les coefficients
 * pour les gains proportionnel, intégral et dérivé.
 */
void PID_hauteur(double consigne_h, double consigne_t, double hauteur, double temperature, double *uh, double *uc, void* params)
{
    args_PID * params_s = (args_PID *) params;
    double commande = PID(consigne_h, hauteur, params_s);

    *uh = commande;
    *uc = commande;
}

/* Fonction controleur MIMO PID pour l'asservissement de la hauteur et la température de l'eau
 * 
 * Calcule la consigne à envoyer au deux robinets avec deux controleur PID, un par robinet
 * 
 * Dans cette configuration, la hauteur controle le robinet d'eau froide
 * et la température controle le robient d'eau chaude
 * 
 * Suit la forme de fonct_controleur pour les arguments et le retour
 * 
 * Ce controleur nécessite une structure de donnée contenant les coefficients
 * pour les gains proportionnel, intégral et dérivé des 2 controleur PID
 */
void MIMO_PID_config1(double consigne_h, double consigne_t, double hauteur, double temperature, double *uh, double *uc, void* params)
{
    args_MIMO_PID* params_s = (args_MIMO_PID *) params;
    
    *uc = PID(consigne_h, hauteur, &params_s->params_PID_1);
    *uh = PID(consigne_t, temperature, &params_s->params_PID_2);
}

/* Fonction controleur MIMO PID pour l'asservissement de la hauteur et la température de l'eau
 * 
 * Calcule la consigne à envoyer au deux robinets avec deux controleur PID, un par robinet
 * 
 * Dans cette configuration, la hauteur controle le robinet d'eau chaude
 * et la température controle le robient d'eau froide
 * 
 * Suit la forme de fonct_controleur pour les arguments et le retour
 * 
 * Ce controleur nécessite une structure de donnée contenant les coefficients
 * pour les gains proportionnel, intégral et dérivé des 2 controleur PID
 */
void MIMO_PID_config2(double consigne_h, double consigne_t, double hauteur, double temperature, double *uh, double *uc, void* params)
{
    args_MIMO_PID* params_s = (args_MIMO_PID *) params;
    
    *uh = PID(consigne_h, hauteur, &params_s->params_PID_1);
    *uc = PID(consigne_t, temperature, &params_s->params_PID_2);
}

/* Fonction d'affichage des valeur dans la console
 *
 * Affiche chacune des valeurs actuelle dans la console
 * 
 * Suit la forme de funct_data_output pour les argument et le retour
 * 
 * Ne nécessite pas de paramètre externe, on peut donc passer NULL
 */
void print_console(size_t i, double consigne_h, double consigne_t, double uH, double uC, double hautM, double tempM, double hauteur, double temperature,
                size_t size_tabs, double tab_consigne_h[], double tab_consigne_t[], double tab_uH[], double tab_uC[], double tab_hautM[], double tab_tempM[],
                double* tab_hauteur, double* tab_temperature, void* params)
{
    static int first = 1;

    if(first) {
        printf("Temps (s)\t consigne h (m)\t consigne t (°C)\t commande H (V)\t commande C (V)\t capteur h (V)\t capteur t (V)\t hauteur (m)\t température (°C)\n");
        first = 0;
    }
    printf("   %6i\t        %7.3f\t         %7.3f\t        %7.3f\t        %7.3f\t       %7.3f\t       %7.3f\t     %7.3f\t          %7.3f\n",i, consigne_h, consigne_t, uH, uC, hautM, tempM, hauteur, temperature);
}

/* Fonction pour ajouter les valeurs à une fichier sous forme de CSV
 * 
 * Chaque valeur est ajoutée dans un fichier de format CSV pour pouvoir être facilement analysé dans un autre logiciel
 * 
 * Suit la forme de funct_data_output pour les argument et le retour
 * 
 * Cette fonction nécessite un pointeur vers le fichier ou écrire
 */
void write_CSV(size_t i, double consigne_h, double consigne_t, double uH, double uC, double hautM, double tempM, double hauteur, double temperature,
                size_t size_tabs, double tab_consigne_h[], double tab_consigne_t[], double tab_uH[], double tab_uC[], double tab_hautM[], double tab_tempM[],
                double* tab_hauteur, double* tab_temperature, void* params)
{
    static int first = 1;

    if(first) {
        fprintf((FILE*) params, "Temps(s):consigne h (m):consigne t (°C):commande h (V):commande C (V):capteur h (V):capteur t (V):hauteur (m):température(°C)\n");
        first = 0;
    }
    fprintf((FILE*) params, "%i:%f:%f:%f:%f:%f:%f:%f:%f\n",i, consigne_h, consigne_t, uH, uC, hautM, tempM, hauteur, temperature);
}

/* Fonction : RESERVOIR:
 * 
 * Entrées :
 * 	uH : Commande du débit d'eau chaude (0 V fermée, 5 V ouverture maximale)
 * 	uC : Commande du débit d'eau froide (0 V fermée, 5 V ouverture maximale)
 * 	*tempM : Adresse de la variable de mesure de température (0 V => 10 C, 5 V => 60 C)
 * 	*tempH : Adresse de la variable de mesure de niveau (0 V => 0 m, 5 V => 3 m)
 * 
 * Sorties :
 * 	Aucune, car les valeurs mesurées sont envoyées aux adresses 
 * 	fournies à *tempM et *tempH.
 * 
 * Code fourni par Guy Gauthier
 */
void reservoir(double uH, double uC, double *tempM, double *hautM)
{
	double Fmax=0.35;  // Débit maximal d'un valve ouverte à 100% (en m^3/sec)
	double Fh = 0.0;   // Débit actuel d'eau chaude(en m^3/s)
	double Fc = 0.0;  // Débit actuel d'eau froide(en m^3/s)
	double Fp = 0.01; // Débit actuel parasite (en m^3/s)
	static double h=1.0, oldh = 0.0;   // Niveaux actuel et précédent (en mètres) 
	static double T=20.0, oldT = 0.0;	// Température actuelle et précédente (en C)

	Fh = Fmax*uH/5.0;
	Fc = Fmax*uC/5.0;
	
	oldh = h;
	oldT = T;

	// Simulation du niveau (mètres)
	h = oldh + 4*(Fh+Fc + Fp - 0.2*sqrt(oldh));
	if (h<0.0) h = 0.0;
	if (h>3.0) h = 3.0;

	// Simulation de la température
	T = oldT + 4*(Fh*(60-oldT)+Fc*(10-oldT)+Fp*(20-oldT))/oldh; 
	if (T<10.0) T = 10.0;
	if (T>60.0) T = 60.0;
	
	*tempM = (T-10.0)*5.0/50.0;
	*hautM = h*5.0/3.0;
}

/* Fonction qui mettera la commande à la valeur spécifié dans la structure
 * d'argument lorsque le bouton spécifié est enfoncé
 * 
 * args : structure d'argument
 * 
 * La fountion ne retourne rien
 * 
 * Sera appelée dans un thread
 */
void* set_consigne_pourc(void* args)
{
    while(1) {
        // Attendre que le bouton du bas soit enfoncé
        while(bcm2835_gpio_lev(((args_commande*)args)->pin)) {
            usleep(1000);
        }

        // Assigner 20% à la commande
        *((args_commande*)args)->consigne = ((args_commande*)args)->consigne_a_assigner;
    }

    pthread_exit(NULL);
}

/* Fontion simulation du réservoir, sera appelée dans un thread
 * Elle sauvegarde 
 * 
 * La fonction reçois ses paramètres via la structure args
 * 
 * La fonction ne retourne aucune valeur
 */
void* simulation_Reservoir(void* args)
{
    args_simulation * args_s = (args_simulation*) args; // Recast de args pour pouvoir accéder au éléments

    double ps = 1 / (args_s->fs); // Calcul de la période entre chaque points

    clock_t debut, fin; // Variables de temps

    size_t t = 0; // Compte les secondes écoulées, correspond au nombre d'échantillons requeuillis

    size_t i;

    debut  = clock();

    while(1)
    {
        // Simulation du réservoir, échatillonnage
        reservoir(args_s->uH, args_s->uC, &args_s->tempM, &args_s->hautM);

        args_s->hauteur = args_s->hautM*3.0/5.0;
        args_s->temperature = args_s->tempM*50.0/5.0+10.0;

        // Le controleur retourne une commade avec les échantillons et les consignes
        args_s->controleur(args_s->consigne_hauteur, args_s->consigne_temp, args_s->hauteur, args_s->temperature, &args_s->uH, &args_s->uC, args_s->controleur_params);

        // décaler les échantillons dans les tableau d'historiques
        for(i = 1; i < args_s->size_tabs; ++i) {
            args_s->tab_consigne_h[i-1] = args_s->tab_consigne_h[i];
            args_s->tab_consigne_t[i-1] = args_s->tab_consigne_t[i];
            args_s->tab_uH[i-1] = args_s->tab_uH[i];
            args_s->tab_uC[i-1] = args_s->tab_uC[i];
            args_s->tab_hautM[i-1] = args_s->tab_hautM[i];
            args_s->tab_tempM[i-1] = args_s->tab_tempM[i];
            args_s->tab_hauteur[i-1] = args_s->tab_hauteur[i];
            args_s->tab_temperature[i-1] = args_s->tab_temperature[i];
        }

        // Insérer les derniers échantillions dans les tableaux d'historiques
        args_s->tab_consigne_h[args_s->size_tabs-1] = args_s->consigne_hauteur;
        args_s->tab_consigne_t[args_s->size_tabs-1] = args_s->consigne_temp;
        args_s->tab_uH[args_s->size_tabs-1] = args_s->uH;
        args_s->tab_uC[args_s->size_tabs-1] = args_s->uC;
        args_s->tab_hautM[args_s->size_tabs-1] = args_s->hautM;
        args_s->tab_tempM[args_s->size_tabs-1] = args_s->tempM;
        args_s->tab_hauteur[args_s->size_tabs-1] = args_s->hauteur;
        args_s->tab_temperature[args_s->size_tabs-1] = args_s->temperature;

        for(i = 0; i < args_s->nb_data_output_fonct; ++i) {
            args_s->data_output[i](t, args_s->consigne_hauteur, args_s->consigne_temp,
                                    args_s->uH, args_s->uC, args_s->hautM, args_s->tempM,
                                    args_s->hauteur, args_s->temperature,
                                    args_s->size_tabs,
                                    args_s->tab_consigne_h, args_s->tab_consigne_t,
                                    args_s->tab_uH, args_s->tab_uC,
                                    args_s->tab_hautM, args_s->tab_hautM,
                                    args_s->tab_hauteur, args_s->tab_temperature,
                                    args_s->data_output_params[i]);
        }

        ++t;

        fin  = clock(); // Temps écoulé depuis le lancement du programme

        // La différence (fin - debut) donne le temps d’exécution du code
        // On cherche une durée égale à demiPer. On compense avec un délai.
        // CLOCK_PER_SEC est le nombre de coups d’horloges en 1 seconde
        usleep(1000000 * (ps - ((double)(fin - debut) / ((double)CLOCKS_PER_SEC))));
        debut = clock();
    }

    pthread_exit(NULL);
}

int main(int args, char *argv[])
{
    int status = EXIT_SUCCESS;

    // Thread de la simulation
    pthread_t thread_simulation;

    // Structure de paramêtres du controleur PID
    args_MIMO_PID params_MIMO_PID;

    // Structure de la simulation
    args_simulation struct_simulation;

    // Fichier CSV ou écrire les données durant la simulation
    FILE* csv_file = NULL;
    csv_file = fopen("resultats_simulation_PID_MIMO_cfg2.csv", "w");
    //csv_file = fopen("test.csv", "w");

    if(!csv_file) {
        printf("Unable to open csv file for data output\n");
        status = EXIT_FAILURE;
        goto init_fail;
    }

    // Initialisation de la structure de paramêtres du PID
    init_param_MIMO_PID(&params_MIMO_PID, 0.0, 5.0, 2.0, 0.1, 0, 1.0/1.0, 0.0, 5.0, -2.0/6.0, -0.1/6.0, -0, 1.0/1.0);

    // Initialisation de la structure de la simulation
    if(init_param_simulation(&struct_simulation, 1.0, 500, MIMO_PID_config2, &params_MIMO_PID)) {
        printf("Unable to initialize parameter structure for simulation\n");
        status = EXIT_FAILURE;
        goto init_fail;
    }

    // Ajouts des fonction d'affichage du data
    if(add_data_output_fonct(&struct_simulation, print_console, NULL)) {
        printf("Unable to add print_console do simulation structure\n");
        status = EXIT_FAILURE;
        goto init_fail;
    }
    
    if(add_data_output_fonct(&struct_simulation, write_CSV, csv_file)) {
        printf("Unable to add print_console do simulation structure\n");
        status = EXIT_FAILURE;
        goto init_fail;
    }

    // Initialiser les commandes à 20 pourcents
    struct_simulation.consigne_hauteur = 2.5;
    struct_simulation.consigne_temp = 55;

    // Initialisation du bcm2835
    if (!bcm2835_init())
    {
        printf("Can't init bcm2835\n");
        status = EXIT_FAILURE;
        goto init_fail;
    }

    // Configuration du GPIO pour bouton - poussoir 1
    bcm2835_gpio_fsel(19, BCM2835_GPIO_FSEL_INPT);
    // Configuration du GPIO pour bouton - poussoir 2
    bcm2835_gpio_fsel(26, BCM2835_GPIO_FSEL_INPT);
    
    // Création et lancement du thread pour la sumulation
    if(pthread_create(&thread_simulation, NULL, &simulation_Reservoir, &struct_simulation)) {
        printf("Unable to start thread for simulation");
        status = EXIT_FAILURE;
        goto simu_start_fail;
    }

    // Boucle tant que les boutons-poussoir sont non enfoncés
    while (bcm2835_gpio_lev(19) || bcm2835_gpio_lev(26))
    {
        // printf("button loop\n");
        usleep(1000); // Délai de 1 ms !!!
    }

    // Si les boutons sont enfoncés, arrêt immédiat des threads
    pthread_cancel(thread_simulation);
    pthread_join(thread_simulation, NULL);
simu_start_fail:

    // Libérer le GPIO
    bcm2835_close();
init_fail:

    // Libérer la mémoire utilisée pour la simulation
    destroy_param_simulation(&struct_simulation);

    fclose(csv_file);

    return status;
}