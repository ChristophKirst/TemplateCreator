/*****************************************************************************************************************

    Numerical Implementations of Data Analysis / Template Creation Functions

    Author: Christoph Kirst (ckirst@nld.ds.mpg.de)
    Date:   2012, LMU Munich

 *****************************************************************************************************************/

#include "numerics.h"

#include "debug.h"

#include "fft.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <math.h>

const double pi = 4*atan(1);

//#define REAL(z,i) ((z)[2*(i)])
//#define IMAG(z,i) ((z)[2*(i)+1])


/*****************************************************************************************************************
 *
 *      Template Numerics
 *
 *****************************************************************************************************************/

//standard manipulations: setting offset and left and right margins
void postprocess_template(DataTYPE samp, DataTYPE off, DataTYPE left, DataTYPE right,
                                      DataVECTOR& v) {

    DataTYPE dt = 1.0 /samp / 1000.0;

    // add offsets
    for (int i =0; i < v.size(); i++) {
        v[i] += off;
    }

    //add left and right margins
    if (left<= 0 && right <= 0) return;

    //resize:
    int nl = int(left / dt);
    int nr = int(right / dt);
    int n = v.size() + nl + nr;

    DataTYPE * vnew = new DataTYPE[n];
    for (int i = 0; i < nl; i++) { vnew[i] = off; }
    copy ( v.begin(), v.end(), vnew + nl);
    for (int i = v.size()+nl; i < n; i++) { vnew[i] = off; }

    v.resize(n);
    copy(vnew, vnew+n, v.begin());

    delete [] vnew;
}




/* create a zap stimulus of duration dur [sec], starting from freq f0 to f1 [Hz] with amplitude amp, and constant offset off, add constant stimulation of length left and right
 * assume sampling frequency of samp [kHz]
 */
bool create_zap(DataTYPE dur, DataTYPE samp,
                            DataTYPE f0, DataTYPE f1, DataTYPE amp, bool reverse,
                            DataVECTOR& v ) {

   /* zap stim is given by  sin(t ((f1-f0) t/dur + f0)) */
   /* here time is measured in secs */

   /* for sampling rate of samp kHz we have a dt = 1/samp / 1000 */


   DataTYPE dt = 1.0 / samp / 1000.0;
   int n = int(dur * samp * 1000) + 1;

   DataTYPE ph0 = 0;
   if (reverse) {
      ph0 = 2 * pi * (dur) * ((f1-f0) / 2.0 + f0);
   }



   v.resize(n);
   DataTYPE t = 0;
   for (int i = 0; i < n; i++) {
       if (reverse)
           v[i] = amp * sin(2 * pi * (dur - t) * ((f1-f0) * (dur -t) / dur / 2.0 + f0) + ph0);
       else
           v[i] = amp * sin(2 * pi * t * ((f1-f0) * (t / dur)/2.0 + f0));
       t+=dt;
   }

   return true;
}



/* create a noise stimulus of duration dur [sec] with uniform frequency spectrum from f0 to f1 [Hz] with standard deviation sigma add constant stimulation offset off
 * add LFP like signal underneath with amplitude amp / phase and freqeuncy omega [Hz]
 * assume sampling frequency of samp [kHz]
 */
bool create_noise(DataTYPE dur, DataTYPE samp,
                              DataTYPE ff,  DataTYPE phase, DataTYPE amp,
                              DataTYPE f0, DataTYPE f1, DataTYPE sigma, int seed,
                              DataVECTOR& v) {
    DEBUG("create_noise")

    //working own fft.h version with optimized sample size

    srand(seed);

    DataTYPE dt = 1.0 /samp / 1000.0;
    int n_final = floor(dur/dt);
    DEBUG(QString("fft dt= %1  n_final =%2, dur= %3").arg(dt).arg(n_final).arg(dur).toStdString())

    if (n_final<1) n_final=1;
    if (n_final % 2 != 0) n_final--;

    //find next larger optimal n;
    int n = find_good_larger_fft_size(n_final);

    DEBUG(QString("n_final=%1 fft_size=%2").arg(n_final).arg(n).toStdString())

    int n2 = n/2;

    DEBUG(QString("fft n=%1, n2=n/2=%2").arg(n).arg(n2).toStdString())

    double * fft_r = new double[n];
    double * fft_i = new double[n];
    double * fft_out_r = new double[n];
    double * fft_out_i = new double[n];
    double rphase;

    fft_r[0] = 0.0;
    fft_i[0] = 0.0;


    for (int i= 1; i < n2; i++) {
        double f = double(i)/dur;
        if (f0 <= f && f <= f1) {
            rphase  = double(rand())/double(RAND_MAX) * 2*3.141592653589793;
            //rphase = 10;
            fft_r[i] = cos(rphase);
            fft_i[i] = sin(rphase);
        } else {
            fft_r[i] = 0.0;
            fft_i[i] = 0.0;
        }

        //complex conjugates
        fft_r[n-i] = fft_r[i];
        fft_i[n-i] = - fft_i[i];
    }

    DEBUG("fft filled!")

    fft(n, fft_r, fft_i, fft_out_r, fft_out_i);

    DEBUG("fft done!")

    v.resize(n_final);
    for (int i= 0; i < n_final; i++) {
        v[i]= fft_out_r[i];
    }

    //normalize standard deviation to sigma
    double var = 0;
    double mean = 0;
    for (int i= 0; i < n_final; i++) {
        var += v[i]*v[i];
        mean += v[i];
    }

    var = var/n_final;
    mean= mean/n_final; //mean should be zero by construction
    var = var - mean*mean;

    double fac = sigma/sqrt(var);

    // add sine wave
    for (int i = 0; i < n_final; i++) {
        v[i] = fac*(v[i]-mean) + amp * sin(2*3.141592653589793*ff*i*dt+phase);
    }

    delete [] fft_r;
    delete [] fft_i;
    delete [] fft_out_r;
    delete [] fft_out_i;

    return true;



/*

    //working own fft.h version

    srand(seed);

    templateTYPE dt = 1.0 /samp / 1000.0;
    int n = floor(dur/dt);
    if (n<1) n=1;
    int n2 = floor(n/2);

    //DEBUG(QString("n=%1, n2=%2").arg(n).arg(n2).toStdString())

    double * fft_r = new double[n];
    double * fft_i = new double[n];
    double * fft_out_r = new double[n];
    double * fft_out_i = new double[n];
    double rphase;

    fft_r[0] = 0.0;
    fft_i[0] = 0.0;

    DEBUG("filling fft")
            DEBUG(n)
            DEBUG(n2)

    for (int i= 1; i < n2; i++) {
        double f = double(i)/dur;
        if (f0 <= f && f <= f1) {
            rphase  = double(rand())/double(RAND_MAX) * 2*3.141592653589793;
            //rphase = 10;
            fft_r[i] = cos(rphase);
            fft_i[i] = - sin(rphase);
        } else {
            fft_r[i] = 0.0;
            fft_i[i] = 0.0;
        }

        //complex conjugates
        fft_r[n-i] = fft_r[i];
        fft_i[n-i] = - fft_i[i];
    }



    DEBUG("fft filled!")

    fft(n, fft_r, fft_i, fft_out_r, fft_out_i);

    DEBUG("fft done!")

    v.resize(n);
    for (int i= 0; i < n; i++) {
        v[i]= fft_out_r[i];
    }


    //normalize standard deviation to sigma
    double var = 0;
    double mean = 0;
    for (int i= 0; i < n; i++) {
        var += v[i]*v[i];
        mean += v[i];
    }
    var = var/n;
    mean= mean/n; //mean should be zero by construction
    var = var - mean*mean;

    double fac = sigma/sqrt(var);

    // add sine wave
    for (int i = 0; i < n; i++) {
        v[i] = fac*(v[i]-mean) + amp * sin(2*3.141592653589793*ff*i*dt+phase);
    }

    delete [] fft_r;
    delete [] fft_i;
    delete [] fft_out_r;
    delete [] fft_out_i;

    return true;
    */


/*
    //working gsl fft complex version -> slow under windows

   srand(seed);

   templateTYPE dt = 1.0 /samp / 1000.0;
   int n = ceil(dur/dt);
   if (n<1) n=1;
   int n2 = floor(n/2);
   DEBUG(QString("n=%1, n2=%2").arg(n).arg(n2).toStdString())


   gsl_fft_complex_wavetable * wavetable;
   gsl_fft_complex_workspace * workspace;

   double * fft = new double[2*n]; //for strange resons fft[2*n] gives seg fault for large n;
   double rphase;

   DEBUG("array creation done!")

   fft[0] = 0.0;
   fft[1] = 0.0;

   DEBUG("filling fft")

   for (int i= 1; i < n2; i++) {
       double f = double(i)/dur;
       if (f0 <= f && f <= f1) {
           rphase  = double(rand())/double(RAND_MAX) * 2*3.141592653589793;
           //rphase = 10;
           REAL(fft,i) = cos(rphase);
           IMAG(fft,i) = sin(rphase);
       } else {
           REAL(fft,i) = 0.0;
           IMAG(fft,i) = 0.0;
       }

       //complex conjugates
       REAL(fft, n-i) = REAL(fft, i);
       IMAG(fft, n-i) = - IMAG(fft, i);
   }

   DEBUG("fft filled!")

   wavetable = gsl_fft_complex_wavetable_alloc (n);
   workspace = gsl_fft_complex_workspace_alloc (n);

   gsl_fft_complex_inverse(fft, 1, n, wavetable, workspace);

   DEBUG("fft done!")

   v.resize(n);
   for (int i= 0; i < n; i++) {
       v[i]= REAL(fft,i);
   }


   //normalize standard deviation to sigma
   double var = 0;
   double mean = 0;
   for (int i= 0; i < n; i++) {
       var += v[i]*v[i];
       mean += v[i];
   }
   var = var/n;
   mean= mean/n; //mean should be zero by construction
   var = var - mean*mean;

   double fac = sigma/sqrt(var);

   // add sine wave
   for (int i = 0; i < n; i++) {
      v[i] = fac*(v[i]-mean) + amp * sin(2*3.141592653589793*ff*i*dt+phase);
   }

   gsl_fft_complex_wavetable_free (wavetable);
   gsl_fft_complex_workspace_free (workspace);
   delete [] fft;

   return true;
*/

   //  half-complex gsl fft version -> extremely slow in windows ???
/*
   srand(seed);

   templateTYPE dt = 1.0 /samp / 1000.0;
   int n = ceil(dur/dt);
   if (n<1) n=1;
   int n2;
   if (n%2 ==0) n2 = n/2; else n2 = (n+1)/2;
   DEBUG(QString("n=%1, n2=%2").arg(n).arg(n2).toStdString())

   //the halfcomplex data
   double * fft = new double[n];
   double rphase;
   DEBUG("array allocation done!")

   fft[0] = 0.0;
   for (int i= 1; i < n2; i++) {
       double f = double(i)/dur;
       if (f0 <= f && f <= f1) {
           rphase  = double(rand())/double(RAND_MAX) * 2*3.141592653589793;
           //rphase = 10;
           fft[2*i-1] = cos(rphase);
           fft[2*i] = sin(rphase);
       } else {
           fft[2*i-1] = 0.0;
           fft[2*i] = 0.0;
       }
   }
   if (n % 2 == 0) fft[n-1] = 0.0;  //in the even case fill last entry
   DEBUG("fft filled!")

   gsl_fft_halfcomplex_wavetable * wavetable;
   gsl_fft_real_workspace * workspace;

   wavetable = gsl_fft_halfcomplex_wavetable_alloc (n);
   workspace = gsl_fft_real_workspace_alloc (n);

   gsl_fft_halfcomplex_backward(fft, 1, n, wavetable, workspace);
   DEBUG("fft done!")

   v.resize(n);
   for (int i= 0; i < n; i++) {
       v[i]= fft[i];
   }

   //normalize standard deviation to sigma
   double var = 0;
   double mean = 0;
   for (int i= 0; i < n; i++) {
       var += v[i]*v[i];
       mean += v[i];
   }
   var = var/n;
   mean= mean/n; //mean should be zero by construction
   var = var - mean*mean;

   double fac = sigma/sqrt(var);

   // add sine wave
   for (int i = 0; i < n; i++) {
       v[i] = fac*(v[i]-mean) + amp * sin(2*3.141592653589793*ff*i*dt+phase);
   }

   gsl_fft_halfcomplex_wavetable_free (wavetable);
   gsl_fft_real_workspace_free (workspace);
   delete [] fft;

   return true;

*/



   // workingt fftw version

/*
   srand(seed);

   templateTYPE dt = 1.0 /samp / 1000.0;
   int n = ceil(dur/dt);
   if (n<1) n=1;
   int n2 = floor(n/2);

   fftw_complex *in, *out;
   fftw_plan p;


   in = (fftw_complex*) malloc(sizeof(fftw_complex) * n);
   out = (fftw_complex*) malloc(sizeof(fftw_complex) * n);
   double rphase;


   //we take no constant offset
   in[0][0] = 0; in[0][1] = 0;

    for (int i= 1; i < n2; i++) {
        double f = double(i)/dur;
        if (f0 <= f && f <= f1) {
            rphase  = double(rand())/double(RAND_MAX) * 2*3.141592653589793;
            in[i][0] = cos(rphase);
            in[i][1] = sin(rphase);
        } else {
            in[i][0] = 0.0;
            in[i][1] = 0.0;
        }

        //complex conjugates
        in[n-i][0] = in[i][0];
        in[n-i][1] = -in[i][1];
    }



    p = fftw_plan_dft_1d(n, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);


    v.resize(n);
    for (int i= 0; i <n; i++) {
        v[i]=out[i][0];
    }

    fftw_destroy_plan(p);
    fftw_free(in); fftw_free(out);


    //normalize standard deviation to sigma
    double var = 0;
    double mean = 0;
    for (int i= 0; i < n; i++) {
        var += v[i]*v[i];
        mean += v[i];
    }
    var = var/n;
    mean= mean/n; //mean should be zero by construction
    var = var - mean*mean;

    double fac = sigma/sqrt(var);

    // add sine wave
    for (int i = 0; i < n; i++) {
       v[i] = fac*(v[i]-mean) + amp * sin(2*3.141592653589793*ff*i*dt+phase);
    }

   return true;

   */
}


/* create a sin stimulus of duration \param dur [sec] assuming a sampling frequency of \param samp [kHz]
 * with amplitude \param amp and phase \param phase and freqeuncy \\param ff [Hz]
 */
bool create_sin(DataTYPE dur, DataTYPE samp,
                            DataTYPE ff,  DataTYPE phase, DataTYPE amp,
                            DataTYPE ff2,  DataTYPE phase2, DataTYPE amp2,
                            bool positive,
                            DataVECTOR& v) {
    DEBUG("create_sin")

    DataTYPE dt = 1.0 / samp / 1000.0;
    int n = int(dur * samp * 1000) + 1;

    v.resize(n);
    // sine wave
    for (int i = 0; i < n; i++) {
        v[i] = amp * sin(2*3.141592653589793*ff*i*dt+phase) +  amp2 * sin(2*3.141592653589793*ff2*i*dt+phase2);
        if (positive) {
            if (v[i] <0) v[i]= 0;
            else if (ff2 > 0 && sin(2*3.141592653589793*ff2*i*dt+phase2) < 0)
                    v[i] = 0;
        }
    }

    return true;
}



/*****************************************************************************************************************
 *
 *      Data Analysis
 *
 *****************************************************************************************************************/


//simple standard routines
void mean(const DataVECTOR& d1, const DataVECTOR& d2, DataVECTOR& m){
    int n = d1.size();
    if (int(d2.size())< n ) n = d2.size();
    m.resize(n);
    for (int i = 0; i < n; i++) {
        m[i] = 0.5*(d1[i]+d2[i]);
    }
}

void reverse_data(DataVECTOR& d) {
    reverse(d.begin(), d.end());
}


//detect offsets
void first_non_zero(const DataVECTOR& d1, int& p, const DataTYPE& zero){
    p = 0; int n = d1.size();
    while (p <n && d1[p] == zero) p++;
}

//detect offsets
void last_non_zero(const DataVECTOR& d1, int& p, const DataTYPE& zero){
    int n = d1.size(); p = n-1;
    while (p >= 0 && d1[p] == zero) p--;
    p = n-1-p;
}

void remove_ends(DataVECTOR& d1, int n1, int n2){
    if (n1 ==0 && n2 ==0) return;
    int n = d1.size();
    if (n==0) return;

    //some safety:
    //we want array to cantain at least one number !

    if ((n1 + n2) > n) {
        double d = d1[0];
        d1.resize(1);
        d1[0] = d;
        return;
    }

    DataVECTOR v;
    v.resize(n - n1 - n2);
    copy(d1.begin()+n1, d1.end()-n2, v.begin());

    d1 = v;
}


//smooth data
void smooth_data(DataVECTOR& data, const int& width, DataVECTOR& smooth){
    int n = data.size();
    int n2 = ceil(double(n)/width);
    DataTYPE mean;
    smooth.resize(n2);

    int k = 0;
    for (int i =0; i < n2-1; i++){
        mean = 0.0;
        for (int j=0; j< width; j++){
            mean += data[k];
            k++;
        }
        smooth[i] = mean/width;
    }
    mean = 0.0;
    int k1 = k;
    while (k<n){
        mean += data[k];
        k++;
    }
    smooth[n2-1] = mean /(k-k1);
}


// peak detection by Tods and Andrews
void find_peaks(const DataVECTOR& data, const DataTYPE& threshold, std::vector<int>& peaks, int nmax){
    peaks.clear();
    int i = 0;
    int d = 0;
    int s = 0;
    DataTYPE qa = data[0], qb = data[0], qi;
    int n= data.size();
    int np = 0; int nm = nmax;
    if (nmax<0) nm = n+1;

    while (i < n-1 && np < nm){
        i++;
        qi = data[i]; //next data point;
        if (d == 0) {
            if (qa >= qi + threshold) d = -1; // falling slope
            else if (qi >= qb + threshold) d = 1; //rising slop
            if (qa < qi) qa = qi; // raise new high data
            else if (qi < qb) qb = qi; // lower new low data
            s = i; // at d==0 always possible peak
        } else if (d>0){  //data was rising
            if (qa < qi) { //data still rising
                qa = qi; //raise new high data
                s = i; //i is possible peak
            } else if (qa >= qi + threshold) { // data has fallen below high data - threshold -> peak
                peaks.push_back(s);
                np++;
                qb = qi; //lower upper data
                d = -1;  // data is really falling again
                //s = i; // should not need this only for simultaneous trough detection
            }
        } else if (d<0) { // data was falling
            if (qi < qb) {// still falling
                qb = qi;  //set new lower data
            } else if (qi >= qb + threshold) { // data is rising again
                qa = qi;
                d = 1;
            }
        }
    }
}





//impedance |ft(ouput)|/|f(input)|^2 here !!
//todos: speedup: we know that we have real data -> can use real gls
//       speedup: impedance needs only e calculated to n/2 as it is symmetric from there and gives no information !!
void impedance(const DataVECTOR& in, const DataVECTOR& out, DataVECTOR& z){
    DEBUG("impedance()")


    int n = in.size();

    if (int(out.size()) < n){
        //batch_error("impedance", "sizes of in and outputs do not match!");
        return;
    }


    double * din_r = new double[n];
    double * din_i = new double[n];
    double * dout_r = new double[n];
    double * dout_i = new double[n];

    for (int i= 1; i < n; i++) {
        din_r[i] = in[i];
        din_i[i] = 0.0;

        dout_r[i] = out[i];
        dout_i[i] = 0.0;
    }

    double * fftin_r = new double[n];
    double * fftin_i = new double[n];
    double * fftout_r = new double[n];
    double * fftout_i = new double[n];

    fft(n, din_r, din_i, fftin_r, fftin_i);
    fft(n, dout_r, dout_i, fftout_r, fftout_i);

    z.resize(n);
    for (int i= 0; i < n; i++) {
        z[i] = ( fftout_r[i]*fftout_r[i] +  fftout_i[i]*fftout_i[i] ) / ( fftin_r[i]* fftin_r[i] +   fftin_i[i]* fftin_i[i] );
    }

    delete [] din_r; delete [] din_i; delete [] fftin_r; delete [] fftin_i;
    delete [] dout_r; delete [] dout_i; delete [] fftout_r; delete [] fftout_i;

    return;



/*
    //working gsl version (slow under windows)

    int n = in.size();

    if (int(out.size()) < n){
        batch_error("impedance", "sizes of in and outputs do not match!");
        return;
    }

    gsl_fft_complex_wavetable * wavetable;
    gsl_fft_complex_workspace * workspace;

    double * fftin = new double[n*2];
    double * fftout = new double[n*2];

    for (int i= 1; i < n; i++) {
        REAL(fftin,i) = in[i];
        IMAG(fftin,i) = 0.0;

        REAL(fftout,i) = out[i];
        IMAG(fftin,i) = 0.0;
    }

    wavetable = gsl_fft_complex_wavetable_alloc (n);
    workspace = gsl_fft_complex_workspace_alloc (n);

    gsl_fft_complex_forward(fftin, 1, n, wavetable, workspace);
    gsl_fft_complex_forward(fftout, 1, n, wavetable, workspace);


    z.resize(n);
    for (int i= 0; i < n; i++) {
        z[i] = ( REAL(fftout,i)*REAL(fftout,i) +  IMAG(fftout,i)*IMAG(fftout,i) ) / ( REAL(fftin,i)*REAL(fftin,i) +  IMAG(fftin,i)*IMAG(fftin,i) );
    }

    gsl_fft_complex_wavetable_free (wavetable);
    gsl_fft_complex_workspace_free (workspace);
    delete [] fftin;
    delete [] fftout;

    return;

            */
}

















