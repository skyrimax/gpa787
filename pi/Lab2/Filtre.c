/*
 * Filtre .c
 *
 * Ce programme fait le filtrage du signal reçu à
 * l'entré 1 du mcp3204 et retourne le signal filtré
 * à la sortie A du mcp4922
 * 
 * L’arrêt sera provoqué par un appui sur le bouton-poussoir 1.
 * Aucun paramètre en entrée.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <string.h>

// Énumération représentant la fenêtre à utiliser pour un filtre FIR
typedef enum{
  Rectangulaire,
  Hanning,
  Hamming,
  Blackman
} fenetre_FIR;

// Énumération représentant le type de filtre
typedef enum{
  Passe_Bas,
  Passe_Haut,
  Passe_Bande,
  Coupe_Bande,
} type_filtre;

// Définir la structure des arguments pour la fonction filtre
typedef struct 
{
  // Fréquence d'échantillonnage
  double fs;

  // Voltage de référence
  double Vref;

  // Coefficients du filtre
  size_t nb_num;
  double *num;
  size_t nb_denom;
  double *denom;

  // Tableaux contenant les valeurs d'entré et de sortie présentes et passées
  double *x;
  double *y;
}args_filtre;

// Y u no already exist?
// Retourne la plus grande valeur entre nb1 et nb2
double max(double nb1, double nb2)
{
  return (nb1 > nb2) ? nb1 : nb2;
}

/* Fonction initialisant la structure d'arguments pour un filtre
 * 
 * args : pointeur vers la structure d'argument à initialiser
 * 
 * fs : fréquence d'échantillonnage
 * 
 * Vref : Voltage de référence du mcp3204
 * 
 * nb_num : nombre de coefficient au numérateur
 * 
 * num : tableau de coefficient du numérateur (fera une copie)
 * 
 * nb_denom : nombre de coefficient au dénominateur
 * 
 * demum : tableau de coefficient du dénominateur (fera une copie)
 * 
 * Retourne s'il y a eu un problème durant l'allocation
 */
int init_param_filtre(args_filtre *args, double fs, double Vref, size_t nb_num, double *num, size_t nb_denom, double *denum)
{
  // Indexeur pour l'initialisation de buffers
  size_t i;
  size_t limit = max(nb_num, nb_denom);
  
  // Initialisation des pointeurs pour pouvoir détecter les erreurs d'allocation de mémoire
  args->num = NULL;
  args->denom = NULL;
  args->x = NULL;
  args->y = NULL;

  // Assignation des tailles des numinateur et dénominateur
  args->nb_num = nb_num;
  args->nb_denom = nb_denom;

  // Allocation de la mémoire
  args->num = (double *)malloc(nb_num * sizeof(double));
  args->x = (double *)malloc(nb_num * sizeof(double));
  args->denom = (double *)malloc(nb_denom * sizeof(double));
  args->y = (double *)malloc((nb_denom + 1) * sizeof(double));

  //// S'assurer que la mémoire a bien été assignée
  //if ((!args->num && nb_num) || (!args->denum && nb_denom) || (!args->x && nb_num) || !args->y)
  //{
  //  printf("Can't allocate memory for coefficients\n");
  //  return EXIT_FAILURE;
  //}

  if(!args->num && nb_num) {
    printf("Can't allocate memory for num coefficients\n");
    return EXIT_FAILURE;
  }

  if(!args->denom && nb_denom) {
    printf("Can't allocate memory for denum coefficients\n");
    return EXIT_FAILURE;
  }

  if(!args->x && nb_num) {
    printf("Can't allocate memory for x buffer\n");
    return EXIT_FAILURE;
  }

  if(!args->y) {
    printf("Can't allocate memory for y buffer\n");
    return EXIT_FAILURE;
  }

  // Copie des coefficients
  memcpy(args->num, num, nb_num * sizeof(*num));
  memcpy(args->denom, denum, nb_denom * sizeof(*num));

  // Initialisation des buffers d'entrées et sorties
  for (i = 0; i < limit; ++i)
  {
    if(i < nb_num)
    {
      args->x[i] = 0;
    }
    
    if(i < nb_denom)
    {
      args->y[i] = 0;
    }
  }

  // Assignation de la fréquence d'échantillonnage
  args->fs = fs;

  // Assignation du voltage de référence du mcp3402
  args->Vref = Vref;

  // Prints for sanity check
  //printf("Coeffs num :\n");
  //for(i = 0; i < nb_num; ++i) {
  //  printf("Coeff %d : %lf\n", i, args->num[i]);
  //}

  //printf("\nCoeffs denom\n");
  //for(i = 0; i < nb_denom; ++i) {
  //  printf("Coeff %d : %lf\n", i, args->denom[i]);
  //}

  return EXIT_SUCCESS;
}

/* Fonction pour désallouer la mémoire occupée dans une structure d'argument pour un filtre
 * 
 * args : pointeur vers la structure d'argument à détruire
 */
void destroy_param_filtre(args_filtre *args)
{
  free(args->num);
  free(args->denom);
  free(args->x);
  free(args->y);
}

/* Function pour l'initialisation d'une structure de filtre avec un filtre FIR
 * Supporte la sélection d'une fenêtre
 * 
 * args : pointeur vers la structure d'argument à initialiser
 * 
 * Vref : Voltage de référence du mcp3204
 * 
 * fs : fréquence d'échantillonnage
 * 
 * fc1 : fréquence de coupure 1 (utilisé pour tout)
 * 
 * fc2 : fréquence de coupure 2 (utilisé seulement pour passe-bande et coupe-bande)
 * 
 * fc1 <= fc2 obligatoirement si coupe-bande ou passe-bande choisi
 * 
 * N : nombre de points pour le filtre (doit être impair)
 * 
 * type : type du filtre à créer
 * 
 * fenetre : fenêtre à applique au filtre
 */
int init_filtre_FIR(args_filtre *args, double Vref, double fs, double fc1, double fc2, size_t N, type_filtre type, fenetre_FIR fenetre)
{
  // Emplacement bidon pour le dénum du filtre FIR
  double denum[] = {};

  // N doit être impair
  if(!(N%2))
  {
    printf("N ne peut pas être pair\nValeur de N : %d\n",N);
    return EXIT_FAILURE;
  }

  // fc1 <= fc2 obligatoirement si coupe-bande ou passe-bande choisi
  if((type == Passe_Bande || type == Coupe_Bande) && fc1 > fc2)
  {
    printf("Erreur fréquences de coupure\nfc1 : %lf\nfc2 : %lf\n", fc1, fc2);
    return EXIT_FAILURE;
  }

  // Fréquences normalisée
  double v1 = 2 * fc1 / fs;
  double v2 = 2 * fc2 / fs;

  // limite pour N points
  int Q = (N-1)/2;

  // Variable contenant le coefficient venant du filtre pur
  double Cn;

  // Variable contenant le coefficient venant de la fenêtre
  double Wh;

  // Tableau qui contiendra les coefficients pour le filtre
  double* coeff = NULL;

  // Assignation du tableau
  coeff = (double*)malloc(N*sizeof(double));

  // S'assurer que la mémoire a bien été assignée
  if(!coeff)
  {
    printf("Can't allocate memory for coefficients\n");
    return EXIT_FAILURE;
  }

  for(int i = 0, n = -Q; i < N; ++i, ++n) {
    // Calcul du coefficient correspondant au type de filtre choisi
    switch (type)
    {
    case Passe_Bas:
      Cn = (n != 0) ? sin(n * M_PI * v1) / (n * M_PI) : v1;
      break;

    case Passe_Haut:
      Cn = (n != 0) ? -sin(n * M_PI * v1) / (n * M_PI) : 1 - v1;
      break;

    case Passe_Bande:
      Cn = (n != 0) ? (sin(n * M_PI * v2) - sin(n * M_PI * v1)) / (n * M_PI) : v2 - v1;
      break;
    
    case Coupe_Bande:
      Cn = (n != 0) ? (sin(n * M_PI * v1) - sin(n * M_PI * v2)) / (n * M_PI) : 1 + v1 - v2;
      break;
    
    default:
      Cn = 0;
      break;
    }

    // Calcul du coefficient correspondant à la fenêtre choisie
    switch (fenetre)
    {
    case Rectangulaire:
      Wh = 1;
      break;

    case Hanning:
      Wh = 0.5 + 0.5 * cos(n * M_PI / Q);
      break;

    case Hamming:
      Wh = 0.54 + 0.46 * cos(n * M_PI / Q);
      break;

    case Blackman:
      Wh = 0.42 + 0.5 * cos(n * M_PI / Q) + 0.08 * cos(2 * n * M_PI / Q);
      break;
    
    default:
      break;
    }

    coeff[i] = Cn * Wh;
  }

  int statut = init_param_filtre(args, fs, Vref, N, coeff, 0, denum);

  free(coeff);

  return statut;
}

/* Fonction poussant une voltage sur le mcp4922
 * 
 * v : volatage à pousser sur le 4922
 * Doit être entre 0.0 et Vref
 * 
 * Vref : voltage de référence du mcp4922
 * doit être positif non nul
 * 
 * canal : sélection du canal
 * 0 = canal A
 * 1 = canal B
 * 
 * buffered : sélectionne si le CDA doit opérer en mode buffered ou unbuffered
 * 0 = unbuffered
 * 1 = buffered
 * 
 * gain : sélectionne le type de gain
 * 0 = Vout = Vref * D/4096
 * 1 = Vout = 2 * Vref * D/4096
 * 
 * shutdown : séletionne le type d'opération
 * 1 = mode actif, le canal maintien sa valeur
 * 0 = shutdown, le canal deviens inactif, Vout est connecté à une résistance de 500 kohm
 */
void voltage_mcp4922(double v, double Vref, uint8_t canal, uint8_t buffered, uint8_t gain, uint8_t shutdown)
{
  //printf("V : %lf\n", v);
  //printf("Vref : %lf\n", Vref);
  //printf("Canal : %d\n", canal);
  //printf("buffered : %d\n", buffered);
  //printf("Gain : %d\n", gain);
  //printf("Shutdown : %d\n", shutdown);

  if (Vref <= 0)
    return;

  if (v < 0)
    v = 0;

  if (v > Vref)
    v = Vref;

  uint16_t data = 4095 * v / Vref;

  if (canal)
    data |= 0b1000000000000000;

  if (buffered)
    data |= 0b0100000000000000;

  if (gain)
    data |= 0b0010000000000000;

  if (shutdown)
    data |= 0b0001000000000000;

  //printf("Data : %x\n", data&0xFFF);
  uint8_t x[2];
  x[0] = (data >> 8) & 0x0FF;
  x[1] = data & 0x0FF;

  bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
  bcm2835_spi_transfern((char *)x, 2);
}

/* Fonction effectuant la lecture du voltage aux entrées du cmp3204
 * 
 * diff : sélection du type d'opération
 * 0 = voltage à l'entrée du canal spécifié
 * 1 = différence de voltage entre 2 canaux
 * 
 * canal : sélection du (des) canal  (canaux) à utiliser pour la lecture
 * si diff  == 0
 * 0 = canal 0
 * 1 = canal 1
 * 2 = canal 2
 * 3 = canal 3
 * 
 * si diff == 1
 * 0 = canal 0 IN+ et canal 1 IN-
 * 1 = canal 1 IN+ et canal 0 IN-
 * 2 = canal 2 IN+ et canal 3 IN-
 * 3 = canal 3 IN+ et canal 2 IN-
 */
double voltage_mcp3204(uint8_t diff, uint8_t canal)
{
  if (diff < 0)
    diff = 0;

  if (diff > 1)
    diff = 1;

  if (canal < 0)
    canal = 0;

  if (canal > 3)
    canal = 3;

  uint8_t data[3];

  data[0] = 0b00000100 | (diff << 1);
  data[1] = canal << 6;

  bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
  bcm2835_spi_transfern((char *)data, 3);

  uint16_t ret = (data[1] & 0x00FF);
  ret = ret << 8;
  ret += data[2];

  return ret * 5.0 / 4095;
}

/* Fonction de filtrage de signal qui sera appeléer par le thread "thread_filtre"
 * 
 * Elle fait le filtrage avec le filtre qu'elle reçoit en paramètre au travers
 * de la structure args
 * 
 * Le signal est échantillonné à partir du canal 0 du mcp3402
 * 
 * Le signal filtré est émi à partit du canal A du mcp4922
 * 
 * La fonction ne retourne aucune valeur
 * La fonction reçoit ses arguments au travers de la structure args
 */
void *filtre_signal(void *args)
{
  args_filtre *args_s = (args_filtre *)args; // Recast de args pour pouvoir accéder au éléments

  size_t i; // Indexeurs pour le filtrage

  clock_t debut, fin; // Variables de temps

  double ps = 1 / (args_s->fs); // Calcul de la période entre chaque points

  debut = clock();

  while (1)
  {
    // Mesurer le voltage au canal 1 mcp3204 et mettre la valeur au début du buffer d'entrée
    args_s->x[0] = voltage_mcp3204(0, 0) - args_s->Vref / 2;

    // Remettre à 0 la valeur de sortie au début du buffer de sorties
    args_s->y[0] = 0;

    // Somme des produits entre x[n-k] et h_num[k]
    for(i = 0; i < args_s->nb_num; ++i)
    {
      args_s->y[0] += args_s->x[i] * args_s->num[i];
    }

    // Somme des produits entre y[n-k-1] et h_denum[k]
    for(i = 0; i < args_s->nb_denom; ++i)
    {
      args_s->y[0] -= args_s->y[i + 1] * args_s->denom[i];
    }

    // Envoyer le voltage du signal filtrer au mcp4922
    voltage_mcp4922(args_s->y[0] + args_s->Vref / 2, args_s->Vref, 0, 0, 1, 1);

    // Décaler les buffers x et y
    for(i = args_s->nb_num - 1; i > 0; --i) {
      args_s->x[i] = args_s->x[i - 1];
    }

    // Décaler les buffers x et y
    for(i = args_s->nb_denom; i > 0; --i) {
      args_s->y[i] = args_s->y[i - 1];
    }

    fin = clock(); // Temps écoulé depuis le lancement du programme

    // La différence (fin - debut) donne le temps d’exécution du code
    // On cherche une durée égale à demiPer. On compense avec un délai.
    // CLOCK_PER_SEC est le nombre de coups d’horloges en 1 seconde
    usleep(1000000 * (ps - ((double)(fin - debut) / ((double)CLOCKS_PER_SEC))));
    debut = clock();
  }

  pthread_exit(NULL);
}

/* Fonction main
  *
  * Elle configure le GPIO et le thread.
  *
  * La fonction ne retourne aucune valeur.
  * La fonction n’exige aucun paramètre en entrée.
*/
int main(int argc, char **argv)
{
  // Identificateur des threads
  pthread_t thread_filtre;

  // Arguments pour le thread
  args_filtre args;

  // Filtre FIR passe bas à 31 points fenêtre rectangulaire
  double filtre_FIR_passe_bas_num[]={-0.021220659078919,-0.021623620818304,-0.019809085184633,-0.015591488063145,-0.0089421058462139,0.,0.010929240478706,0.023387232094718,0.036788301057175,0.05045511524271,0.063661977236758,0.075682672864066,0.085839369133414,0.093548928378863,0.098363164308347,0.1,0.098363164308347,0.093548928378863,0.085839369133414,0.075682672864066,0.063661977236758,0.05045511524271,0.036788301057175,0.023387232094718,0.010929240478706,0.,-0.0089421058462139,-0.015591488063145,-0.019809085184633,-0.021623620818304,-0.021220659078919};
  double filtre_FIR_passe_bas_denom[]={};

  // Filtre Butterworth passe bas 1er ordre
  double filtre_Butterworth_passe_bas_1er_ordre_num[] = {0.13672873599733,0.13672873599733};
  double filtre_Butterworth_passe_bas_1er_ordre_denom[] = {-0.7265425280054};

  // Filtre Butterworth passe bas 4er ordre
  double filtre_Butterworth_passe_bas_4e_ordre_num[] = {0.00041659920440667,0.0016663968176267,0.00249959522644,0.0016663968176267,0.00041659920440667};
  double filtre_Butterworth_passe_bas_4e_ordre_denum[] = {-3.1806385488743,3.8611943489943,-2.1121553551109,0.43826514226196};

  // Filtre Chebishev type 1 passe bas 4e order
  double filtre_Chebishev1_passe_bas_4e_ordre_num[] = {1.2984963538692E-4,5.1939854154768E-4,7.7909781232152E-4,5.1939854154768E-4,1.2984963538692E-4};
  double filtre_Chebishev1_passe_bas_4e_ordre_denom[] = {-3.6078961691291,4.9794708037511,-3.1107636829829,0.74152014735582};

  // Filtre Chebishev type 2 passe bas 1e order
  double filtre_Chebishev2_passe_bas_1er_ordre_num[] = {0.0015814187549159,0.0015814187549159};
  double filtre_Chebishev2_passe_bas_1er_ordre_denom[] = {-0.99683716249018};

  // Filtre Chebishev type 2 passe bas 4e order
  double filtre_Chebishev2_passe_bas_4e_ordre_num[] = {0.0097360574711215,-0.032136974698146,0.045452258999065,-0.032136974698146,0.0097360574711215};
  double filtre_Chebishev2_passe_bas_4e_ordre_denom[] = {-3.5729428087013,4.8079146527183,-2.8863251582841,0.65200370629};

  // Filtre Butterworth passe haut 1er ordre
  double filtre_Butterworth_passe_haut_2e_ordre_num[] = {0.8370891905663,-1.6741783811326,0.8370891905663};
  double filtre_Butterworth_passe_haut_2e_ordre_denom[] = {-1.6474599810769,0.7008967811884};

  // Initialiser la structure d'Arguments du filtre
  //if(init_param_filtre(&args, 1000.0, 5.0, 31, filtre_FIR_passe_bas_num, 0, filtre_FIR_passe_bas_denum))
  //{
  //  return EXIT_FAILURE;
  //}

  // Filtre FIR Passe-Bas à 31 points fenêtre rectangulaire
  //if(init_filtre_FIR(&args, 5.0, 1000.0, 50.0, 0.0, 31, Passe_Bas, Rectangulaire))
  //{
  //  printf("can't create filter\n");
  //  return EXIT_FAILURE;
  //}

  // Filtre FIR Passe-Bas à 31 points fenêtre blackman
  //if(init_filtre_FIR(&args, 5.0, 1000.0, 50.0, 0.0, 31, Passe_Bas, Blackman))
  //{
  //  printf("can't create filter\n");
  //  return EXIT_FAILURE;
  //}

  // Filtre Butterworth passe bas 1er ordre
  //if(init_param_filtre(&args, 1000.0, 5.0, 2, filtre_Butterworth_passe_bas_1er_ordre_num, 1, filtre_Butterworth_passe_bas_1er_ordre_denom))
  //{
  //  printf("can't create filter\n");
  //  return EXIT_FAILURE;
  //}

  // Filtre Butterworth passe bas 4er ordre
  //if(init_param_filtre(&args, 1000.0, 5.0, 5, filtre_Butterworth_passe_bas_4e_ordre_num, 4, filtre_Butterworth_passe_bas_4e_ordre_denum))
  //{
  //  printf("can't create filter\n");
  //  return EXIT_FAILURE;
  //}

  // Filtre Chebishev type 1 passe bas 4e order
  //if(init_param_filtre(&args, 1000.0, 5.0, 5, filtre_Chebishev1_passe_bas_4e_ordre_num, 4, filtre_Chebishev1_passe_bas_4e_ordre_denom))
  //{
  //  printf("can't create filter\n");
  //  return EXIT_FAILURE;
  //}

  // Filtre Chebishev type 2 passe bas 1e order
  //if(init_param_filtre(&args, 1000.0, 5.0, 2, filtre_Chebishev2_passe_bas_1er_ordre_num, 1, filtre_Chebishev2_passe_bas_1er_ordre_denom))
  //{
  //  printf("can't create filter\n");
  //  return EXIT_FAILURE;
  //}

  // Filtre Chebishev type 2 passe bas 4e order
  //if(init_param_filtre(&args, 1000.0, 5.0, 5, filtre_Chebishev2_passe_bas_4e_ordre_num, 4, filtre_Chebishev2_passe_bas_4e_ordre_denom))
  //{
  //  printf("can't create filter\n");
  //  return EXIT_FAILURE;
  //}

  // Filtre Butterworth passe haut 2e ordre
  if(init_param_filtre(&args, 1000.0, 5.0, 3, filtre_Butterworth_passe_haut_2e_ordre_num, 2, filtre_Butterworth_passe_haut_2e_ordre_denom))
  {
    printf("can't create filter\n");
    return EXIT_FAILURE;
  }

  // Initialisation du bcm2835
  if (!bcm2835_init())
  {
    printf("Can't init bcm2835\n");
    return EXIT_FAILURE;
  }

  //printf("bcm2835 initiated\n");
  // Configuration du GPIO pour bouton - poussoir 1
  bcm2835_gpio_fsel(19, BCM2835_GPIO_FSEL_INPT);

  // Initialisation du GPIO en SPI
  if (!bcm2835_spi_begin())
  {
    printf("can't init spi\n");
    return EXIT_FAILURE;
  }

  // Configurer l'ordre de transmition et MSBF
  bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);

  // Configurer la vitesse de transmission
  bcm2835_spi_set_speed_hz(16384);

  // Configuration du mode SPI
  bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);

  // Configuer le chip select et son état d'Activation
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW);
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

  // Création du thread "thread_filtre".
  // Lien avec la fonction filtre_signal.
  // Les paramêtres sont envoyé via args.
  if(pthread_create(&thread_filtre, NULL, &filtre_signal, &args))
  {
    return EXIT_FAILURE;
  }

  // Boucle tant que le bouton - poussoir est non enfoncé
  while (bcm2835_gpio_lev(19))
  {
    // printf("button loop\n");
    usleep(1000); // Délai de 1 ms !!!
  }

  // Si bouton - poussoir enfoncé, arrêt immédiat du thread
  pthread_cancel(thread_filtre);

  // Attente de l’arrêt du thread
  pthread_join(thread_filtre, NULL);

  // Remettre la sortie du mcp4922 à 0
  voltage_mcp4922(0, args.Vref, 0, 0, 1, 0);

  // Libérer le SPI
  bcm2835_spi_end();

  // Libérer le GPIO
  bcm2835_close();

  // Libérer la mémoire utilisée pour le filtrage
  destroy_param_filtre(&args);

  return EXIT_SUCCESS;
}