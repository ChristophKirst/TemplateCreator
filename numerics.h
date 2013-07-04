/*****************************************************************************************************************

    Numerical Implementations of Data Analysis / Template Creation Functions

    Author: Christoph Kirst (ckirst@nld.ds.mpg.de)
    Date:   2012, LMU Munich

 *****************************************************************************************************************/

#ifndef NUMERICS_H
#define NUMERICS_H

#include <string>
#include <vector>

typedef float DataTYPE; //heka uses floats
typedef std::vector<DataTYPE> DataVECTOR;

#define REAL(z,i) ((z)[2*(i)])
#define IMAG(z,i) ((z)[2*(i)+1])


/*****************************************************************************************************************
 *
 *      Template Numerics
 *
 *****************************************************************************************************************/


/*! standard manipulations: Adds offset \param off to sample and extends \param left and \param right seconds
 *                          to the begining and end assuming a smaple rate of \param samp
 */
void postprocess_template(DataTYPE samp, DataTYPE off, DataTYPE left, DataTYPE right,
                          DataVECTOR& v);


/*! create a zap stimulus of duration \param dur [sec] assuming a sampling frequency of \param samp [kHz]
 * starting from freq \param f0 to \param f1 [Hz] with amplitude \param amp
 */
bool create_zap(DataTYPE dur, DataTYPE samp,
                            DataTYPE f0, DataTYPE f1, DataTYPE amp, bool reverse,
                            DataVECTOR& v );


/*! create a noise stimulus of duration \param dur [sec] assuming a sampling frequency of \param samp [kHz]
 * with uniform frequency spectrum from \param f0 to \param f1 [Hz] and standard deviation \param sigma
 * added is a LFP like signal with amplitude \param amp and phase \param phase and freqeuncy \\param ff [Hz]
 */
bool create_noise(DataTYPE dur, DataTYPE samp,
                              DataTYPE ff,  DataTYPE phase, DataTYPE amp,
                              DataTYPE f0, DataTYPE f1, DataTYPE sigma, int seed,
                              DataVECTOR& v);


/*! create a sin stimulus of duration \param dur [sec] assuming a sampling frequency of \param samp [kHz]
 * with amplitude \param amp and phase \param phase and freqeuncy \\param ff [Hz]
 */
bool create_sin(DataTYPE dur, DataTYPE samp,
                            DataTYPE ff,  DataTYPE phase, DataTYPE amp,
                            DataTYPE ff2,  DataTYPE phase2, DataTYPE amp2,
                            bool positive,
                            DataVECTOR& v);



/*****************************************************************************************************************
 *
 *      Data Analysis
 *
 *****************************************************************************************************************/



/*! \param m = 1/2 ( \param d1 + \param d2 )
 */
void mean(const DataVECTOR& d1, const DataVECTOR& d2, DataVECTOR& m);


/*! reverse data
 */
void reverse_data(DataVECTOR& d);


/*! detect position \param p of first value which is not \param zero in \param d1
 */
void first_non_zero(const DataVECTOR& d1, int& p, const DataTYPE& zero);


/*! detect position \param p of last value which is not \param zero in \param d1
 */
void last_non_zero(const DataVECTOR& d1, int& p, const DataTYPE& zero);


/*! removes \param n1 elements in the front and \param n2 in the end of \param d1
 */
void remove_ends(DataVECTOR& d1, int n1, int n2);


/*! smooth data \param d1 by calculating averages in bins of width \param width
 */
void smooth_data(DataVECTOR& data, const int& width, DataVECTOR& smooth);


/*! peak detection algorithm by Tods and Andrews
 *  finds first \param nmax positions of peaks of height \param threshld in \param data
 *  and stores their poisiton in \param peaks
 */
void find_peaks(const DataVECTOR& data, const DataTYPE& threshold, std::vector<int>& peaks, int nmax);


/*! impedance of response \param out to input \param in, i.e.
 * |\param z = fft(out)|/|fft(in)|^2
 */
void impedance(const DataVECTOR& in, const DataVECTOR& out, DataVECTOR& z);


#endif // NUMERICS_H
