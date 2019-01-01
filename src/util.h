/*
Tonemaster - HDR software
Copyright (C) 2018 Stefan Monov <logixoul@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#pragma once

#include "precompiled.h"

#define MULTILINE(...) #__VA_ARGS__

#define COUT_(a) cout << #a << " = " << a << endl;
inline string esc_macro_helper(string s) { return s.substr(1, s.length()-2); }
#define ESC_(s) esc_macro_helper(string(#s))

typedef unsigned char byte;

template<class T>
class ArrayDeleter
{
public:
	ArrayDeleter(T* arrayPtr)
	{
		refcountPtr = new int(0);
		(*refcountPtr)++;

		this->arrayPtr = arrayPtr;
	}

	ArrayDeleter(ArrayDeleter const& other)
	{
		arrayPtr = other.arrayPtr;
		refcountPtr = other.refcountPtr;
		(*refcountPtr)++;
	}

	ArrayDeleter const& operator=(ArrayDeleter const& other)
	{
		reduceRefcount();

		arrayPtr = other.arrayPtr;
		refcountPtr = other.refcountPtr;
		(*refcountPtr)++;
		
		return *this;
	}

	~ArrayDeleter()
	{
		reduceRefcount();
	}

private:
	void reduceRefcount()
	{
		(*refcountPtr)--;
		if(*refcountPtr == 0)
		{
			delete refcountPtr;
			//delete[] array;
			fftwf_free(arrayPtr);
		}
	}
	
	int* refcountPtr;
	T* arrayPtr;
};

enum nofill {};

template<class T>
struct Array2D;

typedef glm::tvec3<byte> bytevec3;

void copyCvtData(ci::Surface8u const& surface, Array2D<bytevec3> dst);
void copyCvtData(ci::Surface8u const& surface, Array2D<vec3> dst);
void copyCvtData(ci::SurfaceT<float> const& surface, Array2D<vec3> dst);
void copyCvtData(ci::SurfaceT<float> const& surface, Array2D<float> dst);
void copyCvtData(ci::ChannelT<float> const& surface, Array2D<float> dst);

template<class T>
struct Array2D
{
	T* data;
	typedef T value_type;
	int area;
	int w, h;
	int NumBytes() const {
		return area * sizeof(T);
	}
	ci::ivec2 Size() const { return ci::ivec2(w, h); }
	ArrayDeleter<T> deleter;

	Array2D(int w, int h, nofill) : deleter(Init(w, h)) { }
	Array2D(ivec2 s, nofill) : deleter(Init(s.x, s.y)) { }
	Array2D(int w, int h, T const& defaultValue = T()) : deleter(Init(w, h)) { fill(defaultValue); }
	Array2D(ivec2 s, T const& defaultValue = T()) : deleter(Init(s.x, s.y)) { fill(defaultValue); }
	Array2D() : deleter(Init(0, 0)) { }
	
	Array2D(ci::Surface8u const& surface) : deleter(Init(surface.getWidth(), surface.getHeight()))
	{
		::copyCvtData(surface, *this);
	}

	Array2D(ci::SurfaceT<float> const& surface) : deleter(Init(surface.getWidth(), surface.getHeight()))
	{
		::copyCvtData(surface, *this);
	}

	Array2D(ci::ChannelT<float> const& surface) : deleter(Init(surface.getWidth(), surface.getHeight()))
	{
		::copyCvtData(surface, *this);
	}
	
#ifdef OPENCV_CORE_HPP
	template<class TSrc>
	Array2D(cv::Mat_<TSrc> const& mat) : deleter(nullptr)
	{
		Init(mat.cols, mat.rows, (T*)mat.data);
	}
#endif

	T* begin() { return data; }
	T* end() { return data+w*h; }
	T const* begin() const { return data; }
	T const* end() const { return data+w*h; }
	
	T& operator()(int i) { return data[i]; }
	T const& operator()(int i) const { return data[i]; }

	T& operator()(int x, int y) { return data[offsetOf(x, y)]; }
	T const& operator()(int x, int y) const { return data[offsetOf(x, y)]; }

	T& operator()(ivec2 const& v) { return data[offsetOf(v)]; }
	T const& operator()(ivec2 const& v) const { return data[offsetOf(v)]; }
	
	ivec2 wrapPoint(ivec2 p)
	{
		ivec2 wp = p;
		wp.x %= w; if(wp.x < 0) wp.x += w;
		wp.y %= h; if(wp.y < 0) wp.y += h;
		return wp;
	}
	
	T& wr(int x, int y) { return wr(ivec2(x, y)); }
	T const& wr(int x, int y) const { return wr(ivec2(x, y)); }

	T& wr(ivec2 const& v) { return (*this)(wrapPoint(v)); }
	T const& wr(ivec2 const& v) const { return (*this)(wrapPoint(v)); }

	int offsetOf(int x, int y) const { return y * w + x; }
	int offsetOf(ci::ivec2 const& p) const { return p.y * w + p.x; }
	bool contains(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h; }
	bool contains(ivec2 const& p) const { return p.x >= 0 && p.y >= 0 && p.x < w && p.y < h; }

	int xStep() const { return offsetOf(1, 0) - offsetOf(0, 0); }
	int yStep() const { return offsetOf(0, 1) - offsetOf(0, 0); }

	Array2D clone() const {
		Array2D result(Size());
		std::copy(begin(), end(), result.begin());
		return result;
	}

private:
	void fill(T const& value)
	{
		std::fill(begin(), end(), value);
	}
	T* Init(int w, int h) {
		// fftwf_malloc so we can use "new-array execute" fftw functions
		auto data = (T*)fftwf_malloc(w * h * sizeof(T)); // data = new T[w * h]
		Init(w, h, data);
		return data;
	}
	void Init(int w, int h, T* data) {
		this->data = data;
		area = w * h;
		this->w = w;
		this->h = h;
	}
};

inline ivec2 imod(ivec2 a, ivec2 b)
{
	return ivec2(a.x % b.x, a.y % b.y);
}

inline void rotate(vec2& p, float angle)
{
    float c = cos(angle), s = sin(angle);
    p = vec2(p.x * c + p.y * (-s), p.x * s + p.y * c);
}

inline bool isnan_(float f) { return f!=f; }

inline void check(vec3 v)
{
	if(isnan_(v.x) || isnan_(v.y) || isnan_(v.z)) throw exception();
}

void trapFP();

template<class F> vec3 apply(vec3 const& v, F f)
{
	return vec3(f(v.x), f(v.y), f(v.z));
}

template<class F> vec3 apply(float val, F f)
{
	return f(val);
}

// todo rm this
/*

template<class F,class T> Vec3<T> apply(Vec3<T> const& v, F f)
{
return Vec3<T>(f(v.x), f(v.y), f(v.z));
}

*/

template<class F> float apply(float v, F f)
{
	return f(v);
}

// not sure if the following is needed. it causes a macro redefinition warning.
// commenting it out until I've seen whether removing it breaks my projs.
// #define forxy(w, h) for(int i = 0; i < w; i++) for(int j = 0; j < h; j++)
#define forxy(image) \
	for(ivec2 p(0, 0); p.y < image.h; p.y++) \
		for(p.x = 0; p.x < image.w; p.x++)

inline float psin(float a)
{
	return sin(a)*0.5f + 0.5f;
}

const float pi = 3.14159265f;
const float twoPi = 2.0f * 3.14159265f;
const float halfPi = .5f * 3.14159265f;

void loadFile(std::vector<unsigned char>& buffer, const std::string& filename);

// todo rm this
/*template<class T>
T min(ci::Vec3<T> vec)
{
	return std::min(vec.x, std::min(vec.y, vec.z));
}

template<class T>
T max(ci::Vec3<T> vec)
{
	return std::max(vec.x, std::max(vec.y, vec.z));
}*/

template<class T>
T Parse(string const& src)
{
	std::istringstream iss(src);
	T t;
	iss >> t;
	return t;
}

template <typename T> int sgn(T val)
{
    return (val > T(0)) - (val < T(0));
}

float linearstep(float edge0, float edge1, float x);

void createConsole();

template<class T>
struct ListOf
{
	vector<T> data;
	ListOf(T t)
	{
		data.push_back(t);
	}
	ListOf<T>& operator()(T t)
	{
		data.push_back(t);
		return *this;
	}
	operator vector<T>()
	{
		return data;
	}
};

template<class T>
ListOf<T> list_of(T t)
{
	return ListOf<T>(t);
}

template<class T>
Array2D<T> empty_like(Array2D<T> a) {
	return Array2D<T>(a.Size(), nofill());
}

template<class T>
Array2D<T> ones_like(Array2D<T> a) {
	return Array2D<T>(a.Size(), 1.0f);
}

template<class T>
Array2D<T> zeros_like(Array2D<T> a) {
	return Array2D<T>(a.Size(), ::zero<T>());
}

#define STRING(...) #__VA_ARGS__