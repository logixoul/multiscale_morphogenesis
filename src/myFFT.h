#pragma once
#include "util.h"
#include <complex> // tmp

class myFFT
{
public:
	typedef float Scalar;
	enum class Dir { Forward, Backward };
	static Array2D<complex<Scalar>> fft(Array2D<complex<Scalar>> in, Dir dir);

	static void fftGpu(gl::TextureRef& in, Dir dir);
};

