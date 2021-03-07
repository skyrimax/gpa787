/*
 * complexe01.c
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define N 16384

typedef struct compl {
    double re;
    double im;
} compl;

compl add(compl, compl);
compl sub(compl, compl);
compl mul(compl, compl);
compl mulAlpha(double, compl);
compl divi(compl, compl);
compl conju(compl);
compl expIx(double);
double ampl(compl);


int main(int argc, char **argv)
{
	compl twid[N];
	double signal[N];
	compl fft[N];
	int k, l;
	clock_t debut, fin ;
	
	FILE *fichier = NULL ; // Déclaration de la variable fichier

	fichier = fopen ("fft3.txt","w"); // Ouverture du fichier
	
	for(k=0;k<N;k++)
	{
		twid[k] = expIx(-2*M_PI/((double) N)*k);
		if(k<N/2) signal[k] = 5.0;
		else signal[k] = -5.0;
		//signal[k] = sin(2*M_PI*0.25*10.0/256.0*k)+cos(2*M_PI*2.5*10.0/256.0*k);
	}
	debut = clock();
	for(k=0;k<N;k++)
	{
		fft[k].re = 0;
		fft[k].im = 0;
		for(l=0;l<N;l++)
		{
			fft[k] = add(fft[k],mulAlpha(signal[l],twid[(k*l)%N]));
		}
		//printf("%f\n",ampl(fft[k]));
		//fprintf(fichier,"%f\n",ampl(fft[k]));
	}
	fin = clock();
	printf("%f\n",((double) (fin-debut))/((double) CLOCKS_PER_SEC));
	for(k=0;k<N;k++)
		fprintf(fichier,"%f\n",ampl(fft[k]));
	fclose(fichier);
	return 0;
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
