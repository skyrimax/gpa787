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

#define NB_POINTS_COS 16 // Nombre de points utilisé pour discrétiser le cosin
#define FREQUENCE 1.0   // Hertz : fréquence du cosinus désirée
#define AMPLITUDE 4.0    // Voltage crête à crête de l'onde cosinusoïdale
#define VREF 5.0         // Voltage de référence du mcp4922

// Définir la structure des arguments pour la fonction cosinus
struct args_signal
{
  double frequence;
  double amplitude;
  double vRef;
  uint nbPoints;
  double *points_signal;
};

double *discret_ligne(int N)
{
  double *points = NULL;
  points = (double *)malloc(N * sizeof(double));

  if(!points){
    return NULL;
  }

  for(int i=0; i<N; ++i){
    points[i]=i/(N-1.0);
    //printf("points[%d] = %lf\n", i, points[i]);
  }

  return points;
}

/* Fonction calculant les point de la fonction cosinus et les retourne
  * dans un tableau de N doubles
  * 
  * N : nombre de points désiré pour la discrétisation du cosinus
  */
double *discret_cosinus(uint N)
{
  double *points = (double *)malloc(N * sizeof(double));

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

  uint16_t data = 4096 * v / Vref;

  if (canal)
    data |= 0b1000000000000000;

  if (buffered)
    data |= 0b0100000000000000;

  if (gain)
    data |= 0b0010000000000000;

  if (shutdown)
    data |= 0b0001000000000000;

  //printf("Data : %x\n", data&0xFFF);
  bcm2835_spi_transfern((char *)&data, 2);
}

/* Fonction clignote qui sera appelée par le thread " cligne "
*
* Elle fait clignoter le DEL rouge à 1 Hertz .
*
* La fonction ne retourne aucune valeur .
* La fonction n’exige aucun param ètre en entrée.
*/
void *signal_mcp4922(void *args)
{
  uint i = 0;
  clock_t debut, fin; // Variables de temps
  struct args_signal *args_s = (struct args_signal *)args;
  //printf("Frequency : %lf\n", args_s->frequence);
  //printf("Amplitude : %lf\n", args_s->amplitude);
  //printf("Vref : %lf\n", args_s->vRef);
  //printf("Nb points : %d\n\n", args_s->nbPoints);

  //for(i=0; i< args_s->nbPoints; ++i){
  //  printf("Point %d : %lf\n", i, args_s->points_signal[i]);
  //}

  double periode_echantillonnage = 1 / (args_s->nbPoints * args_s->frequence); // Calcul de la période entre chaque points
  //printf("Sampling period : %lf\n", periode_echantillonnage);

  debut = clock(); // Temps écoulé depuis le lancement du programme

  // Boucle infinie pour le signal périodique
  while (1)
  {
    //printf("i : %d\n", i);
    voltage_mcp4922(args_s->amplitude / 2 * args_s->points_signal[i] + args_s->vRef / 2, args_s->vRef, 0, 0, 1, 1);

    i = (i + 1) % args_s->nbPoints;

    fin = clock(); // Temps écoulé depuis le lancement du programme

    // La différence (fin - debut) donne le temps d’exécution du code
    // On cherche une durée égale à demiPer. On compense avec un délai.
    // CLOCK_PER_SEC est le nombre de coups d’horloges en 1 seconde

    usleep(1000000 * (periode_echantillonnage - ((double)(fin - debut) / ((double)CLOCKS_PER_SEC))));
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
  // Identificateur du thread
  pthread_t thread_signal;

  // Arguments pour le thread
  struct args_signal args;
  args.amplitude = AMPLITUDE;
  args.frequence = FREQUENCE;
  args.nbPoints = NB_POINTS_COS;
  args.points_signal = NULL;
  args.vRef = VREF;

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

  //printf("spi initiated\n");
  // Calculs des points du cosinus et assignation dans la variable globale
  //args.points_signal = discret_cosinus(args.nbPoints);
  args.points_signal = discret_cosinus(args.nbPoints);

  // S'assurer que les points sont bien assignées
  if (!args.points_signal)
  {
    //printf("can't create points\n");
    return EXIT_FAILURE;
  }

  // Configurer l'ordre de transmition et MSBF
  bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);

  // Configurer la vitesse de transmission
  bcm2835_spi_set_speed_hz(16384);

  // Configuration du mode SPI
  bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);

  // Configuer le chip select et son état d'Activation
  bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW);

  // Création du thread "cosin".
  // Lien avec la fonction clignote.
  // Cette dernière n’exige pas de paramètres.
  if (pthread_create(&thread_signal, NULL, &signal_mcp4922, &args))
  {
    //printf("can't create thread\n");
    return EXIT_FAILURE;
  }

  // Boucle tant que le bouton - poussoir est non enfoncé
  while (bcm2835_gpio_lev(19) || bcm2835_gpio_lev(26))
  {
    // printf("button loop\n");
    usleep(1000); // Délai de 1 ms !!!
  }

  // Si bouton - poussoir enfoncé, arrêt immédiat du thread
  pthread_cancel(thread_signal);
  // Attente de l’arrêt du thread
  pthread_join(thread_signal, NULL);

  // Remettre la sortie du mcp4922 à 0
  voltage_mcp4922(0, args.vRef, 0, 0, 1, 0);

  // Libérer le SPI
  bcm2835_spi_end();

  // Libérer le GPIO
  bcm2835_close();

  // Libérer la mémoire utilisée pour les points du cosinus
  free(args.points_signal);

  return 0;
}