/*****************************************************************************************************************

    Fast Fourier Transfrom

    Author: Christoph Kirst (ckirst@nld.ds.mpg.de)
    Date:   2012, LMU Munich

 *****************************************************************************************************************/

//simple fft

#include "fft.h"
#include "debug.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>


/************************************************************************
  fft(int n, double xRe[], double xIm[], double yRe[], double yIm[])

  Input/output:
      int n          transformation length.
      double xRe[]   real part of input sequence.
      double xIm[]   imaginary part of input sequence.
      double yRe[]   real part of output sequence.
      double yIm[]   imaginary part of output sequence.
 ------------------------------------------------------------------------
  Function:
      The procedure performs a fast discrete Fourier transform (FFT) of
      a complex sequence, x, of an arbitrary length, n. The output, y,
      is also a complex sequence of length n.

      y[k] = sum(x[m]*exp(-i*2*pi*k*m/n), m=0..(n-1)), k=0,...,(n-1)

      The largest prime factor of n must be less than or equal to the
      constant maxPrimeFactor defined below.
 ------------------------------------------------------------------------
  Implementation notes:
      The general idea is to factor the length of the DFT, n, into
      factors that are efficiently handled by the routines.

      A number of short DFT's are implemented with a minimum of
      arithmetical operations and using (almost) straight line code
      resulting in very fast execution when the factors of n belong
      to this set. Especially radix-10 is optimized.

      Prime factors, that are not in the set of short DFT's are handled
      with direct evaluation of the DFP expression.

      Please report any problems to the author.
      Suggestions and improvements are welcomed.
 ------------------------------------------------------------------------
  Benchmarks:
      The Microsoft Visual C++ compiler was used with the following
      compile options:
      /nologo /Gs /G2 /W4 /AH /Ox /D "NDEBUG" /D "_DOS" /FR
      and the FFTBENCH test executed on a 50MHz 486DX :

      Length  Time [s]  Accuracy [dB]

         128   0.0054     -314.8
         256   0.0116     -309.8
         512   0.0251     -290.8
        1024   0.0567     -313.6
        2048   0.1203     -306.4
        4096   0.2600     -291.8
        8192   0.5800     -305.1
         100   0.0040     -278.5
         200   0.0099     -280.3
         500   0.0256     -278.5
        1000   0.0540     -278.5
        2000   0.1294     -280.6
        5000   0.3300     -278.4
       10000   0.7133     -278.5
 ------------------------------------------------------------------------
  The following procedures are used :
      factorize       :  factor the transformation length.
      transTableSetup :  setup table with sofar-, actual-, and remainRadix.
      permute         :  permutation allows in-place calculations.
      twiddleTransf   :  twiddle multiplications and DFT's for one stage.
      initTrig        :  initialise sine/cosine table.
      fft_4           :  length 4 DFT, a la Nussbaumer.
      fft_5           :  length 5 DFT, a la Nussbaumer.
      fft_10          :  length 10 DFT using prime factor FFT.
      fft_odd         :  length n DFT, n odd.
*************************************************************************/

#define  maxPrimeFactor        37
#define  maxPrimeFactorDiv2    (maxPrimeFactor+1)/2
#define  maxFactorCount        20

static double  c3_1 = -1.5000000000000E+00;  /*  c3_1 = cos(2*pi/3)-1;          */
static double  c3_2 =  8.6602540378444E-01;  /*  c3_2 = sin(2*pi/3);            */

static double  u5   =  1.2566370614359E+00;  /*  u5   = 2*pi/5;                 */
static double  c5_1 = -1.2500000000000E+00;  /*  c5_1 = (cos(u5)+cos(2*u5))/2-1;*/
static double  c5_2 =  5.5901699437495E-01;  /*  c5_2 = (cos(u5)-cos(2*u5))/2;  */
static double  c5_3 = -9.5105651629515E-01;  /*  c5_3 = -sin(u5);               */
static double  c5_4 = -1.5388417685876E+00;  /*  c5_4 = -(sin(u5)+sin(2*u5));   */
static double  c5_5 =  3.6327126400268E-01;  /*  c5_5 = (sin(u5)-sin(2*u5));    */
static double  c8   =  7.0710678118655E-01;  /*  c8 = 1/sqrt(2);    */

static double   pi;
static int      groupOffset,dataOffset,blockOffset,adr;
static int      groupNo,dataNo,blockNo,twNo;
static double   omega, tw_re,tw_im;
static double   twiddleRe[maxPrimeFactor], twiddleIm[maxPrimeFactor],
                trigRe[maxPrimeFactor], trigIm[maxPrimeFactor],
                zRe[maxPrimeFactor], zIm[maxPrimeFactor];
static double   vRe[maxPrimeFactorDiv2], vIm[maxPrimeFactorDiv2];
static double   wRe[maxPrimeFactorDiv2], wIm[maxPrimeFactorDiv2];

void factorize(int n, int *nFact, int fact[])
{
    int i,j,k;
    int nRadix;
    int radices[7];
    int factors[maxFactorCount];

    nRadix    =  6;
    radices[1]=  2;
    radices[2]=  3;
    radices[3]=  4;
    radices[4]=  5;
    radices[5]=  8;
    radices[6]= 10;

    if (n==1)
    {
        j=1;
        factors[1]=1;
    }
    else j=0;
    i=nRadix;
    while ((n>1) && (i>0))
    {
      if ((n % radices[i]) == 0)
      {
        n=n / radices[i];
        j=j+1;
        factors[j]=radices[i];
      }
      else  i=i-1;
    }
    if (factors[j] == 2)   /*substitute factors 2*8 with 4*4 */
    {
      i = j-1;
      while ((i>0) && (factors[i] != 8)) i--;
      if (i>0)
      {
        factors[j] = 4;
        factors[i] = 4;
      }
    }
    if (n>1)
    {
        for (k=2; k<sqrt(n)+1; k++)
            while ((n % k) == 0)
            {
                n=n / k;
                j=j+1;
                factors[j]=k;
            }
        if (n>1)
        {
            j=j+1;
            factors[j]=n;
        }
    }
    for (i=1; i<=j; i++)
    {
      fact[i] = factors[j-i+1];
    }
    *nFact=j;
}   /* factorize */

/****************************************************************************
  After N is factored the parameters that control the stages are generated.
  For each stage we have:
    sofar   : the product of the radices so far.
    actual  : the radix handled in this stage.
    remain  : the product of the remaining radices.
 ****************************************************************************/

void transTableSetup(int sofar[], int actual[], int remain[],
                     int *nFact,
                     int *nPoints)
{
    int i;

    factorize(*nPoints, nFact, actual);
    if (actual[*nFact] > maxPrimeFactor)
    {
        printf("\nPrime factor of FFT length too large : %6d",actual[*nFact]);
        exit(1);
    }
    remain[0]=*nPoints;
    sofar[1]=1;
    remain[1]=*nPoints / actual[1];
    for (i=2; i<=*nFact; i++)
    {
        sofar[i]=sofar[i-1]*actual[i-1];
        remain[i]=remain[i-1] / actual[i];
    }
}   /* transTableSetup */

/****************************************************************************
  The sequence y is the permuted input sequence x so that the following
  transformations can be performed in-place, and the final result is the
  normal order.
 ****************************************************************************/

void permute(int nPoint, int nFact,
             int fact[], int remain[],
             double xRe[], double xIm[],
             double yRe[], double yIm[])

{
    int i,j,k;
    int count[maxFactorCount];

    for (i=1; i<=nFact; i++) count[i]=0;
    k=0;
    for (i=0; i<=nPoint-2; i++)
    {
        yRe[i] = xRe[k];
        yIm[i] = xIm[k];
        j=1;
        k=k+remain[j];
        count[1] = count[1]+1;
        while (count[j] >= fact[j])
        {
            count[j]=0;
            k=k-remain[j-1]+remain[j+1];
            j=j+1;
            count[j]=count[j]+1;
        }
    }
    yRe[nPoint-1]=xRe[nPoint-1];
    yIm[nPoint-1]=xIm[nPoint-1];
}   /* permute */


/****************************************************************************
  Twiddle factor multiplications and transformations are performed on a
  group of data. The number of multiplications with 1 are reduced by skipping
  the twiddle multiplication of the first stage and of the first group of the
  following stages.
 ***************************************************************************/

void initTrig(int radix)
{
    int i;
    double w,xre,xim;

    w=2*pi/radix;
    trigRe[0]=1; trigIm[0]=0;
    xre=cos(w);
    xim=-sin(w);
    trigRe[1]=xre; trigIm[1]=xim;

    for (i=2; i<radix; i++)
    {
        trigRe[i]=xre*trigRe[i-1] - xim*trigIm[i-1];
        trigIm[i]=xim*trigRe[i-1] + xre*trigIm[i-1];
    }
}   /* initTrig */

void fft_4(double aRe[], double aIm[])
{
    double  t1_re,t1_im, t2_re,t2_im;
    double  m2_re,m2_im, m3_re,m3_im;

    t1_re=aRe[0] + aRe[2]; t1_im=aIm[0] + aIm[2];
    t2_re=aRe[1] + aRe[3]; t2_im=aIm[1] + aIm[3];

    m2_re=aRe[0] - aRe[2]; m2_im=aIm[0] - aIm[2];
    m3_re=aIm[1] - aIm[3]; m3_im=aRe[3] - aRe[1];

    aRe[0]=t1_re + t2_re; aIm[0]=t1_im + t2_im;
    aRe[2]=t1_re - t2_re; aIm[2]=t1_im - t2_im;
    aRe[1]=m2_re + m3_re; aIm[1]=m2_im + m3_im;
    aRe[3]=m2_re - m3_re; aIm[3]=m2_im - m3_im;
}   /* fft_4 */


void fft_5(double aRe[], double aIm[])
{
    double  t1_re,t1_im, t2_re,t2_im, t3_re,t3_im;
    double  t4_re,t4_im, t5_re,t5_im;
    double  m2_re,m2_im, m3_re,m3_im, m4_re,m4_im;
    double  m1_re,m1_im, m5_re,m5_im;
    double  s1_re,s1_im, s2_re,s2_im, s3_re,s3_im;
    double  s4_re,s4_im, s5_re,s5_im;

    t1_re=aRe[1] + aRe[4]; t1_im=aIm[1] + aIm[4];
    t2_re=aRe[2] + aRe[3]; t2_im=aIm[2] + aIm[3];
    t3_re=aRe[1] - aRe[4]; t3_im=aIm[1] - aIm[4];
    t4_re=aRe[3] - aRe[2]; t4_im=aIm[3] - aIm[2];
    t5_re=t1_re + t2_re; t5_im=t1_im + t2_im;
    aRe[0]=aRe[0] + t5_re; aIm[0]=aIm[0] + t5_im;
    m1_re=c5_1*t5_re; m1_im=c5_1*t5_im;
    m2_re=c5_2*(t1_re - t2_re); m2_im=c5_2*(t1_im - t2_im);

    m3_re=-c5_3*(t3_im + t4_im); m3_im=c5_3*(t3_re + t4_re);
    m4_re=-c5_4*t4_im; m4_im=c5_4*t4_re;
    m5_re=-c5_5*t3_im; m5_im=c5_5*t3_re;

    s3_re=m3_re - m4_re; s3_im=m3_im - m4_im;
    s5_re=m3_re + m5_re; s5_im=m3_im + m5_im;
    s1_re=aRe[0] + m1_re; s1_im=aIm[0] + m1_im;
    s2_re=s1_re + m2_re; s2_im=s1_im + m2_im;
    s4_re=s1_re - m2_re; s4_im=s1_im - m2_im;

    aRe[1]=s2_re + s3_re; aIm[1]=s2_im + s3_im;
    aRe[2]=s4_re + s5_re; aIm[2]=s4_im + s5_im;
    aRe[3]=s4_re - s5_re; aIm[3]=s4_im - s5_im;
    aRe[4]=s2_re - s3_re; aIm[4]=s2_im - s3_im;
}   /* fft_5 */

void fft_8()
{
    double  aRe[4], aIm[4], bRe[4], bIm[4], gem;

    aRe[0] = zRe[0];    bRe[0] = zRe[1];
    aRe[1] = zRe[2];    bRe[1] = zRe[3];
    aRe[2] = zRe[4];    bRe[2] = zRe[5];
    aRe[3] = zRe[6];    bRe[3] = zRe[7];

    aIm[0] = zIm[0];    bIm[0] = zIm[1];
    aIm[1] = zIm[2];    bIm[1] = zIm[3];
    aIm[2] = zIm[4];    bIm[2] = zIm[5];
    aIm[3] = zIm[6];    bIm[3] = zIm[7];

    fft_4(aRe, aIm); fft_4(bRe, bIm);

    gem    = c8*(bRe[1] + bIm[1]);
    bIm[1] = c8*(bIm[1] - bRe[1]);
    bRe[1] = gem;
    gem    = bIm[2];
    bIm[2] =-bRe[2];
    bRe[2] = gem;
    gem    = c8*(bIm[3] - bRe[3]);
    bIm[3] =-c8*(bRe[3] + bIm[3]);
    bRe[3] = gem;

    zRe[0] = aRe[0] + bRe[0]; zRe[4] = aRe[0] - bRe[0];
    zRe[1] = aRe[1] + bRe[1]; zRe[5] = aRe[1] - bRe[1];
    zRe[2] = aRe[2] + bRe[2]; zRe[6] = aRe[2] - bRe[2];
    zRe[3] = aRe[3] + bRe[3]; zRe[7] = aRe[3] - bRe[3];

    zIm[0] = aIm[0] + bIm[0]; zIm[4] = aIm[0] - bIm[0];
    zIm[1] = aIm[1] + bIm[1]; zIm[5] = aIm[1] - bIm[1];
    zIm[2] = aIm[2] + bIm[2]; zIm[6] = aIm[2] - bIm[2];
    zIm[3] = aIm[3] + bIm[3]; zIm[7] = aIm[3] - bIm[3];
}   /* fft_8 */

void fft_10()
{
    double  aRe[5], aIm[5], bRe[5], bIm[5];

    aRe[0] = zRe[0];    bRe[0] = zRe[5];
    aRe[1] = zRe[2];    bRe[1] = zRe[7];
    aRe[2] = zRe[4];    bRe[2] = zRe[9];
    aRe[3] = zRe[6];    bRe[3] = zRe[1];
    aRe[4] = zRe[8];    bRe[4] = zRe[3];

    aIm[0] = zIm[0];    bIm[0] = zIm[5];
    aIm[1] = zIm[2];    bIm[1] = zIm[7];
    aIm[2] = zIm[4];    bIm[2] = zIm[9];
    aIm[3] = zIm[6];    bIm[3] = zIm[1];
    aIm[4] = zIm[8];    bIm[4] = zIm[3];

    fft_5(aRe, aIm); fft_5(bRe, bIm);

    zRe[0] = aRe[0] + bRe[0]; zRe[5] = aRe[0] - bRe[0];
    zRe[6] = aRe[1] + bRe[1]; zRe[1] = aRe[1] - bRe[1];
    zRe[2] = aRe[2] + bRe[2]; zRe[7] = aRe[2] - bRe[2];
    zRe[8] = aRe[3] + bRe[3]; zRe[3] = aRe[3] - bRe[3];
    zRe[4] = aRe[4] + bRe[4]; zRe[9] = aRe[4] - bRe[4];

    zIm[0] = aIm[0] + bIm[0]; zIm[5] = aIm[0] - bIm[0];
    zIm[6] = aIm[1] + bIm[1]; zIm[1] = aIm[1] - bIm[1];
    zIm[2] = aIm[2] + bIm[2]; zIm[7] = aIm[2] - bIm[2];
    zIm[8] = aIm[3] + bIm[3]; zIm[3] = aIm[3] - bIm[3];
    zIm[4] = aIm[4] + bIm[4]; zIm[9] = aIm[4] - bIm[4];
}   /* fft_10 */

void fft_odd(int radix)
{
    double  rere, reim, imre, imim;
    int     i,j,k,n,max;

    n = radix;
    max = (n + 1)/2;
    for (j=1; j < max; j++)
    {
      vRe[j] = zRe[j] + zRe[n-j];
      vIm[j] = zIm[j] - zIm[n-j];
      wRe[j] = zRe[j] - zRe[n-j];
      wIm[j] = zIm[j] + zIm[n-j];
    }

    for (j=1; j < max; j++)
    {
        zRe[j]=zRe[0];
        zIm[j]=zIm[0];
        zRe[n-j]=zRe[0];
        zIm[n-j]=zIm[0];
        k=j;
        for (i=1; i < max; i++)
        {
            rere = trigRe[k] * vRe[i];
            imim = trigIm[k] * vIm[i];
            reim = trigRe[k] * wIm[i];
            imre = trigIm[k] * wRe[i];

            zRe[n-j] += rere + imim;
            zIm[n-j] += reim - imre;
            zRe[j]   += rere - imim;
            zIm[j]   += reim + imre;

            k = k + j;
            if (k >= n)  k = k - n;
        }
    }
    for (j=1; j < max; j++)
    {
        zRe[0]=zRe[0] + vRe[j];
        zIm[0]=zIm[0] + wIm[j];
    }
}   /* fft_odd */


void twiddleTransf(int sofarRadix, int radix, int remainRadix,
                    double yRe[], double yIm[])

{   /* twiddleTransf */
    double  cosw, sinw, gem;
    double  t1_re,t1_im, t2_re,t2_im, t3_re,t3_im;
    double  t4_re,t4_im, t5_re,t5_im;
    double  m2_re,m2_im, m3_re,m3_im, m4_re,m4_im;
    double  m1_re,m1_im, m5_re,m5_im;
    double  s1_re,s1_im, s2_re,s2_im, s3_re,s3_im;
    double  s4_re,s4_im, s5_re,s5_im;


    initTrig(radix);
    omega = 2*pi/(double)(sofarRadix*radix);
    cosw =  cos(omega);
    sinw = -sin(omega);
    tw_re = 1.0;
    tw_im = 0;
    dataOffset=0;
    groupOffset=dataOffset;
    adr=groupOffset;

    for (dataNo=0; dataNo<sofarRadix; dataNo++)
    {
        if (sofarRadix>1)
        {
            twiddleRe[0] = 1.0;
            twiddleIm[0] = 0.0;
            twiddleRe[1] = tw_re;
            twiddleIm[1] = tw_im;
            for (twNo=2; twNo<radix; twNo++)
            {
                twiddleRe[twNo]=tw_re*twiddleRe[twNo-1]
                               - tw_im*twiddleIm[twNo-1];
                twiddleIm[twNo]=tw_im*twiddleRe[twNo-1]
                               + tw_re*twiddleIm[twNo-1];
            }
            gem   = cosw*tw_re - sinw*tw_im;
            tw_im = sinw*tw_re + cosw*tw_im;
            tw_re = gem;
        }
        for (groupNo=0; groupNo<remainRadix; groupNo++)
        {
            if ((sofarRadix>1) && (dataNo > 0))
            {
                zRe[0]=yRe[adr];
                zIm[0]=yIm[adr];
                blockNo=1;
                do {
                    adr = adr + sofarRadix;
                    zRe[blockNo]=  twiddleRe[blockNo] * yRe[adr]
                                 - twiddleIm[blockNo] * yIm[adr];
                    zIm[blockNo]=  twiddleRe[blockNo] * yIm[adr]
                                 + twiddleIm[blockNo] * yRe[adr];

                    blockNo++;
                } while (blockNo < radix);
            }
            else
                for (blockNo=0; blockNo<radix; blockNo++)
                {
                   zRe[blockNo]=yRe[adr];
                   zIm[blockNo]=yIm[adr];
                   adr=adr+sofarRadix;
                }
            switch(radix) {
              case  2  : gem=zRe[0] + zRe[1];
                         zRe[1]=zRe[0] -  zRe[1]; zRe[0]=gem;
                         gem=zIm[0] + zIm[1];
                         zIm[1]=zIm[0] - zIm[1]; zIm[0]=gem;
                         break;
              case  3  : t1_re=zRe[1] + zRe[2]; t1_im=zIm[1] + zIm[2];
                         zRe[0]=zRe[0] + t1_re; zIm[0]=zIm[0] + t1_im;
                         m1_re=c3_1*t1_re; m1_im=c3_1*t1_im;
                         m2_re=c3_2*(zIm[1] - zIm[2]);
                         m2_im=c3_2*(zRe[2] -  zRe[1]);
                         s1_re=zRe[0] + m1_re; s1_im=zIm[0] + m1_im;
                         zRe[1]=s1_re + m2_re; zIm[1]=s1_im + m2_im;
                         zRe[2]=s1_re - m2_re; zIm[2]=s1_im - m2_im;
                         break;
              case  4  : t1_re=zRe[0] + zRe[2]; t1_im=zIm[0] + zIm[2];
                         t2_re=zRe[1] + zRe[3]; t2_im=zIm[1] + zIm[3];

                         m2_re=zRe[0] - zRe[2]; m2_im=zIm[0] - zIm[2];
                         m3_re=zIm[1] - zIm[3]; m3_im=zRe[3] - zRe[1];

                         zRe[0]=t1_re + t2_re; zIm[0]=t1_im + t2_im;
                         zRe[2]=t1_re - t2_re; zIm[2]=t1_im - t2_im;
                         zRe[1]=m2_re + m3_re; zIm[1]=m2_im + m3_im;
                         zRe[3]=m2_re - m3_re; zIm[3]=m2_im - m3_im;
                         break;
              case  5  : t1_re=zRe[1] + zRe[4]; t1_im=zIm[1] + zIm[4];
                         t2_re=zRe[2] + zRe[3]; t2_im=zIm[2] + zIm[3];
                         t3_re=zRe[1] - zRe[4]; t3_im=zIm[1] - zIm[4];
                         t4_re=zRe[3] - zRe[2]; t4_im=zIm[3] - zIm[2];
                         t5_re=t1_re + t2_re; t5_im=t1_im + t2_im;
                         zRe[0]=zRe[0] + t5_re; zIm[0]=zIm[0] + t5_im;
                         m1_re=c5_1*t5_re; m1_im=c5_1*t5_im;
                         m2_re=c5_2*(t1_re - t2_re);
                         m2_im=c5_2*(t1_im - t2_im);

                         m3_re=-c5_3*(t3_im + t4_im);
                         m3_im=c5_3*(t3_re + t4_re);
                         m4_re=-c5_4*t4_im; m4_im=c5_4*t4_re;
                         m5_re=-c5_5*t3_im; m5_im=c5_5*t3_re;

                         s3_re=m3_re - m4_re; s3_im=m3_im - m4_im;
                         s5_re=m3_re + m5_re; s5_im=m3_im + m5_im;
                         s1_re=zRe[0] + m1_re; s1_im=zIm[0] + m1_im;
                         s2_re=s1_re + m2_re; s2_im=s1_im + m2_im;
                         s4_re=s1_re - m2_re; s4_im=s1_im - m2_im;

                         zRe[1]=s2_re + s3_re; zIm[1]=s2_im + s3_im;
                         zRe[2]=s4_re + s5_re; zIm[2]=s4_im + s5_im;
                         zRe[3]=s4_re - s5_re; zIm[3]=s4_im - s5_im;
                         zRe[4]=s2_re - s3_re; zIm[4]=s2_im - s3_im;
                         break;
              case  8  : fft_8(); break;
              case 10  : fft_10(); break;
              default  : fft_odd(radix); break;
            }
            adr=groupOffset;
            for (blockNo=0; blockNo<radix; blockNo++)
            {
                yRe[adr]=zRe[blockNo]; yIm[adr]=zIm[blockNo];
                adr=adr+sofarRadix;
            }
            groupOffset=groupOffset+sofarRadix*radix;
            adr=groupOffset;
        }
        dataOffset=dataOffset+1;
        groupOffset=dataOffset;
        adr=groupOffset;
    }
}   /* twiddleTransf */


void fft(int n, double xRe[], double xIm[],
                double yRe[], double yIm[])
{
    int   sofarRadix[maxFactorCount],
          actualRadix[maxFactorCount],
          remainRadix[maxFactorCount];
    int   nFactor;
    int   count;

    pi = 4*atan(1);

    transTableSetup(sofarRadix, actualRadix, remainRadix, &nFactor, &n);
    permute(n, nFactor, actualRadix, remainRadix, xRe, xIm, yRe, yIm);

    for (count=1; count<=nFactor; count++)
      twiddleTransf(sofarRadix[count], actualRadix[count], remainRadix[count],
                    yRe, yIm);

}   /* fft */



/*************************************************************************************************

 simple algorith to find a good fft size which speeds up the transfrom extremely !

**************************************************************************************************/


// 2^a 3^b 5^c  which are smaller than 10^10
const int n_fft_nice_numbers = 767;
const int fft_nice_numbers[n_fft_nice_numbers] = {1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 15, 16, 18, 20, 24, 25, 27, 30, 32, \
                             36, 40, 45, 48, 50, 54, 60, 64, 72, 75, 80, 81, 90, 96, 100, 108, \
                             120, 125, 128, 135, 144, 150, 160, 162, 180, 192, 200, 216, 225, 240, \
                             243, 250, 256, 270, 288, 300, 320, 324, 360, 375, 384, 400, 405, 432, \
                             450, 480, 486, 500, 512, 540, 576, 600, 625, 640, 648, 675, 720, 729, \
                             750, 768, 800, 810, 864, 900, 960, 972, 1000, 1024, 1080, 1125, 1152, \
                             1200, 1215, 1250, 1280, 1296, 1350, 1440, 1458, 1500, 1536, 1600, \
                             1620, 1728, 1800, 1875, 1920, 1944, 2000, 2025, 2048, 2160, 2187, \
                             2250, 2304, 2400, 2430, 2500, 2560, 2592, 2700, 2880, 2916, 3000, \
                             3072, 3125, 3200, 3240, 3375, 3456, 3600, 3645, 3750, 3840, 3888, \
                             4000, 4050, 4096, 4320, 4374, 4500, 4608, 4800, 4860, 5000, 5120, \
                             5184, 5400, 5625, 5760, 5832, 6000, 6075, 6144, 6250, 6400, 6480, \
                             6561, 6750, 6912, 7200, 7290, 7500, 7680, 7776, 8000, 8100, 8192, \
                             8640, 8748, 9000, 9216, 9375, 9600, 9720, 10000, 10125, 10240, 10368, \
                             10800, 10935, 11250, 11520, 11664, 12000, 12150, 12288, 12500, 12800, \
                             12960, 13122, 13500, 13824, 14400, 14580, 15000, 15360, 15552, 15625, \
                             16000, 16200, 16384, 16875, 17280, 17496, 18000, 18225, 18432, 18750, \
                             19200, 19440, 19683, 20000, 20250, 20480, 20736, 21600, 21870, 22500, \
                             23040, 23328, 24000, 24300, 24576, 25000, 25600, 25920, 26244, 27000, \
                             27648, 28125, 28800, 29160, 30000, 30375, 30720, 31104, 31250, 32000, \
                             32400, 32768, 32805, 33750, 34560, 34992, 36000, 36450, 36864, 37500, \
                             38400, 38880, 39366, 40000, 40500, 40960, 41472, 43200, 43740, 45000, \
                             46080, 46656, 46875, 48000, 48600, 49152, 50000, 50625, 51200, 51840, \
                             52488, 54000, 54675, 55296, 56250, 57600, 58320, 59049, 60000, 60750, \
                             61440, 62208, 62500, 64000, 64800, 65536, 65610, 67500, 69120, 69984, \
                             72000, 72900, 73728, 75000, 76800, 77760, 78125, 78732, 80000, 81000, \
                             81920, 82944, 84375, 86400, 87480, 90000, 91125, 92160, 93312, 93750, \
                             96000, 97200, 98304, 98415, 100000, 101250, 102400, 103680, 104976, \
                             108000, 109350, 110592, 112500, 115200, 116640, 118098, 120000, \
                             121500, 122880, 124416, 125000, 128000, 129600, 131072, 131220, \
                             135000, 138240, 139968, 140625, 144000, 145800, 147456, 150000, \
                             151875, 153600, 155520, 156250, 157464, 160000, 162000, 163840, \
                             164025, 165888, 168750, 172800, 174960, 177147, 180000, 182250, \
                             184320, 186624, 187500, 192000, 194400, 196608, 196830, 200000, \
                             202500, 204800, 207360, 209952, 216000, 218700, 221184, 225000, \
                             230400, 233280, 234375, 236196, 240000, 243000, 245760, 248832, \
                             250000, 253125, 256000, 259200, 262144, 262440, 270000, 273375, \
                             276480, 279936, 281250, 288000, 291600, 294912, 295245, 300000, \
                             303750, 307200, 311040, 312500, 314928, 320000, 324000, 327680, \
                             328050, 331776, 337500, 345600, 349920, 354294, 360000, 364500, \
                             368640, 373248, 375000, 384000, 388800, 390625, 393216, 393660, \
                             400000, 405000, 409600, 414720, 419904, 421875, 432000, 437400, \
                             442368, 450000, 455625, 460800, 466560, 468750, 472392, 480000, \
                             486000, 491520, 492075, 497664, 500000, 506250, 512000, 518400, \
                             524288, 524880, 531441, 540000, 546750, 552960, 559872, 562500, \
                             576000, 583200, 589824, 590490, 600000, 607500, 614400, 622080, \
                             625000, 629856, 640000, 648000, 655360, 656100, 663552, 675000, \
                             691200, 699840, 703125, 708588, 720000, 729000, 737280, 746496, \
                             750000, 759375, 768000, 777600, 781250, 786432, 787320, 800000, \
                             810000, 819200, 820125, 829440, 839808, 843750, 864000, 874800, \
                             884736, 885735, 900000, 911250, 921600, 933120, 937500, 944784, \
                             960000, 972000, 983040, 984150, 995328, 1000000, 1012500, 1024000, \
                             1036800, 1048576, 1049760, 1062882, 1080000, 1093500, 1105920, \
                             1119744, 1125000, 1152000, 1166400, 1171875, 1179648, 1180980, \
                             1200000, 1215000, 1228800, 1244160, 1250000, 1259712, 1265625, \
                             1280000, 1296000, 1310720, 1312200, 1327104, 1350000, 1366875, \
                             1382400, 1399680, 1406250, 1417176, 1440000, 1458000, 1474560, \
                             1476225, 1492992, 1500000, 1518750, 1536000, 1555200, 1562500, \
                             1572864, 1574640, 1594323, 1600000, 1620000, 1638400, 1640250, \
                             1658880, 1679616, 1687500, 1728000, 1749600, 1769472, 1771470, \
                             1800000, 1822500, 1843200, 1866240, 1875000, 1889568, 1920000, \
                             1944000, 1953125, 1966080, 1968300, 1990656, 2000000, 2025000, \
                             2048000, 2073600, 2097152, 2099520, 2109375, 2125764, 2160000, \
                             2187000, 2211840, 2239488, 2250000, 2278125, 2304000, 2332800, \
                             2343750, 2359296, 2361960, 2400000, 2430000, 2457600, 2460375, \
                             2488320, 2500000, 2519424, 2531250, 2560000, 2592000, 2621440, \
                             2624400, 2654208, 2657205, 2700000, 2733750, 2764800, 2799360, \
                             2812500, 2834352, 2880000, 2916000, 2949120, 2952450, 2985984, \
                             3000000, 3037500, 3072000, 3110400, 3125000, 3145728, 3149280, \
                             3188646, 3200000, 3240000, 3276800, 3280500, 3317760, 3359232, \
                             3375000, 3456000, 3499200, 3515625, 3538944, 3542940, 3600000, \
                             3645000, 3686400, 3732480, 3750000, 3779136, 3796875, 3840000, \
                             3888000, 3906250, 3932160, 3936600, 3981312, 4000000, 4050000, \
                             4096000, 4100625, 4147200, 4194304, 4199040, 4218750, 4251528, \
                             4320000, 4374000, 4423680, 4428675, 4478976, 4500000, 4556250, \
                             4608000, 4665600, 4687500, 4718592, 4723920, 4782969, 4800000, \
                             4860000, 4915200, 4920750, 4976640, 5000000, 5038848, 5062500, \
                             5120000, 5184000, 5242880, 5248800, 5308416, 5314410, 5400000, \
                             5467500, 5529600, 5598720, 5625000, 5668704, 5760000, 5832000, \
                             5859375, 5898240, 5904900, 5971968, 6000000, 6075000, 6144000, \
                             6220800, 6250000, 6291456, 6298560, 6328125, 6377292, 6400000, \
                             6480000, 6553600, 6561000, 6635520, 6718464, 6750000, 6834375, \
                             6912000, 6998400, 7031250, 7077888, 7085880, 7200000, 7290000, \
                             7372800, 7381125, 7464960, 7500000, 7558272, 7593750, 7680000, \
                             7776000, 7812500, 7864320, 7873200, 7962624, 7971615, 8000000, \
                             8100000, 8192000, 8201250, 8294400, 8388608, 8398080, 8437500, \
                             8503056, 8640000, 8748000, 8847360, 8857350, 8957952, 9000000, \
                             9112500, 9216000, 9331200, 9375000, 9437184, 9447840, 9565938, \
                             9600000, 9720000, 9765625, 9830400, 9841500, 9953280};



int find_good_fft_size(int n) {

    //goal is to reduce n so that fftw will be working and not to slow

    //small input
    if (n<=1) return 1;

    //close to one of the nice numbers below ??
    int i = 1;
    while (i < n_fft_nice_numbers && n > fft_nice_numbers[i]) {
        i++;
    }

    //close to a nice number
    if (i < n_fft_nice_numbers) {
        if ((n-fft_nice_numbers[i-1]) < (fft_nice_numbers[i]-n)) {
            return fft_nice_numbers[i-1];
        } else {
            return fft_nice_numbers[i];
        }
    }

    //larger than stored nice numbers

    int factors[3] = {2, 3, 5};
    int mult[3] = {0,0,0};
    const int nfactors = 3;
    int k = n;

    //divide by all factors
    for (int i =0; i < nfactors; i++) {
        while ((k % factors[i]) == 0) {
            k=k/factors[i];
            mult[i]++;
        }
    }
    if (k==1) return n;

    //now find closest power of 2
    int p2 = 1;
    while (p2 < k) p2*=2;

    k = p2/2;
    for (int i =0; i < nfactors; i++) {
        for (int j = 0; j < mult[i]; j++) {
            k*=factors[i];
        }
    }

    if ((n-k) < (k*2-n)) {
            return k;
    } else {
            return k*2;
    }
}


//simple algorith to find a good fft size
int find_good_smaller_fft_size(int n) {

    //goal is to reduce n so that fftw will be working and not to slow

    //small input
    if (n<=1) return 1;

    //close to one of the nice numbers below ??
    int i = 1;
    while (i < n_fft_nice_numbers && n > fft_nice_numbers[i]) {
        i++;
    }

    //close to a nice number
    if (i < n_fft_nice_numbers) return fft_nice_numbers[i-1];


    //larger than stored nice numbers
    int factors[3] = {2, 3, 5};
    int mult[3] = {0,0,0};
    const int nfactors = 3;
    int k = n;

    //divide by all factors
    for (int i =0; i < nfactors; i++) {
        while ((k % factors[i]) == 0) {
            k=k/factors[i];
            mult[i]++;
        }
    }
    if (k==1) return n;

    //now find closest power of 2
    int p2 = 1;
    while (p2 < k) p2*=2;

    k = p2/2;
    for (int i =0; i < nfactors; i++) {
        for (int j = 0; j < mult[i]; j++) {
            k*=factors[i];
        }
    }
    return k;
}


//simple algorith to find a good fft size
int find_good_larger_fft_size(int n) {

    //goal is to reduce n so that fftw will be working and not to slow

    //small input
    if (n<=1) return 1;

    //close to one of the nice numbers below ??
    int i = 1;
    while (i < n_fft_nice_numbers && n > fft_nice_numbers[i]) {
        i++;

    }
    //DEBUG(QString("n=%1, i=%2,  number=%3").arg(n).arg(i).arg(fft_nice_numbers[i]).toStdString())


    //close to a nice number
    if (i < n_fft_nice_numbers) return fft_nice_numbers[i];


    //larger than stored nice numbers
    int factors[3] = {2, 3, 5};
    int mult[3] = {0,0,0};
    const int nfactors = 3;
    int k = n;

    //divide by all factors
    for (int i =0; i < nfactors; i++) {
        while ((k % factors[i]) == 0) {
            k=k/factors[i];
            mult[i]++;
        }
    }
    if (k==1) return n;

    //now find closest power of 2
    int p2 = 1;
    while (p2 < k) p2*=2;

    k = p2;
    for (int i =0; i < nfactors; i++) {
        for (int j = 0; j < mult[i]; j++) {
            k*=factors[i];
        }
    }
    return k;
}

