/*
 * mcp4922.c
 * 
 * Test du MCP4922 - Software SPI
 * 
 * Dans les laboratoires, vous devrez utiliser le SPI hardware.
 * 
 * Janvier 2021 
 * 
 */

#include <stdio.h>
#include <bcm2835.h>

#define DELAI	10	// Délai (en us) dans les signaux du SPI
#define SCLK 	11  // GPIO du SCLK du Raspberry Pi
#define MOSI	10	// GPIO du MOSI du Raspberry Pi
#define MISO	9	// GPIO du MISO du Raspberry Pi (non utilisé)
#define CS_4922	7	// GPIO du CS du Raspberry Pi

/* ***************************
 * ecriture_4922:
 * 
 * Routine d'écriture au MCP4922 (CNA)
 * 
 * Paramètre en entrée : valeur numérique entre 0 et 4095
 * Ne retourne pas de valeur en sortie
 * 
 * N.B. dans les laboratoires #1 et #2, la valeur en entrée
 * devra être la tension désirée en Volts (entre 0 et 5 V)
 * 
 * *************************** */
void ecriture_4922(int dac)
{
	int k,j;
	
	if (dac>4095) dac=4095;
	if (dac<0) dac=0;
	
	dac = dac+0x03000; // Valeur + configuration CAN (Canal A)
	
	// Début de la transaction de 16 bits
	bcm2835_gpio_write(CS_4922, LOW);
	bcm2835_delayMicroseconds(DELAI);
	
	// Transaction
	for(k=0;k<=15;k++){
		j = (dac>>(15-k))&0x01;
		if (j==0) bcm2835_gpio_write(MOSI, LOW);
		else bcm2835_gpio_write(MOSI, HIGH);
		bcm2835_delayMicroseconds(DELAI);
		bcm2835_gpio_write(SCLK, HIGH);
		bcm2835_delayMicroseconds(DELAI);
		bcm2835_gpio_write(SCLK, LOW);
		
	}
	
	// Fin de transaction
	bcm2835_delayMicroseconds(DELAI);
	bcm2835_gpio_write(CS_4922, HIGH);
}

int main(int argc, char **argv)
{
	int x = 0;
	
	if (!bcm2835_init()){
		return 1;
	}
	// Définition des broches branchées au MCP4922
	
	bcm2835_gpio_fsel(SCLK, BCM2835_GPIO_FSEL_OUTP); // SLCK du SPI
	bcm2835_gpio_write(SCLK, LOW);
	bcm2835_gpio_fsel(MOSI, BCM2835_GPIO_FSEL_OUTP); // MOSI du SPI
	bcm2835_gpio_fsel(MISO, BCM2835_GPIO_FSEL_INPT); // MISO du SPI (non utilisé)
	bcm2835_gpio_fsel(CS_4922, BCM2835_GPIO_FSEL_OUTP);  // CS1 du SPI
	bcm2835_gpio_write(CS_4922, HIGH);
	
	while(1){
		ecriture_4922(x);  // Envoi de x au MAC4922
		x += 10;
		if (x>4095) x=0;
		
		bcm2835_delay(10); // Délai entre deux transferts de 10 ms
	}
	
	bcm2835_close();
	return 0;
}

