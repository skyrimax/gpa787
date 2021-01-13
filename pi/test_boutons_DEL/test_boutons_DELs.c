/*
 * test_boutons_DELs.c
 * 
 * Copyright 2018  <pi@raspberrypi>
 * 
 */

#include <unistd.h>
#include <stdio.h>
#include <bcm2835.h>

int main(int argc, char **argv)
{
	if (!bcm2835_init()){
		return 1;
	}
	bcm2835_gpio_fsel(20, BCM2835_GPIO_FSEL_OUTP); // DEL rouge
	bcm2835_gpio_fsel(21, BCM2835_GPIO_FSEL_OUTP); // DEL bleu
	bcm2835_gpio_fsel(19, BCM2835_GPIO_FSEL_INPT); // Bouton #1
	bcm2835_gpio_fsel(26, BCM2835_GPIO_FSEL_INPT); // Bouton #2
	
	printf("Programme de test des boutons et des DELs:\n");
	printf("==========================================\n\n");
	printf("En appuyant sur un bouton seulement allume \n");
	printf("l'un ou l'autre des DELs.\n\n");
	printf("En appuyant sur les deux boutons en même temps,\n");
	printf("on stoppe le programme qui termine son exécution.\n");
	
	while(bcm2835_gpio_lev(19) || bcm2835_gpio_lev(26)){
		if (!bcm2835_gpio_lev(19)) bcm2835_gpio_write(20, HIGH);
		else bcm2835_gpio_write(20, LOW);
		if (!bcm2835_gpio_lev(26)) bcm2835_gpio_write(21, HIGH);
		else bcm2835_gpio_write(21, LOW);
		usleep(100);
	}
	
    bcm2835_gpio_write(20, LOW);
	bcm2835_gpio_write(21, LOW);
	bcm2835_close();
	return 0;
}
