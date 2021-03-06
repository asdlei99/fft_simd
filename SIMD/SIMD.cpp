// SIMD.cpp: 定义控制台应用程序的入口点。
// Target: Be familiar with the SIMD command.

#include <iostream>
#include <vector>
//#include <intrin.h>
#include <emmintrin.h>	// sse2
#include <math.h>
#include <fstream>
#include <vector>
#include <string>
#include <complex>
#include <time.h>
using namespace std;
#define PI 3.1415926535897932384626433832

int standard_dft( vector<double>dft_in, vector<complex<double>> &dft_out );
int fft(vector<double> &fft_in, vector<complex<double>> &fft_out);
int time_consum(vector<double>&data);			// time consum
template<typename T> 
void separate(T* a, int n);
struct comp {
	double real =0;
	double imag = 0;
};
void fft2(complex<double>* X, int N);	
void fft2_simd(comp *X, int N);
double *complex_mul(double *A, double *B);
comp complex_mul_c(comp A, comp B);				// C version complex mul
//#define OUTPUT_TO_TXT
#define USING_IMPROVEMENT
int main()
{
	/*----------------------------------------------------------------
	 *	SIMD test.
	 * -Task 1. Implement standard DFT algorithm with C++.
	 * -Task 2. Rewrite the DFT with Cooley–Tukey FFT algorithm.
	 * -Task 3. Ues SIMD to accelerate the FFT alogrithm.
	 * -Task 4. Make a time consuming contrast among these algorithms.
	----------------------------------------------------------------*/
	ifstream fi("fft_1024.txt");
	vector<double> data;
	string read_temp;
	while (fi.good())
	{
		getline(fi, read_temp);
		data.push_back( stod(read_temp));
	}

	time_consum(data); // time consuming contrast

#ifdef OUTPUT_TO_TXT

	// load fft samples for test
	comp fft_in[1024];
	for (size_t i = 0; i < 1024; i++)
	{
		fft_in[i].real = data.at(i);
		fft_in[i].imag = 0;
	}

	fft2_simd(fft_in, 1024);
	//----------------------Output the result----------------------------//
	ofstream fo("fft_out_v2.txt", ios::ate);
	for (size_t i = 0; i != 1024; ++i) {
		fo << fft_in[i].real << " + " << fft_in[i].imag << "j" << "\n";
	}
	fo.close();

#endif // OUTPUT_TO_TXT

	fi.close();
    return 0;
}

int standard_dft(vector<double> dft_in, vector<complex<double>> &dft_out)
{
	// Standard fft algorithm without any optimization.
	// Ref:https://en.wikipedia.org/wiki/DFT

	int N = dft_in.size();
	for (size_t k = 0; k != N; ++k) {
		//complex<double> x_n(0, 0);
		double re = 0;												// the real part
		double im = 0;												// the imag part
		for (size_t n = 0; n != N; ++n) {
			re += dft_in[n] * cos(-double(k * 2 * PI * n / N));		// Euler's formula： exp( i*x ) = cos_x + i*sin_x
			im += dft_in[n] * sin(-double(k * 2 * PI * n / N));
			// x_n += fft_in[n] * exp( (double)k *-1i * 2.0*PI* (double)( n )/double( N) );	
			// Be careful for the integer division  in C++
		}
		dft_out.push_back( complex<double>(re, im) );
	}
	return 0;
}

int fft(vector<double> &fft_in, vector<complex<double>>& fft_out)
{
	// fft_in	: the vector of input data
	// fft_out	: fft result
	// Notes	: the length of fft must be a power of 2,and it is not a in-place algorithm
	// ref		: https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm
	int N = fft_in.size();	 
	if (N >=2) {
		// Seperate into two parts
		vector<double> odd;			// odd part
		vector<double> even;		// even part
		for (size_t n = 0; n != N ; ++n) {
			if ( (n+1) % 2 == 0) {
				even.push_back(fft_in.at( n ));
			}
			else {
				odd.push_back(fft_in.at( n ));
			}
		}

		// recursive
		vector<complex<double>> fft_even_out(fft_out.begin(),fft_out.begin()+N/2);	// low 
		vector<complex<double>> fft_odd_out(fft_out.begin() + N / 2,fft_out.end());	// high
		fft(even, fft_even_out);
		fft(odd, fft_odd_out);

		// combine the odd and even part
		complex<double> odd_out;
		complex<double> even_out;
		for (size_t n = 0; n != N / 2; ++n) {					
			even_out = (N == 2) ? even[n] + fft_even_out[n ] : fft_even_out[n ];
			odd_out  = (N == 2) ? odd[n] + fft_odd_out[n ]: fft_odd_out[n ];		
			complex<double> w = exp( complex<double>( 0, -2.0 * PI *double(n) / (double)(N) ) );
			fft_out[n] = even_out + w * odd_out;				// even part
			fft_out[n + N / 2] = even_out - w * odd_out;		// odd part
		}
	}
	else
		return 0;

	return 0;
}
int time_consum(vector<double>&data)
{
	vector<complex<double>> result(1024);
	size_t counter = 10;
	clock_t dft_start = clock();
	// dft
	for (size_t n = 0; n != counter; ++n) {
		standard_dft(data, result); // Task 1.
	}
	clock_t dft_end = clock();
	// fft
	for (size_t n = 0; n != counter; ++n) {
		fft(data, result);	// Task 2.
	}
	clock_t fft_end = clock();
	// fft simd version
	// load data
	double fft_real[1024] = { 0 };
	memcpy(fft_real, (double*)&data[0], data.size() * sizeof(double));		// initialize the array from vector
	double fft_imag[1024] = { 0 };
	comp fft_in[1024];
	for (size_t n = 0; n != 1024; ++n) {
		fft_in[n].real = fft_real[n];
	}
	// start to check
	clock_t simd_s = clock();
	for (size_t ns = 0; ns != 10000; ns++) {
	fft2_simd(fft_in, 1024);
	}
	clock_t simd_e = clock();
	cout << "Standard dft cost : " << (dft_end - dft_start) / 10 << " ms.\n";
	cout << "      FFT cost    : " << (fft_end - dft_end) / 10 << " ms.\n";
	cout << "  FFT simd cost   : " << (simd_e - simd_s) / 10000.0 << " ms.\n";
	return 0;
}

void fft2(complex<double>* X, int N) {
	if (N < 2) {
		// bottom of recursion.
		// Do nothing here, because already X[0] = x[0]
	}
	else {
		separate(X, N);						// all evens to lower half, all odds to upper half
		fft2(X, N / 2);						// recurse even items
		fft2(X + N / 2, N / 2);				// recurse odd  items
											// combine results of two half recursions
		for (int k = 0; k<N / 2; k++) {

			complex<double> e = X[k];			// even
			complex<double> o = X[k + N / 2];   // odd
												// w is the "twiddle-factor"
			complex<double> w = exp(complex<double>(0, -2.*PI*k / N));
			X[k] = e + w * o;
			X[k + N / 2] = e - w * o;
			/*
			comp e = X[k];						// even
			comp o = X[k + N / 2];				// odd
			complex<double> ww = exp(complex<double>(0, -2.*PI*k / N));
			comp w;
			w.real = ww.real();
			w.imag = ww.imag();
			comp wo = complex_mul_c(w, o);
			X[k].real = e.real + wo.real;
			X[k].imag = e.imag + wo.imag;
			X[k + N / 2].real = e.real - wo.real;
			X[k + N / 2].imag = e.imag - wo.imag;
			*/
		}
	}
}

void fft2_simd(comp* X, int N)
{	
	// Notes	: the length of fft must be a power of 2,and it is  a in-place algorithm
	// ref		: https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm
	if (N < 2) {
		// bottom of recursion.
		// Do nothing here, because already X[0] = x[0]
	}
	else {
		separate(X, N);							// all evens to lower half, all odds to upper half
		fft2_simd(X, N / 2);					// recurse even items
		fft2_simd(X + N / 2, N / 2);			// recurse odd  items
												// combine results of two half recursions
		for (int k = 0; k<N / 2; k++) {

			//__m128d e = _mm_load_pd( (double *)&X[k] );   // even
			__m128d o = _mm_load_pd( (double *)&X[k + N/2 ] );   // odd
			double cc = cos(-2.*PI*k / N);
			double ss = sin(-2.*PI*k / N);
#ifndef USING_IMPROVEMENT
			__m128d wr =  _mm_set_pd( cc,cc );		// cc 
			__m128d wi =  _mm_set_pd( ss, ss );		// dd 
			// compute the w*o
			wr = _mm_mul_pd(o,wr);					// ac|bc
			__m128d n1 = _mm_shuffle_pd(o,o,_MM_SHUFFLE2(0,1) ); // invert
			wi = _mm_mul_pd(n1, wi);				// bd|ad
			n1 = _mm_sub_pd(wr, wi);				// ac-bd|x
			wr = _mm_add_pd(wr, wi);				// x|bc+ad
			n1 = _mm_shuffle_pd(n1,wr,_MM_SHUFFLE2(1,0));// select ac-bd|bc+ad
#else
			// Thanks for the improvement from @htltdco. Now we can use fewer instructions.
			// reference from: https://github.com/jagger2048/fft_simd/issues/1 
			__m128d wr = _mm_load1_pd(&cc);			//__m128d wr =  _mm_set_pd( cc,cc );		// cc 
			__m128d wi = _mm_set_pd(ss, -ss);		// -d | d	, note that it is reverse order
			// compute the w*o
			wr = _mm_mul_pd(o, wr);					// ac|bc
			__m128d n1 = _mm_shuffle_pd(o, o, _MM_SHUFFLE2(0, 1)); // invert
			wi = _mm_mul_pd(n1, wi);				// -bd|ad
			n1 = _mm_add_pd(wr, wi);				// ac-bd|bc+ad 

#endif // USING_IMPROVEMENT


			o = _mm_load_pd((double *)&X[k]);		// load even part
			wr = _mm_add_pd(o, n1);					// compute even part, X_e + w * X_o;
			wi = _mm_sub_pd(o, n1);					// compute odd part,  X_e - w * X_o;
			_mm_store_pd((double *)&X[k],wr);
			_mm_store_pd((double *)&X[k + N/2],wi);
		}
	}
}

double * complex_mul(double * A, double * B)
{
	//-------------Basied SIMD testing--------------//
	//  Using SIMD to compute the complex multiplication

	// if A=a+b*j,B=c+d*j; then A * B = (ac -bd) + (bc+ad)*j
	// load a b
	__m128d n1 = _mm_load_pd(A);
	// load b a
	__m128d n2 = _mm_loadr_pd(A);
	// load c c
	__m128d n3 = _mm_load1_pd(B);
	// mul, ac bc
	n1 = _mm_mul_pd(n1, n3);//  a b * c c = ac|bc
	// load d d
	n3 = _mm_load1_pd(B + 1);
	// mul,bd ad
	n3 = _mm_mul_pd(n2, n3);
	// sub n1 n3->ac-bd,bc-ad
	n2 = _mm_sub_pd(n1, n3);
	// add n1 n3->ac+bd,bc+ad
	n3 = _mm_add_pd(n1, n3);
	// select n2.at[0] n3.at[1] || ac-bd bc+ad
	n1 = _mm_shuffle_pd(n2, n3, _MM_SHUFFLE2(1, 0));
	//dst[63:0] : = (imm8[0] == 0) ? a[63:0] : a[127:64]
	//dst[127:64] : = (imm8[1] == 0) ? b[63:0] : b[127:64]
	// store and output
	double *C = new double[2];
	_mm_store_pd(C, n1);
	return C;
}
comp complex_mul_c(comp  A, comp  B)
{
	// c version of complex multiplication
	comp out;
	out.real = A.real*B.real - A.imag*B.imag;
	out.imag = A.real*B.imag + A.imag*B.real;
	return out;
}

template<typename T>
void separate(T* a, int n)
{
	T* b = new T[n / 2];				// get temp heap storage
	for (int i = 0; i<n / 2; i++)		// copy all odd elements to heap storage
		b[i] = a[i * 2 + 1];
	for (int i = 0; i<n / 2; i++)		// copy all even elements to lower-half of a[]
		a[i] = a[i * 2];
	for (int i = 0; i<n / 2; i++)		// copy all odd (from heap) to upper-half of a[]
		a[i + n / 2] = b[i];
	delete[] b;							// delete heap storage
}

/*
// the other version of complex multiplication with SIMD
int complex_mul(__m128d &A, __m128d &B)
{
// if A=a+b*j,B=c+d*j; then A * B = (ac -bd) + (bc+ad)*j
__m128d C = _mm_mul_pd(A, B);// ac|bd
__m128d D = _mm_shuffle_pd(C, C ,_MM_SHUFFLE2(0,1)); // invert bd|ac
C = _mm_sub_pd(C, D);  // ac-bd|x
A = _mm_shuffle_pd(A, A, _MM_SHUFFLE2(0, 1)); // invert b|a
B = _mm_mul_pd(A, B);	// bc|ad
D = _mm_shuffle_pd(B, B, _MM_SHUFFLE2(0, 1)); // invert
B = _mm_add_pd(B, D);	// bc+ad|x
A = _mm_shuffle_pd(C, B, _MM_SHUFFLE2(0, 0)); // select

return 0;
}
*/