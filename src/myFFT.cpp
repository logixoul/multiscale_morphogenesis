#include "precompiled.h"
#include "myFFT.h"
#include "gpgpu.h"
#include "stuff.h"

static int count;

string compmul = R"END(
		vec2 compmul(vec2 p, vec2 q) {
			return mat2(p,-p.y,p.x) * q;
		}
		)END";

int bitReverse(int n, int bits) {
	int reversedN = n;
	int count = bits - 1;

	n >>= 1;
	while (n > 0) {
		reversedN = (reversedN << 1) | (n & 1);
		count--;
		n >>= 1;
	}

	return ((reversedN << count) & ((1 << bits) - 1));
}
// N input samples in X[] are FFT'd and results left in X[].
// Because of Nyquist theorem, N samples means 
// only first N/2 FFT results in X[] are the answer.
// (upper half of X[] is a reflection with no new information).
static void fftImpl(complex<myFFT::Scalar>* X, int bufSize, myFFT::Scalar expSign) {
	::count++;
	int bits = ilog2(bufSize); // this works correctly. verified.
	/*auto Y = new complex<myFFT::Scalar>[bufSize];
	for (int i = 0; i < bufSize; i++) {
		int j = bitReverse(i, bits);
		Y[i] = X[j];
	}
	for (int i = 0; i < bufSize; i++) {
		X[i] = Y[i];
	}*/
	for (int i = 0; i < bufSize; i++) {
		int j = bitReverse(i, bits);
		if (j > i) swap(X[i], X[j]);
	}
	//delete[] Y;
	for (int N = 2; N <= bufSize; N *= 2) {
		for (int i = 0; i < bufSize; i += N) {
			for (int k = 0; k < N / 2; k++) {
				int evenIndex = i + k;
				int oddIndex = i + k + (N / 2);
				complex<myFFT::Scalar> e = X[evenIndex];   // even
				complex<myFFT::Scalar> o = X[oddIndex];   // odd
								// w is the "twiddle-factor"
				float arg = expSign * 2.*M_PI*k / (float)N;
				complex<myFFT::Scalar> w(cos(arg), sin(arg));
				X[evenIndex] = e + w * o;
				X[oddIndex] = e - w * o;
			}
		}
	}
}

Array2D<complex<myFFT::Scalar>> myFFT::fft(Array2D<complex<myFFT::Scalar>> in, Dir dir)
{
	cout << "CPU ================= " << endl;
	::count = 0;
	myFFT::Scalar expSign = dir == Dir::Forward ? -1.0 : 1.0;
	Array2D<complex<myFFT::Scalar>> inC(in.Size());
	for (int y = 0; y < in.h; y++) {
		vector<complex<myFFT::Scalar>> v;
		v.reserve(in.w);
		for (int x = 0; x < in.w; x++) {
			auto& here = in(x, y);
			v.push_back(here);
		}
		fftImpl(v.data(), v.size(), expSign);
		for (int x = 0; x < in.w; x++) {
			auto& here = inC(x, y);
			here = v[x];
		}
	}
	for (int x = 0; x < in.w; x++) {
		vector<complex<myFFT::Scalar>> v;
		v.reserve(in.h);
		for (int y = 0; y < in.h; y++) {
			auto& here = inC(x, y);
			v.push_back(here);
		}
		fftImpl(v.data(), v.size(), expSign);
		for (int y = 0; y < in.h; y++) {
			auto& here = inC(x, y);
			here = v[y];
		}
	}
	forxy(inC) {
		inC(p) /= sqrt<myFFT::Scalar>(inC.area);
	}
	//cout << "count=" << ::count << endl;
	return inC;
}

// =============================== BEGIN GPU CODE ==========================

// https://rosettacode.org/wiki/Fast_Fourier_transform#C.23

static void fftGpuOneAxis(gl::TextureRef& X, int problemSize, int axis, myFFT::Scalar expSign) {
	globaldict["expSign"] = expSign;
	globaldictI["problemSize"] = problemSize;
	static string getWo =
		compmul +
		"vec2 getWo(int indexInSubproblem, vec2 o) {"
		"const float M_PI = 3.14159265359;"
		"float k = float(indexInSubproblem);"
		"float arg = expSign * 2.*M_PI*k / problemSize;"
		"vec2 w = vec2(cos(arg), sin(arg));"
		"return compmul(w, o);"
		"}";
	globaldictI["halfProblemSize"] = problemSize / 2;
	globaldictI["axis"] = axis;
	glBindImageTexture(0, X->getId(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32F);
	shade2(X,
		"ivec2 fragCoord = ivec2(gl_FragCoord.xy);"
		"int indexInSubproblem = fragCoord[axis] % problemSize;"
		"if(indexInSubproblem >= halfProblemSize) {"
		"	discard;"
		"}"
		"ivec2 fetchPosE = fragCoord;"
		"vec2 e = texelFetch(tex, fetchPosE, 0).rg;"
		"ivec2 fetchPosO = fetchPosE;"
		"fetchPosO[axis] += halfProblemSize;"
		"vec2 o = texelFetch(tex, fetchPosO, 0).xy;"
		"vec2 wo = getWo(indexInSubproblem, o);"
		"memoryBarrier();"
		"imageStore(image, fetchPosE, vec4(e + wo, 0, 0));"
		"imageStore(image, fetchPosO, vec4(e - wo, 0, 0));"
		, ShadeOpts().scope("e+wo").enableResult(false).enableWrite(false)
		, getWo
	);
}

void breadthFirst(gl::TextureRef& in, int axis, myFFT::Scalar expSign) {
	GPU_SCOPE("breadthFirst");
	int N = in->getSize()[axis];
	int bits = (int)ilog2(N);
	globaldictI["axis"] = axis;
	globaldictI["bits"] = bits;
	globaldictI["N"] = N;
	glBindImageTexture(0, in->getId(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32F);
	shade2(in,
		"ivec2 hereCoords = ivec2(gl_FragCoord.xy);"
		"ivec2 thereCoords = hereCoords;"
		"thereCoords[axis] = bitReverse(thereCoords[axis]);"

		"if (thereCoords[axis] <= hereCoords[axis]) return;"
		
		"vec4 fromHere = imageLoad(image, hereCoords);"
		"vec4 fromThere = imageLoad(image, thereCoords);"
		"memoryBarrier();"
		"imageStore(image, hereCoords, fromThere);"
		"imageStore(image, thereCoords, fromHere);"
		, ShadeOpts().scope("bitReverse").enableResult(false).enableWrite(false),
		R"END(
		int bitReverse(int n) {
			int reversedN = n;
			int count = bits - 1;
 
			n >>= 1;
			while (n > 0) {
				reversedN = (reversedN << 1) | (n & 1);
				count--;
				n >>= 1;
			}
 
			return ((reversedN << count) & ((1 << bits) - 1));
		}
		)END"
	);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	for (int problemSize = 2; problemSize <= N; problemSize *= 2) {
		/*for (int i = 0; i < N; i += problemSize) {
			fftGpuOneAxis(in, i, i + problemSize, axis, expSign);
		}
		*/
		fftGpuOneAxis(in, problemSize, axis, expSign);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
}

void myFFT::fftGpu(gl::TextureRef& in, Dir dir)
{
	//cout << "GPU ================= " << endl;
	beginRTT(nullptr);
	myFFT::Scalar expSign = dir == Dir::Forward ? -1.0 : 1.0;
	breadthFirst(in, 0, expSign);
	breadthFirst(in, 1, expSign);
	glBindImageTexture(0, in->getId(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32F);
	shade2(in,
		"ivec2 fragCoord = ivec2(gl_FragCoord.xy);"
		"vec2 loaded = imageLoad(image, fragCoord).xy;"
		"vec2 toStore = loaded / sqrt(texSize.x*texSize.y);"
		"memoryBarrier();"
		"imageStore(image, fragCoord, vec4(toStore, 0, 0));"
		, ShadeOpts().enableResult(false).enableWrite(false)
	);
	endRTT();
}

// #########################################################
// verified that splitInTwo and mergeFromTwo work fine (in RDoc). Even by doing a diff.
// `separate` too. `wo` too.

// About "count":
//		261632/1022=256 so the number of calls in my (GLSL) version is correct

// Observation: all the 1x256 (and such) things look kinda like a 1->0 big gradient. But that's because the sky is brighter than the buildings.

// Don't forget this:
//		"layout(origin_upper_left) in vec4 gl_FragCoord;"


/*
post-merging prints of `result`:
	GPU ================ =
	[0.085, 0.000][0.056, 0.000]
	CPU ================ =
	(0.0846252, 0) (0.0265808, 0)
*/

/*  prints of `w`: (till N==4) // NO PROBLEM HERE
	GPU =================
	[    1.000,   -0.000]
	[    1.000,   -0.000]
	[    1.000,   -0.000] [    0.000,   -1.000]
	CPU =================
	(1,-0)
	(1,-0)
	(1,-0) (-8.74228e-08,-1)
*/

/* prints of `wo`: (till N==4) // BROKEN
	GPU =================
	[    0.029,    0.000] // correct
	[    0.099,    0.000] // correct
	[    0.212,    0.000] [    0.000,   -0.113] // error
	CPU =================
	(0.0290222,0)
	(0.0994263,0)
	(0.212219,0) (-1.16855e-09,-0.0133667)
*/

/* prints of `e`: (till N==4) // BROKEN
	GPU =================
	[    0.056,    0.000]
	[    0.113,    0.000]
	[    0.085,    0.000] [    0.056,    0.000] <---------- big error here
	CPU =================
	(0.055603,0)
	(0.112793,0)
	(0.0846252,0) (0.0265808,0)
*/

/*
pre-summing merge (e and o): (till N=4) // BROKEN
	GPU =================
	[    0.056,    0.000] [    0.029,    0.000] // correct
	[    0.113,    0.000] [    0.099,    0.000] // correct
	[    0.085,    0.000] [    0.056,    0.000] [    0.212,    0.000] [    0.113,    0.000] <---- error
	CPU =================
	(0.055603,0) (0.0290222,0)
	(0.112793,0) (0.0994263,0)
	(0.0846252,0) (0.0265808,0) (0.212219,0) (0.0133667,0)
*/