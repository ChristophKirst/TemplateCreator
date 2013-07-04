/*****************************************************************************************************************

    Fast Fourier Transfrom

    Author: Christoph Kirst (ckirst@nld.ds.mpg.de)
    Date:   2012, LMU Munich

 *****************************************************************************************************************/

#ifndef FFT_H
#define FFT_H

/*! Fast Fourier Transfrom optimized for radix-10. Fourier transforms the complex vector
 * \param xRe and \param xIm of length \param n into the complex vector \param yRe and \param yIm
 * the algorithm is optimized for radix-10
 */
void fft(int n, double xRe[], double xIm[],double yRe[], double yIm[]);

/*! Find a good vector size close to \param n that is optimized for use with \ref fft.
 *  The number can be larger or smaller than \param n
 */
int find_good_fft_size(int n);

/*! Find a good vector size close to \param n that is optimized for use with \ref fft.
 *  The number will be smaller or equal to \param n
 */
int find_good_smaller_fft_size(int n);

/*! Find a good vector size close to \param n that is optimized for use with \ref fft.
 *  The number will be larger or equal to \param n
 */
int find_good_larger_fft_size(int n);


#endif // FFT_H
