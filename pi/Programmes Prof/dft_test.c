/*
 * complexe01.c
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define N 16384

typedef struct compl {
    double re;
    double im;
} compl;

compl twid[N];
compl signal[N];

compl add(compl, compl);
compl sub(compl, compl);
compl mul(compl, compl);
compl mulAlpha(double, compl);
compl divi(compl, compl);
compl conju(compl);
compl expIx(double);
double ampl(compl);
void dft(compl *, compl *, int, double);
void fourier_shift(compl *, int, double); 
compl initsignal(double);

double frequence = 1000.0; //Hz

double lecture_CAN(int canal)
{
	char buf[3] = {0};			// Buffer de la trame à envoyer
	int a = 0;				// Buffer de la trame reçue
	
	// Selectionner le CAN
	bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
	buf[0] = 0x06; 
	buf[2] = 0x00;
	switch(canal)
	{
		case 0: 			// Canal 0 broche 1 (oscilloscope)
			buf[1] = 0x00; break; 
		case 1:				// Canal 1 broche 2
			buf[1] = 0x40; break; 
		case 2:				// Canal 2 broche 3
			buf[1] = 0x80; break;
		case 3:				// Canal 3 broche 4
			buf[1] = 0xC0; break; 
	    default:
			buf[1] = 0x00; break; // Canal 0 broche 1 (défault)
	}
	
	bcm2835_spi_transfern(buf,3);
	
	a = buf[2] + 256*buf[1];
	return (double) a*5.0/4096.0;
}

void *clignote()
{
	clock_t debut, fin;
	double demiPer = 1.0/frequence;
	double mesure;
	int k=0;
	
	debut = clock();
	
	while(k<N){
		mesure = lecture_CAN(0)-2.5;
		signal[k++] = initsignal(mesure);
		fin = clock();
		
		usleep(1000000*(demiPer-((double) (fin-debut)/((double) CLOCKS_PER_SEC))));
		
		debut = clock();
	}
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	compl fft[N];
	int k;
	clock_t debut, fin ;
	
	FILE *fichier = NULL ; // Déclaration de la variable fichier
	
	pthread_t cligne;
	
	if (!bcm2835_init()){
		return 1;
	}
	
	if (!bcm2835_spi_begin()){
		printf("Echec d'initialisation du SPI bcm2835...\n");
		return 1;
	}
	
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
	bcm2835_spi_set_speed_hz(200000);
	bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
	bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW);
	bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);
	
	bcm2835_gpio_fsel(20,BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(21,BCM2835_GPIO_FSEL_OUTP);

	fichier = fopen ("TEST.txt","w"); // Ouverture du fichier
	
	bcm2835_gpio_write(20,HIGH);
	pthread_create(&cligne, NULL, &clignote, NULL);
	
	pthread_join(cligne, NULL);
	bcm2835_gpio_write(20,LOW);
	bcm2835_gpio_write(21,HIGH);
	for(k=0;k<N;k++)
	{
		twid[k] = expIx(-2*M_PI/((double) N)*k);
	}
	debut = clock();
	dft(signal, fft, N, 1.0);
	fin = clock();
	printf("%f\n",((double) (fin-debut))/((double) CLOCKS_PER_SEC));
	for(k=0;k<N;k++)
		fprintf(fichier,"%f\n",ampl(fft[k]));
	fclose(fichier);
	fichier = fopen ("MESURE.txt","w"); // Ouverture du fichier
	for(k=0;k<N;k++)
		fprintf(fichier,"%f\n",signal[k].re);
	fclose(fichier);
	
	bcm2835_gpio_write(21,LOW);
	bcm2835_spi_end();
	bcm2835_close();
	return 0;
}

compl initsignal(double x)
{
	compl y;
	y.re = x;
	y.im = 0;
	return y;
}

compl add(compl x, compl y)
{
	compl temp;
	temp.re = x.re + y.re;
	temp.im = x.im + y.im;
	return temp;
}

compl sub(compl x, compl y)
{
	compl temp;
	y.re = -y.re;
	y.im = -y.im;
	temp=add(x,y);
	return temp;
}

compl mul(compl x, compl y)
{
	compl temp;
	temp.re = x.re*y.re - x.im*y.im;
	temp.im = x.im*y.re + x.re*y.im;
	return temp;
}

compl mulAlpha(double alpha, compl x)
{
	compl temp;
	temp.re = alpha*x.re;
	temp.im = alpha*x.im;
	return temp;
}

compl divi(compl x, compl y)
{
	compl temp;
	double tempo = (y.re*y.re + y.im*y.im);
	temp = mul(x, conju(y));
	temp.re = temp.re/tempo;
	temp.im = temp.im/tempo;
	return temp;
}

compl conju(compl x)
{
	compl temp;
	temp.re = x.re;
	temp.im = -x.im;
	return temp;
}

compl expIx(double x)
{
	compl tempo;
	tempo.re = cos(x);
	tempo.im = sin(x);
	return tempo;
}

double ampl(compl x)
{
	return sqrt(x.re*x.re+x.im*x.im);
}

/* Inspiré de : https://cnx.org/contents/0sbTkzWQ@2.2:zmcmahhR@7/Decimation-in-time-DIT-Radix-2-FFT
 * 
 */
void dft(compl *a, compl *b, int n, double is)
{
	compl c[n/2], d[n/2], s[n/2], t[n/2];
	int nh, k;
	
	if (n==1)
	{
		b[0] = a[0];
		return;
	}
	
	nh = n/2;
	
	for (k=0; k<nh; k++)
	{
		s[k] = a[2*k];
		t[k] = a[2*k+1];
	}
	
	dft(s, c, nh, is);
	dft(t, d, nh, is);
	
	fourier_shift(d, nh, is/2.0);
	
	for (k=0; k<nh; k++)
	{
		b[k] = add(c[k],d[k]);
		b[k+nh] = sub(c[k],d[k]);
	}
}

void fourier_shift(compl *a, int n, double v)
{
	int k;
	for(k=0;k<n;k++)
	{
		a[k] = mul(a[k], twid[(int) (v*N*k/n)]);
	}
}
