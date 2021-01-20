/*
* Cligne_1_Hz.c
*
* Ce programme fait clignoter un DEL à une fréquence de 1 Hz.
* L’arrêt sera provoqué par un appui sur le bouton - poussoir 1.
* Aucun paramètre en entrée.
*
*/

#include <stdio.h>
#include <bcm2835.h>

int main( int argc, char **argv )
{
  // Initialisation du bcm2835
  if(!bcm2835_init()){
    return 1;
  }
   
  // Configuration du GPIO pour DEL 1 (rouge)
  bcm2835_gpio_fsel(20, BCM2835_GPIO_FSEL_OUTP);
  // COnfiguration du GPIO pour DEL 2 (orange) (Ajouté)
  bcm2835_gpio_fsel(21, BCM2835_GPIO_FSEL_OUTP);
  // Configuration du GPIO pour bouton - poussoir 1
  bcm2835_gpio_fsel(19, BCM2835_GPIO_FSEL_INPT);
  // Configuration du GPIO pour bouton - poussoir 2 (Ajouté)
  bcm2835_gpio_fsel(26, BCM2835_GPIO_FSEL_INPT);

  // Tant que bouton - poussoir #1 non actionné, clignoter
  // DEL allumée 500 ms et éteinte 500 ms
  while (bcm2835_gpio_lev(19) || bcm2835_gpio_lev(26)){
    // Allumer la DEL rouge (Déjà en place)
    bcm2835_gpio_write(20, HIGH);
    // Éteindre la DEL orange (Ajouté)
    bcm2835_gpio_write(21, LOW);

    // Attendre la moitié de la période
    bcm2835_delay(500);

    // Éteindre la DEL rouge (Déjà en place)
    bcm2835_gpio_write(20, LOW);
    // Allumer la DEL orange (Ajouté)
    bcm2835_gpio_write(21, HIGH);

    // Attendre la moitié de la période
    bcm2835_delay(500);
  }

  // Éteindre la DEL rouge (Ajouté)
  bcm2835_gpio_write(20, LOW);
  // Éteindre la DEL orange (Ajouté)
  bcm2835_gpio_write(21, LOW);

  // Libérer les broches du GPIO
  bcm2835_close();
  return 0;
}