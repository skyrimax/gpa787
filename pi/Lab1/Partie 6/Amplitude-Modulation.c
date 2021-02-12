/*
 * Thread_Cligne_10_Hz_time .c
 *
 * Ce programme fait clignoter un DEL à une fréquence de 10 Hz .
 * Le clignotement se fait à l’intérieur d’un thread et le délai est
 * ajusté avec des fonctions de la librarie " time .h".
 * L’arrêt sera provoqué par un appui sur le bouton-poussoir 1.
 * Aucun paramètre en entrée.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#define NB_POINTS_COSIN 16 // Nombre de points utilisé pour discrétiser le cosin
#define NB_POINTS_MOYENNE 10 // Nombre de pôints utilisé pour la moyenne
#define FREQUENCE_GENEREE_MAX 32 // Fréquence maximale que le système aura à moduler
#define VREF 5.0         // Voltage de référence du mcp4922

// Définir la structure des arguments pour la fonction aplitude_modulation
struct args_AM
{
  size_t mode;
  size_t nb_modes;
  double frequence_echantillonnage;
  double Vref;
  size_t nb_points_cos;
  double* points_cos;
  double* points_cos_carree;
  size_t nb_points_moy;
  double* x;
  double y[3];
};


double *discret_ligne(int N)
{
  double *points = NULL;
  points = (double *)malloc(N * sizeof(double));

  if(!points){
    return NULL;
  }

  for(int i=0; i<N; ++i){
    points[i]=2*i/(N-1.0)-1;
    //printf("points[%d] = %lf\n", i, points[i]);
  }

  return points;
}

/* Fonction calculant les point de la fonction cosinus et les retourne
  * dans un tableau de N doubles
  * 
  * N : nombre de points désiré pour la discrétisation du cosinus
  */
double *discret_cosinus(size_t N)
{
  double *points = NULL;
  
  points = (double *)malloc(N * sizeof(double));

  if (!points)
  {
    return NULL;
  }

  for (uint i = 0; i < N; ++i)
  {
    points[i] = cos((2.0 * M_PI * i) / N);
  }

  return points;
}

/* Fonction calculant les points de la fonction cosinus^2 et les retourne
 * dans un tableau de N doubles
 * 
 * N : nombre de points désiré pour la discrétisation du cosinus^2
 */
double* discret_cosinus_carre(size_t N)
{
  double *points = NULL;
  double x;
  
  points = (double*)malloc(N * sizeof(double));

  if(!points)
  {
    return NULL;
  }

  for (uint i = 0; i < N; ++i)
  {
    x = cos((2.0 * M_PI * i) / N);
    points[i] = x * x;
  }

  return points;
}

/* Fonction initialisant une structure d'argument pour une modulation AM
 * 
 * args : pointeur vers la structure d'argument à initialiser
 * 
 * nb_points_cos : nombre de points à utiliser pour discrétiser le cosinus
 * 
 * nb_points_cos_carre : nombre de points à utiliser pour discrétiser le cosinus carré
 * 
 * nb_points_moy : nombre de points à utiliser pour faire la moyenne
 * 
 * frequence_max : fréquence maximal qui sera à moduler
 */
int init_param_AM(struct args_AM *args, size_t nb_points_cos, size_t nb_points_moy, double frequence_max, double Vref)
{
  args->points_cos=discret_cosinus(nb_points_cos);
  args->points_cos_carree=discret_cosinus_carre(nb_points_cos);

  args->x = NULL;
  args->x = (double*)malloc(nb_points_moy * sizeof(double));

  if(!args->points_cos || !args->points_cos_carree || !args->x || !args->y)
  {
    return EXIT_FAILURE;
  }

  args->nb_points_cos=nb_points_cos;

  args->nb_points_moy=nb_points_moy;

  args->frequence_echantillonnage=2*frequence_max*nb_points_cos;

  args->Vref=Vref;

  args->mode=0;
  args->nb_modes=3;

  return EXIT_SUCCESS;
}

/* Fonction pour désallouer la mémoire occupée dans une structure d'argument pour une modulation AM
 * 
 * args : pointeur vers la structure d'argument à détruire
 */
void destroy_param_AM(struct args_AM *args)
{
  free(args->points_cos);
  free(args->points_cos_carree);
  free(args->x);
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
  x[0] = (data>>8)&0x0FF;
  x[1] = data&0x0FF;
  
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
  if(diff < 0)
    diff = 0;
  
  if(diff > 1)
    diff = 1;
  
  if(canal < 0)
    canal = 0;

  if(canal > 3)
    canal =3;

  uint8_t data[3];

  data[0] = 0b00000100 | (diff << 1);
  data[1] = canal << 6;

  bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
  bcm2835_spi_transfern((char *) data, 3);

  uint16_t ret = (data[1] & 0x00FF);
  ret = ret << 8;
  ret += data[2];

  return ret * 5.0/4095;
}

/* Fonction modulation_AM qui sera appelée par le thread " thread_modulation "
 *
 * Elle fait la modulation AM di signal ainsi que la démodulation de celui-ci
 * 
 * La sortie est déterminée par le mode
 *
 * La fonction ne retourne aucune valeur 
 * La fonction reçoit ses paramêtre via la structure args
 */
void *modulation_AM(void *args)
{
  size_t i = 0;
  size_t j = 0;
  double v;
  double somme_x = 0;
  clock_t debut, fin; // Variables de temps
  struct args_AM *args_s = (struct args_AM *)args;

  double periode_echantillonnage = 1 / (args_s->frequence_echantillonnage); // Calcul de la période entre chaque points
  //printf("Sampling period : %lf\n", periode_echantillonnage);

  debut = clock(); // Temps écoulé depuis le lancement du programme

  // Boucle infinie pour le signal périodique
  while (1)
  {
    //printf("i : %d\n", i);
    v = voltage_mcp3204(0, 0) - args_s->Vref / 2;

    args_s->y[0] = v * args_s->points_cos[j];

    somme_x -= args_s->x[i];

    args_s->x[i] = args_s->y[1] = v * args_s->points_cos_carree[j];

    somme_x += args_s->x[i];

    args_s->y[2] = 2 * (somme_x / args_s->nb_points_moy);

    voltage_mcp4922(args_s->y[args_s->mode] + args_s->Vref / 2, args_s->Vref, 0, 0, 1, 1);

    i = (i + 1) % args_s->nb_points_moy;
    j = (j + 1) % args_s->nb_points_cos;

    fin = clock(); // Temps écoulé depuis le lancement du programme

    // La différence (fin - debut) donne le temps d’exécution du code
    // On cherche une durée égale à demiPer. On compense avec un délai.
    // CLOCK_PER_SEC est le nombre de coups d’horloges en 1 seconde
    usleep(1000000 * (periode_echantillonnage - ((double)(fin - debut) / ((double)CLOCKS_PER_SEC))));
    debut = clock();
  }
  pthread_exit(NULL);
}

/* Fonction choix_mode qui sera appelée par le thread " thread_choix_mode "
 * 
 * Cette fonction modifie le mode à l'appuis du bouton 2
 * 
 * La fonction ne retourne aucune valeur
 * La fonction reçois le mode à modifier au travers de la structure args
 */
void *choix_mode(void* args)
{
  struct args_AM *args_s = (struct args_AM *)args;

  while(1)
  {
    while(bcm2835_gpio_lev(26))
    {
      usleep(1000);
    }

    args_s->mode = (args_s->mode + 1) % args_s->nb_modes;

    while(!bcm2835_gpio_lev(26))
    {
      usleep(1000);
    }
  }
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
  pthread_t thread_modulation;
  pthread_t thread_choix_mode;

  // Arguments pour le thread
  struct args_AM args;

  if(init_param_AM(&args, NB_POINTS_COSIN, NB_POINTS_MOYENNE, FREQUENCE_GENEREE_MAX, VREF))
  {
    return EXIT_FAILURE;
  }

  // Initialisation du bcm2835
  if (!bcm2835_init())
  {
    //printf("Can't init bcm2835\n");
    return EXIT_FAILURE;
  }

  //printf("bcm2835 initiated\n");
  // Configuration du GPIO pour bouton - poussoir 1
  bcm2835_gpio_fsel(19, BCM2835_GPIO_FSEL_INPT);
  // Configuration du GPIO pour bouton - poussoir 2 (Ajouté)
  bcm2835_gpio_fsel(26, BCM2835_GPIO_FSEL_INPT);

  // Initialisation du GPIO en SPI
  if (!bcm2835_spi_begin())
  {
    //printf("can't init spi\n");
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

  // Création du thread "thread_choix_mode".
  // Lien avec la fonction traitement_signal.
  // Les paramêtres sont envoyé via args.
  if(pthread_create(&thread_choix_mode, NULL, &choix_mode, &args))
  {
    return EXIT_FAILURE;
  }

  // Création du thread "thread_signal".
  // Lien avec la fonction modulation_AM.
  // Les paramêtres sont envoyé via args.
  if (pthread_create(&thread_modulation, NULL, &modulation_AM, &args))
  {
    //printf("can't create thread\n");
    return EXIT_FAILURE;
  }

  // Boucle tant que le bouton - poussoir est non enfoncé
  while (bcm2835_gpio_lev(19))
  {
    // printf("button loop\n");
    usleep(1000); // Délai de 1 ms !!!
  }

  // Si bouton - poussoir enfoncé, arrêt immédiat du thread
  pthread_cancel(thread_modulation);
  pthread_cancel(thread_choix_mode);
  // Attente de l’arrêt du thread
  pthread_join(thread_modulation, NULL);
  pthread_join(thread_choix_mode, NULL);

  // Remettre la sortie du mcp4922 à 0
  voltage_mcp4922(0, args.Vref, 0, 0, 1, 0);

  // Libérer le SPI
  bcm2835_spi_end();

  // Libérer le GPIO
  bcm2835_close();

  // Libérer la mémoire utilisée pour la modulation et démodulation
  destroy_param_AM(&args);

  return EXIT_SUCCESS;
}