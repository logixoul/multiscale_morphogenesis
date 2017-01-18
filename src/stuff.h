#pragma once
#include "precompiled.h"
namespace gl=ci::gl;

template<class T>
class FETCHFUNCTRAITS {
public:
	typedef T&(*type)(Array2D<T>&,int,int);
	template<class T>
	static T& defaultImpl(Array2D<T>& src, int x, int y) {
		return get_clamped(src, x, y);
	}
};

const GLenum hdrFormat = GL_RGBA16F;
inline void gotoxy(int x, int y) { 
    COORD pos = {x, y};
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorPosition(output, pos);
}
inline void clearconsole() {
    COORD topLeft  = { 0, 0 };
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
    DWORD written;

    GetConsoleScreenBufferInfo(console, &screen);
    FillConsoleOutputCharacterA(
        console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    FillConsoleOutputAttribute(
        console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
        screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    SetConsoleCursorPosition(console, topLeft);
}
////////////////
#if 0
template<class Func>
struct ToBind
{
	/*function<Func> f;
	ToBind(Func f) : f(f){
	}*/
};

template<class Func>
ToBind<Func> toBind(Func func) {
	throw 0;
	//return ToBind<Func>(func);
}

void test()
{
	toBind(powf);
}
#endif

template<class F>
struct Transformed {
	F f;
	Transformed(F f) : f(f)
	{
	}
};

template<class Range, class F>
auto operator|(Array2D<Range> r, Transformed<F> transformed_) -> Array2D<decltype(transformed_.f(r(0, 0)))>
{
	Array2D<decltype(transformed_.f(r(0, 0)))> r2(r.w, r.h);
	forxy(r2)
	{
		r2(p) = transformed_.f(r(p));
	}
	return r2;
}

template<class F>
Transformed<F> transformed(F f)
{
	return Transformed<F>(f);
}

template<class T>
Vec2i wrapPoint(Array2D<T> const& src, Vec2i p)
{
	Vec2i wp = p;
	wp.x %= src.w; if(wp.x < 0) wp.x += src.w;
	wp.y %= src.h; if(wp.y < 0) wp.y += src.h;
	return wp;
}
template<class T>
T const& getWrapped(Array2D<T> const& src, Vec2i p)
{
	return src(wrapPoint(src, p));
}
template<class T>
T const& getWrapped(Array2D<T> const& src, int x, int y)
{
	return getWrapped(src, Vec2i(x, y));
}
template<class T>
T& getWrapped(Array2D<T>& src, Vec2i p)
{
	return src(wrapPoint(src, p));
}
template<class T>
T& getWrapped(Array2D<T>& src, int x, int y)
{
	return getWrapped(src, Vec2i(x, y));
}
struct WrapModes {
	struct GetWrapped {
		template<class T>
		static T& fetch(Array2D<T>& src, int x, int y)
		{
			return ::getWrapped(src, x, y);
		}
	};
	struct Get_WrapZeros {
		template<class T>
		static T& fetch(Array2D<T>& src, int x, int y)
		{
			return ::get_wrapZeros(src, x, y);
		}
	};
	struct NoWrap {
		template<class T>
		static T& fetch(Array2D<T>& src, int x, int y)
		{
			return src(x, y);
		}
	};
	typedef GetWrapped DefaultImpl;
};
template<class T, class FetchFunc>
Array2D<T> blur(Array2D<T>& src, int r, T zero = T())
{
	Array2D<T> newImg(src.w, src.h);
	float divide = pow(2.0f * r + 1.0f, 2.0f);
	for(int x = 0; x < src.w; x++)
	{
		T sum = zero;
		for(int y = -r; y <= r; y++)
		{
			sum += FetchFunc::fetch(src, x, y);
		}
		for(int y = 0; y < src.h; y++)
		{
			newImg(x, y) = sum;
			sum -= FetchFunc::fetch(src, x, y - r);
			sum += FetchFunc::fetch(src, x, y + r + 1);
		}
	}
	Array2D<T> newImg2(src.w, src.h);
	for(int y = 0; y < src.h; y++)
	{
		T sum = zero;
		for(int x = -r; x <= r; x++)
		{
			sum += FetchFunc::fetch(newImg, x, y);
		}
		for(int x = 0; x < src.w; x++)
		{
			newImg2(x, y) = sum;
			sum -= FetchFunc::fetch(newImg, x - r, y);
			sum += FetchFunc::fetch(newImg, x + r + 1, y);
		}
	}
	forxy(img)
		newImg2(p) /= divide;
	return newImg2;
}
template<class T>
Array2D<T> blur(Array2D<T>& src, int r, T zero = T())
{
	return blur<T, WrapModes::DefaultImpl>(src, r, zero);
}
template<class T, class FetchFunc>
void blurFaster_helper1d(Array2D<T>& arr, int r, Vec2i startXY, Vec2i endXY, Vec2i stepXY)
{
	// init stuff
	T zero = ::zero<T>();
	int d = 2 * r + 1;
	float normMul = 1.0f / float(d);
	
	// setup buffer
	struct BufferEntry
	{
		T val;
		BufferEntry* next;
	};
	vector<BufferEntry> buffer(d);
	for(int i = 0; i < buffer.size(); i++)
	{
		if(i == buffer.size() - 1)
		{
			buffer[i].next = &buffer[0];
			continue;
		}
		buffer[i].next = &buffer[i+1];
	}
	
	// fill beginning of buffer
	BufferEntry* b = &buffer[0];
	for(int i = -r; i <= r; i++)
	{
		Vec2i fetchPos = startXY + i * stepXY;
		b->val = FetchFunc::fetch(arr, fetchPos.x, fetchPos.y);
		b = b->next;
	}
	
	// init sum
	T sum=zero;
	foreach(auto& entry, buffer)
	{
		sum += entry.val;
	}
	
	// do main work
	BufferEntry* oldest = &buffer[0];
	BufferEntry* newest = &buffer[buffer.size() - 1];
	//const T* startPtr = &arr(startXY);
	//int ptrStep = &arr(stepXY) - &arr(0, 0);
	//T* outP = startPtr - r * ptrStep;
	Vec2i fetchOffset = stepXY * (r + 1);
	Vec2i outP;
	for(outP = startXY; outP + fetchOffset != endXY; outP += stepXY) {
		arr(outP) = sum * normMul;
		sum -= oldest->val;
		oldest = oldest->next;
		newest = newest->next;
		Vec2i fetchPos = outP + fetchOffset;
		newest->val = WrapModes::NoWrap::fetch(arr, fetchPos.x, fetchPos.y);
		sum += newest->val;
	}

	// do last part
	for(/*outP already initted*/; outP != endXY; outP += stepXY) {
		arr(outP) = sum * normMul;
		sum -= oldest->val;
		oldest = oldest->next;
		newest = newest->next;
		Vec2i fetchPos = outP + fetchOffset;
		newest->val = FetchFunc::fetch(arr, fetchPos.x, fetchPos.y);
		sum += newest->val;
	}
}
template<class T, class FetchFunc>
Array2D<T> blurFaster(Array2D<T>& src, int r, T zero = T())
{
	auto newImg = src.clone();
	// blur columns
	for(int x = 0; x < src.w; x++)
	{
		blurFaster_helper1d<T, FetchFunc>(newImg, r,
			/*startXY*/Vec2i(x, 0),
			/*endXY*/Vec2i(x, src.h),
			/*stepXY*/Vec2i(0, 1));
	}
	// blur rows
	for(int y = 0; y < src.h; y++)
	{
		blurFaster_helper1d<T, FetchFunc>(newImg, r,
			/*startXY*/Vec2i(0, y),
			/*endXY*/Vec2i(src.w, y),
			/*stepXY*/Vec2i(1, 0));
	}
	return newImg;
}
/*template<class T>
Array2D<T> blur2(Array2D<T> const& src, int r)
{
	T zero = ::zero<T>();
	Array2D<T> newImg(sx, sy);
	float mul = 1.0f/pow(2.0f * r + 1.0f, 2.0f);
	auto blur1d = [&](int imax, T* p0, T* pEnd, int step) {
		vector<T> buffer;
		int windowSize = 2*r+1;
		buffer.resize(windowSize, zero);
		T* back = &buffer[0];
		T* front = &buffer[buffer.size()-1];
		T* dst=p0;
		for(int i = 0; i <= r; i++)
		{
			*(back+i+r)=*dst;
			dst += step;
		}
		dst=p0;
		T sum=zero;
		T* rStep=r*step;
		for(dst=p0; dst!=pEnd; dst+=step)
		{
			*dst=sum;
			sum-=*back;
			front++;if(front==&*buffer.end())front=&buffer[0];
			back++;if(back==&*buffer.end())back=&buffer[0];
			*front=*(dst+rStep);
			sum+=*front;
		}
	};

	for(int x = 0; x < sx; x++)
	{
		T sum = zero;
		for(int y = 0; y <= r; y++)
		{
			sum += src(x, y);
		}
		for(int y = 0; y < sy; y++)
		{
			newImg(x, y) = sum;
			sum -= get_wrapZeros(src, x, y - r);
			sum += get_wrapZeros(src, x, y + r + 1);
		}
	}
	Array2D<T> newImg2(sx, sy);
	for(int y = 0; y < sy; y++)
	{
		T sum = zero;
		for(int x = 0; x <= r; x++)
		{
			sum += newImg(x, y);
		}
		for(int x = 0; x < sx; x++)
		{
			newImg2(x, y) = sum;
			sum -= get_wrapZeros(newImg, x - r, y);
			sum += get_wrapZeros(newImg, x + r + 1, y);
		}
	}
	forxy(img)
		newImg2(p) *= mul;
	return newImg2;
}*/
template<class T>
void aaPoint_i2(Array2D<T>& dst, Vec2i p, T value)
{
	if(dst.contains(p))
		dst(p) += value;
}
template<class T>
void aaPoint_i2(Array2D<T>& dst, int x, int y, T value)
{
	if(dst.contains(x, y))
		dst(p) += value;
}
template<class T>
void aaPoint2(Array2D<T>& dst, Vec2f p, T value)
{
	aaPoint2(dst, p.x, p.y, value);
}
template<class T>
void aaPoint2(Array2D<T>& dst, float x, float y, T value)
{
	int ix = x, iy = y;
	float fx = ix, fy = iy;
	if(x < 0.0f && fx != x) { fx--; ix--; }
	if(y < 0.0f && fy != y) { fy--; iy--; }
	float fractx = x - fx;
	float fracty = y - fy;
	float fractx1 = 1.0 - fractx;
	float fracty1 = 1.0 - fracty;
	get2(dst, ix, iy) += (fractx1 * fracty1) * value;
	get2(dst, ix, iy+1) += (fractx1 * fracty) * value;
	get2(dst, ix+1, iy) += (fractx * fracty1) * value;
	get2(dst, ix+1, iy+1) += (fractx * fracty) * value;
}
template<class T>
void aaPoint2_fast(Array2D<T>& dst, Vec2f p, T const& value)
{
	aaPoint2_fast(dst, p.x, p.y, value);
}
inline void my_assert_func(bool isTrue, string desc) {
	if(!isTrue) {
		cout << "assert failure: " << desc << endl;
		system("pause");
		throw std::runtime_error(desc.c_str());
	}
}
#define my_assert(isTrue) my_assert_func(isTrue, #isTrue);
//#define AAPOINT_DEBUG
template<class T>
void aaPoint2_fast(Array2D<T>& dst, float x, float y, T const& value)
{
	int ix = x, iy = y;
	float fx = ix, fy = iy;
#ifdef AAPOINT_DEBUG
	my_assert(x>=0.0f);
	my_assert(y>=0.0f);
	my_assert(ix+1<=dst.w-1);
	my_assert(iy+1<=dst.h-1);
#endif
	//if(x < 0.0f && fx != x) { fx--; ix--; }
	//if(y < 0.0f && fy != y) { fy--; iy--; }
	float fractx = x - fx;
	float fracty = y - fy;
	//float fractx1 = 1.0 - fractx;
	//float fracty1 = 1.0 - fracty;
	
	T partB = fracty*value;
	T partT = value - partB;
	T partBR = fractx * partB;
	T partBL = partB - partBR;
	T partTR = fractx * partT;
	T partTL = partT - partTR;
	auto basePtr = &dst(ix, iy);
	basePtr[0] += partTL;
	basePtr[1] += partTR;
	basePtr[dst.w] += partBL;
	basePtr[dst.w+1] += partBR;
}
template<class T>
T& get2(Array2D<T>& src, int x, int y)
{
	static T t;
	if(src.contains(x, y))
	{
		return src(x, y);
	}
	else
	{
		return t;
	}
}
template<class T>
void aaPoint_i(Array2D<T>& dst, Vec2i p, T value)
{
	dst.wr(p) += value;
}
template<class T>
void aaPoint_i(Array2D<T>& dst, int x, int y, T value)
{
	dst.wr(x, y) += value;
}
template<class T, class FetchFunc>
void aaPoint(Array2D<T>& dst, Vec2f p, T value)
{
	aaPoint<T, FetchFunc>(dst, p.x, p.y, value);
}
template<class T, class FetchFunc>
void aaPoint(Array2D<T>& dst, float x, float y, T value)
{
	int ix = x, iy = y;
	float fx = ix, fy = iy;
	if(x < 0.0f && fx != x) { fx--; ix--; }
	if(y < 0.0f && fy != y) { fy--; iy--; }
	float fractx = x - fx;
	float fracty = y - fy;
	float fractx1 = 1.0 - fractx;
	float fracty1 = 1.0 - fracty;
	FetchFunc::fetch(dst, ix, iy) += (fractx1 * fracty1) * value;
	FetchFunc::fetch(dst, ix, iy+1) += (fractx1 * fracty) * value;
	FetchFunc::fetch(dst, ix+1, iy) += (fractx * fracty1) * value;
	FetchFunc::fetch(dst, ix+1, iy+1) += (fractx * fracty) * value;
}
template<class T>
void aaPoint(Array2D<T>& dst, float x, float y, T value)
{
	aaPoint<T, WrapModes::DefaultImpl>(dst, p.x, p.y, value);
}
template<class T>
void aaPoint(Array2D<T>& dst, Vec2f p, T value)
{
	aaPoint<T, WrapModes::DefaultImpl>(dst, p, value);
}
template<class T, class FetchFunc>
T getBilinear(Array2D<T> src, Vec2f p)
{
	return getBilinear<T, FetchFunc>(src, p.x, p.y);
}
template<class T, class FetchFunc>
T getBilinear(Array2D<T> src, float x, float y)
{
	int ix = x, iy = y;
	float fx = ix, fy = iy;
	if(x < 0.0f && fx != x) { fx--; ix--; }
	if(y < 0.0f && fy != y) { fy--; iy--; }
	float fractx = x - fx;
	float fracty = y - fy;
	return lerp(
		lerp(FetchFunc::fetch(src, ix, iy), FetchFunc::fetch(src, ix + 1, iy), fractx),
		lerp(FetchFunc::fetch(src, ix, iy + 1), FetchFunc::fetch(src, ix + 1, iy + 1), fractx),
		fracty);
}
template<class T>
T getBilinear(Array2D<T> src, float x, float y)
{
	return getBilinear<T, WrapModes::DefaultImpl>(src, x, y);
}
template<class T>
T getBilinear(Array2D<T> src, Vec2f p)
{
	return getBilinear<T, WrapModes::DefaultImpl>(src, p);
}

inline gl::Texture gtex(Array2D<float> a)
{
	gl::Texture::Format fmt;
	fmt.setInternalFormat(hdrFormat);
	gl::Texture tex(a.w, a.h, fmt);
	tex.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_LUMINANCE, GL_FLOAT, a.data);
	return tex;
}
inline gl::Texture gtex(Array2D<Vec2f> a)
{
	gl::Texture::Format fmt;
	fmt.setInternalFormat(hdrFormat);
	gl::Texture tex(a.w, a.h, fmt);
	tex.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RG, GL_FLOAT, a.data);
	return tex;
}
inline gl::Texture gtex(Array2D<Vec3f> a)
{
	gl::Texture::Format fmt;
	fmt.setInternalFormat(hdrFormat);
	gl::Texture tex(a.w, a.h, fmt);
	tex.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RGB, GL_FLOAT, a.data);
	return tex;
}
inline Array2D<float> to01(Array2D<float> a) {
	auto minn = *std::min_element(a.begin(), a.end());
	auto maxx = *std::max_element(a.begin(), a.end());
	auto b = a.clone();
	forxy(b) {
		b(p) -= minn;
		b(p) /= (maxx - minn);
	}
	return b;
}
template<class T>
T& zero() {
	static T val=T()*0.0f;
	val = T()*0.0f;
	return val;
}

inline Vec2i clampPoint(Vec2i p, int w, int h)
{
	Vec2i wp = p;
	if(wp.x < 0) wp.x = 0;
	if(wp.x > w-1) wp.x = w-1;
	if(wp.y < 0) wp.y = 0;
	if(wp.y > h-1) wp.y = h-1;
	return wp;
}
template<class T>
T& get_clamped(Array2D<T>& src, int x, int y)
{
	return src(clampPoint(Vec2i(x, y), src.w, src.h));
}
template<class T>
T const& get_clamped(Array2D<T> const& src, int x, int y)
{
	return src(clampPoint(Vec2i(x, y), src.w, src.h));
}

template<class T>
Array2D<T> gauss3(Array2D<T> src) {
	T zero=::zero<T>();
	Array2D<T> dst1(src.w, src.h);
	Array2D<T> dst2(src.w, src.h);
	forxy(dst1)
		dst1(p) = .25f * (2 * get_clamped(src, p.x, p.y) + get_clamped(src, p.x-1, p.y) + get_clamped(src, p.x+1, p.y));
	forxy(dst2)
		dst2(p) = .25f * (2 * get_clamped(dst1, p.x, p.y) + get_clamped(dst1, p.x, p.y-1) + get_clamped(dst1, p.x, p.y+1));
	return dst2;
}

inline int sign(float f)
{
	if(f < 0)
		return -1;
	if(f > 0)
		return 1;
	return 0;
}
inline float expRange(float x, float min, float max) {
	return exp(lerp(log(min),log(max), x));
}

inline float niceExpRangeX(float mouseX, float min, float max) {
	float x2=sign(mouseX)*std::max(0.0f,abs(mouseX)-40.0f/(float)App::get()->getWindowWidth());
	return sign(x2)*expRange(abs(x2), min, max);
}

inline float niceExpRangeY(float mouseY, float min, float max) {
	float y2=sign(mouseY)*std::max(0.0f,abs(mouseY)-40.0f/(float)App::get()->getWindowHeight());
	return sign(y2)*expRange(abs(y2), min, max);
}

template<class Func>
class MapHelper {
private:
	static Func* func;
public:
	typedef typename decltype((*func)(Vec2i(0, 0))) result_dtype;
};

template<class TSrc, class Func>
auto map(Array2D<TSrc> a, Func func) -> Array2D<typename MapHelper<Func>::result_dtype> {
	auto result = Array2D<typename MapHelper<Func>::result_dtype>(a.w, a.h);
	forxy(a) {
		result(p) = func(p);
	}
	return result;
}

template<class T>
Vec2f gradient_i2(Array2D<T> src, Vec2i p)
{
	T nbs[3][3];
	for(int x = -1; x <= 1; x++) {
		for(int y = -1; y <= 1; y++) {
			nbs[x+1][y+1] = get_clamped(src, p.x + x, p.y + y);
		}
	}
	Vec2f gradient;
	T aTL = nbs[0][0];
	T aTC = nbs[1][0];
	T aTR = nbs[2][0];
	T aML = nbs[0][1];
	T aMR = nbs[2][1];
	T aBL = nbs[0][2];
	T aBC = nbs[1][2];
	T aBR = nbs[2][2];
	// removed '2' distance-denominator for backward compat for now
	float norm = 1.0f/16.f; //1.0f / 32.0f;
	gradient.x = ((3.0f * aTR + 10.0f * aMR + 3.0f * aBR) - (3.0f * aTL + 10.0f * aML + 3.0f * aBL)) * norm;
	gradient.y = ((3.0f * aBL + 10.0f * aBC + 3.0f * aBR) - (3.0f * aTL + 10.0f * aTC + 3.0f * aTR)) * norm;
	//gradient.x = get_clamped(src, p.x + 1, p.y) - get_clamped(src, p.x - 1, p.y);
	//gradient.y = get_clamped(src, p.x, p.y + 1) - get_clamped(src, p.x, p.y - 1);
	return gradient;
}

inline void mm(Array2D<float> arr, string desc="") {
	if(desc!="") {
		cout << "[" << desc << "] ";
	}
	cout<<"min: " << *std::min_element(arr.begin(),arr.end()) << ", ";
	cout<<"max: " << *std::max_element(arr.begin(),arr.end()) << endl;
}
//get_wrapZeros
template<class T>
T& get_wrapZeros(Array2D<T>& src, int x, int y)
{
	if(x < 0 || y < 0 || x >= src.w || y >= src.h)
	{
		return zero<T>();
	}
	return src(x, y);
}
template<class T>
T const& get_wrapZeros(Array2D<T> const& src, int x, int y)
{
	if(x < 0 || y < 0 || x >= src.w || y >= src.h)
	{
		return zero<T>();
	}
	return src(x, y);
}
template<class T, class FetchFunc>
Vec2f gradient_i(Array2D<T> src, Vec2i p)
{
	//if(p.x<1||p.y<1||p.x>=src.w-1||p.y>=src.h-1)
	//	return Vec2f::zero();
	Vec2f gradient;
	gradient.x = (FetchFunc::fetch(src,p.x + 1, p.y) - FetchFunc::fetch(src, p.x - 1, p.y)) / 2.0f;
	gradient.y = (FetchFunc::fetch(src,p.x, p.y + 1) - FetchFunc::fetch(src, p.x, p.y - 1)) / 2.0f;
	return gradient;
}
template<class T, class FetchFunc>
Vec2f gradient_i_nodiv(Array2D<T> src, Vec2i p)
{
	//if(p.x<1||p.y<1||p.x>=src.w-1||p.y>=src.h-1)
	//	return Vec2f::zero();
	Vec2f gradient;
	gradient.x = FetchFunc::fetch(src,p.x + 1, p.y) - FetchFunc::fetch(src, p.x - 1, p.y);
	gradient.y = FetchFunc::fetch(src,p.x, p.y + 1) - FetchFunc::fetch(src, p.x, p.y - 1);
	return gradient;
}
template<class T, class FetchFunc>
Array2D<Vec2f> get_gradients(Array2D<T> src)
{
	auto src2=src.clone();
	forxy(src2)
		src2(p) /= 2.0f;
	Array2D<Vec2f> gradients(src.w, src.h);
	for(int x = 0; x < src.w; x++)
	{
		gradients(x, 0) = gradient_i_nodiv<T, FetchFunc>(src2, Vec2i(x, 0));
		gradients(x, src.h-1) = gradient_i_nodiv<T, FetchFunc>(src2, Vec2i(x, src.h-1));
	}
	for(int y = 1; y < src.h-1; y++)
	{
		gradients(0, y) = gradient_i_nodiv<T, FetchFunc>(src2, Vec2i(0, y));
		gradients(src.w-1, y) = gradient_i_nodiv<T, FetchFunc>(src2, Vec2i(src.w-1, y));
	}
	for(int x=1; x < src.w-1; x++)
	{
		for(int y=1; y < src.h-1; y++)
		{
			gradients(x, y) = gradient_i_nodiv<T, WrapModes::NoWrap>(src2, Vec2i(x, y));
		}
	}
	return gradients;
}
template<class T>
Array2D<Vec2f> get_gradients(Array2D<T> src)
{
	return get_gradients<T, WrapModes::DefaultImpl>(src);
}

inline gl::Texture maketex(int w, int h, GLint internalFormat) {
	gl::Texture::Format fmt; fmt.setInternalFormat(internalFormat); return gl::Texture(NULL, GL_RGBA, w, h, fmt);
}
inline gl::Texture maketex(Array2D<Vec3f> arr, GLint internalFormat) {
	gl::Texture::Format fmt; fmt.setInternalFormat(internalFormat);
	auto tex = gl::Texture(arr.w, arr.h, fmt);
	tex.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, arr.w, arr.h, GL_RGB, GL_FLOAT, arr.data);
	return tex;
}
inline gl::Texture maketex(Array2D<Vec2f> arr, GLint internalFormat) {
	gl::Texture::Format fmt; fmt.setInternalFormat(internalFormat);
	auto tex = gl::Texture(arr.w, arr.h, fmt);
	tex.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, arr.w, arr.h, GL_RG, GL_FLOAT, arr.data);
	return tex;
}
inline gl::Texture maketex(Array2D<float> arr, GLint internalFormat) {
	gl::Texture::Format fmt; fmt.setInternalFormat(internalFormat);
	auto tex = gl::Texture(arr.w, arr.h, fmt);
	tex.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, arr.w, arr.h, GL_LUMINANCE, GL_FLOAT, arr.data);
	return tex;
}

template<class T>
Array2D<T> gettexdata(gl::Texture tex, GLenum format, GLenum type) {
	return gettexdata<T>(tex, format, type, tex.getBounds());
}

inline void checkGLError(string place)
{
	GLenum errCode;
	if ((errCode = glGetError()) != GL_NO_ERROR) 
	{
		cout<<"GL error "<<hex<<errCode<<dec<< " at " << place << endl;
	}
	else
		cout << "NO error at " << place << endl;
}
#define MY_STRINGIZE_DETAIL(x) #x
#define MY_STRINGIZE(x) MY_STRINGIZE_DETAIL(x)
#define CHECK_GL_ERROR() checkGLError(__FILE__ ": " MY_STRINGIZE(__LINE__))

template<class T>
Array2D<T> gettexdata(gl::Texture tex, GLenum format, GLenum type, ci::Area area) {
	Array2D<T> data(area.getWidth(), area.getHeight());
	CHECK_GL_ERROR();
	tex.bind();
	//glGetTexImage(GL_TEXTURE_2D, 0, format, type, data.data);
	CHECK_GL_ERROR();
	beginRTT(tex);
	CHECK_GL_ERROR();
	glReadPixels(area.x1, area.y1, area.getWidth(), area.getHeight(), format, type, data.data);
	CHECK_GL_ERROR();
	endRTT();
	CHECK_GL_ERROR();
	//glGetTexS
	return data;
}
inline float sq(float f) {
	return f * f;
}

inline vector<float> getGaussianKernel(int ksize) { // ksize must be odd
	float sigma = 0.3*((ksize-1)*0.5 - 1) + 0.8;
	vector<float> result;
	int r=ksize/2;
	float sum=0.0f;
	for(int i=-r;i<=r;i++) {
		float exponent = -(i*i/sq(2*sigma));
		float val = exp(exponent);
		sum += val;
		result.push_back(val);
	}
	for(int i=0; i<result.size(); i++) {
		result[i] /= sum;
	}
	return result;
}
/*template<class T>
Array2D<T> gaussianBlur1D(Array2D<T> src, int ksize, int dim, vector<float> const& kernel) { // ksize must be odd
	int r = ksize / 2;
	
	int w = dim == 0 ? src.w : src.h,
	int h = dim == 0 ? src.h : src.w;
	auto blurPart = [&](int x0, int x1) {
		for(int y = 0; y < h; y++)
		{
			for(int x = x0; x < x1; x++)
			{
				T sum = zero;
				for(int i = -r; i <= r; i++)
				{
					if(x + i < 0 || x + i >= w)
						continue;
					sum += kernel[i + r] * src(x + i, y);
				}
				dst1(y, x) = sum;
			}
		}
	};

	blurPart(0, r);
	blurPart(w-r, w);
	for(int y = 0; y < h; y++)
	{
		for(int x = r; x < w-r-1; x++)
		{
			T sum = zero;
			for(int i = -r; i <= r; i++)
			{
				sum += kernel[i + r] * src(x + i, y);
			}
			dst1(x, y) = sum;
		}
	}
}*/

/*template<class T> void foo(T) {}
template<class T> struct Traits { typedef void(*type)(T); static void default_(T) {} };
template<class T, void(*Fetch)(T)=foo> void test() {}
void runMain() {test<int >(); }*/

template<class T,class FetchFunc>
Array2D<T> gaussianBlur(Array2D<T> src, int ksize) { // ksize must be odd. fastpath is for r%3==0.
	int r = ksize / 2;
	if(r % 3 == 0)
	{
		//cout << "BLUR fastpath" << endl;
		auto blurred = src;
		for(int i = 0; i < 3; i++)
		{
			blurred = blur<T, FetchFunc>(blurred, r / 3, ::zero<T>());
		}
		return blurred;
	}
	//cout << "BLUR slowpath" << endl;

	auto kernel = getGaussianKernel(ksize);
	T zero=::zero<T>();
	Array2D<T> dst1(src.w, src.h);
	Array2D<T> dst2(src.w, src.h);
	
	int w = src.w, h = src.h;
	
	// vertical

	auto runtime_fetch = FetchFunc::fetch<T>;
	for(int y = 0; y < h; y++)
	{
		auto blurVert = [&](int x0, int x1) {
			// guard against w<r
			x0 = max(x0, 0);
			x1 = min(x1, w);

			for(int x = x0; x < x1; x++)
			{
				T sum = zero;
				for(int xadd = -r; xadd <= r; xadd++)
				{
					sum += kernel[xadd + r] * (runtime_fetch(src, x + xadd, y));
				}
				dst1(x, y) = sum;
			}
		};

		
		blurVert(0, r);
		blurVert(w-r, w);
		for(int x = r; x < w-r; x++)
		{
			T sum = zero;
			for(int xadd = -r; xadd <= r; xadd++)
			{
				sum += kernel[xadd + r] * src(x + xadd, y);
			}
			dst1(x, y) = sum;
		}
	}
	
	// horizontal
	for(int x = 0; x < w; x++)
	{
		auto blurHorz = [&](int y0, int y1) {
			// guard against h<r
			y0 = max(y0, 0);
			y1 = min(y1, h);
			for(int y = y0; y < y1; y++)
			{
				T sum = zero;
				for(int yadd = -r; yadd <= r; yadd++)
				{
					sum += kernel[yadd + r] * runtime_fetch(dst1, x, y + yadd);
				}
				dst2(x, y) = sum;
			}
		};

		blurHorz(0, r);
		blurHorz(h-r, h);
		for(int y = r; y < h-r-1; y++)
		{
			T sum = zero;
			for(int yadd = -r; yadd <= r; yadd++)
			{
				sum += kernel[yadd + r] * dst1(x, y + yadd);
			}
			dst2(x, y) = sum;
		}
	}
	return dst2;
}
template<class T>
Array2D<T> gaussianBlur(Array2D<T> src, int ksize) {
	return gaussianBlur<T, WrapModes::DefaultImpl>(src, ksize);
}
struct denormal_check {
	static int num;
	static void begin_frame() {
		num = 0;
	}
	static void check(float f) {
		if ( f != 0 && fabsf( f ) < numeric_limits<float>::min() ) {
			// it's denormalized
			num++;
		}
	}
	static void end_frame() {
		cout << "denormals detected: " << num << endl;
	}
};

inline vector<Array2D<float> > split(Array2D<Vec3f> arr) {
	Array2D<float> r(arr.w, arr.h);
	Array2D<float> g(arr.w, arr.h);
	Array2D<float> b(arr.w, arr.h);
	forxy(arr) {
		r(p) = arr(p).x;
		g(p) = arr(p).y;
		b(p) = arr(p).z;
	}
	vector<Array2D<float> > result;
	result.push_back(r);
	result.push_back(g);
	result.push_back(b);
	return result;
}

inline Array2D<Vec3f> merge(vector<Array2D<float> > channels) {
	Array2D<float>& r = channels[0];
	Array2D<float>& g = channels[1];
	Array2D<float>& b = channels[2];
	Array2D<Vec3f> result(r.w, r.h);
	forxy(result) {
		result(p) = Vec3f(r(p), g(p), b(p));
	}
	return result;
}

inline Array2D<Vec3f> resize(Array2D<Vec3f> src, Vec2i dstSize, const ci::FilterBase &filter)
{
	ci::SurfaceT<float> tmpSurface(
		(float*)src.data, src.w, src.h, /*rowBytes*/sizeof(Vec3f) * src.w, ci::SurfaceChannelOrder::RGB);
	auto resizedSurface = ci::ip::resizeCopy(tmpSurface, tmpSurface.getBounds(), dstSize, filter);
	Array2D<Vec3f> resultArray = resizedSurface;
	return resultArray;
}

inline Array2D<float> resize(Array2D<float> src, Vec2i dstSize, const ci::FilterBase &filter)
{
#if 0
	ci::ChannelT<float> tmpSurface(
		(float*)src.data, src.w, src.h, /*rowBytes*/sizeof(float) * src.w, ci::SurfaceChannelOrder::ABGR);
	auto resizedSurface = ci::ip::resizeCopy(tmpSurface, tmpSurface.getBounds(), dstSize, filter);
	Array2D<float> resultArray = resizedSurface;
	return resultArray;
#endif
	auto srcRgb = ::merge(list_of(src)(src)(src));
	auto resized = resize(srcRgb, dstSize, filter);
	return ::split(resized)[0];
}

#undef FETCHARG

template<class T>
Array2D<T> longTailBlur(Array2D<T> src)
{
	auto state = src.clone();
	float sumw=1.0f;
	for(int i = 0; i < 10; i++)
	{
		auto srcb = gaussianBlur(src, i*2+1);
		sumw*=4.0f;
		sumw+=1.0f;
		forxy(state) {
			state(p) *= 4.0f;
			state(p) += srcb(p);
		}
	}
	forxy(state) {
		state(p) /= sumw;
	}
	return state;
}
	