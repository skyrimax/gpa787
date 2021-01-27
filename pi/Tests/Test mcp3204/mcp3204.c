/*
 * mcp3204.c
 * 
 * Test du MCP3204 - Software SPI
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
#define MISO	9	// GPIO du MISO du Raspberry Pi
#define CS_3204 8	// GPIO du CS du Raspberry Pi

/* ***************************
 * lecture_3204:
 * 
 * Routine de lecture du canal 0 du MCP3204 (CAN)
 * 
 * Paramètre en entrée : Aucune
 * Paramètre en sortie : valeur entre 0 et 4095
 * 
 * N.B. dans les laboratoires #1 et #2, la valeur en sortie
 * devra être la tension mesurée en Volts (entre 0 et 5 V)
 * 
 * *************************** */
int lecture_3204()
{
	int adc =0;
	int k,j;
	const int para = 0x060000;
		
	// Début de la transaction de 24 bits
	bcm2835_gpio_write(CS_3204, LOW);
	bcm2835_delayMicroseconds(DELAI);
	
	// Transaction
	for(k=0;k<=23;k++){
		j = (para>>(23-k))&0x01;
		if (j==0) bcm2835_gpio_write(MOSI, LOW);
		else bcm2835_gpio_write(MOSI, HIGH);
		bcm2835_delayMicroseconds(DELAI);
		adc = (adc<<1)+bcm2835_gpio_lev(MISO);
		bcm2835_gpio_write(SCLK, HIGH);
		bcm2835_delayMicroseconds(DELAI);
		bcm2835_gpio_write(SCLK, LOW);
		
	}
	// Fin de transaction
	bcm2835_delayMicroseconds(DELAI);
	bcm2835_gpio_write(CS_3204, HIGH);
	
	return (adc&0x0FFF);
}

int main(int argc, char **argv)
{
	if (!bcm2835_init()){
		return 1;
	}
	// Définition des broches branchées au MCP3204
	
	bcm2835_gpio_fsel(SCLK, BCM2835_GPIO_FSEL_OUTP); // SLCK du SPI
	bcm2835_gpio_write(SCLK, LOW);
	bcm2835_gpio_fsel(MOSI, BCM2835_GPIO_FSEL_OUTP); // MOSI du SPI
	bcm2835_gpio_fsel(MISO, BCM2835_GPIO_FSEL_INPT); // MISO du SPI (non utilisé)
	bcm2835_gpio_fsel(CS_3204, BCM2835_GPIO_FSEL_OUTP);  // CS1 du SPI
	bcm2835_gpio_write(CS_3204, HIGH);
	
	while(1){
		printf("%d \n",lecture_3204());  // Lecture du MCP3304
		bcm2835_delay(1000); // Délai entre deux transferts de 1000 ms
	}
	
	bcm2835_close();
	return 0;
}

