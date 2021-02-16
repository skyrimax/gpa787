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

compl twid[N];

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

int main(int argc, char **argv)
{
	compl signal[N];
	double si[N]; // = {0,1,1,0,0,2,2,0,0,1,1,0,0,2,2,0,0,1,1,0,0,2,2,0,0,1,1,0,0,2,2,0};
	compl fft[N];
	int k;
	clock_t debut, fin ;
	
	FILE *fichier = NULL ; // Déclaration de la variable fichier

	fichier = fopen ("dft1.txt","w"); // Ouverture du fichier
	
	for(k=0;k<N;k++)
	{
		twid[k] = expIx(-2*M_PI/((double) N)*k);
		if(k<N/2) si[k] = 5.0;
		else si[k] = -5.0;
		//si[k] = sin(2*M_PI*0.25*10.0/256.0*k)+cos(2*M_PI*2.5*10.0/256.0*k);
		signal[k] = initsignal(si[k]);
	}
	debut = clock();
	dft(signal, fft, N, 1.0);
	fin = clock();
	printf("%f\n",((double) (fin-debut))/((double) CLOCKS_PER_SEC));
	for(k=0;k<N;k++)
		fprintf(fichier,"%f\n",ampl(fft[k]));
	fclose(fichier);
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
