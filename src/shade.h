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
#include "util.h"
#include "TextureCache.h"

struct GpuScope {
	GpuScope(string name);
	~GpuScope();
};

#define GPU_SCOPE(name) GpuScope gpuScope_ ## __LINE__  (name);

void beginRTT(gl::TextureRef fbotex);
void endRTT();

void drawRect();

struct Str {
	string s;
	Str& operator<<(string s2) {
		s += s2 + "\n";
		return *this;
	}
	Str& operator<<(Str s2) {
		s += s2.s + "\n";
		return *this;
	}
	operator std::string() {
		return s;
	}
};

extern std::map<string, float> globaldict;
extern std::map<string, int> globaldictI;
void globaldict_default(string s, float f);
template<class T>
struct optional {
	T val;
	bool exists;
	optional(T const& t) { val=t; exists=true; }
	optional() { exists=false; }
};
struct ShadeOpts
{
	ShadeOpts();
	ShadeOpts& ifmt(GLenum val) { _ifmt=val; return *this; }
	ShadeOpts& scale(float val) { _scaleX=val; _scaleY=val; return *this; }
	ShadeOpts& scale(float valX, float valY) { _scaleX=valX; _scaleY=valY; return *this; }
	ShadeOpts& texCache(TextureCache* val) { _texCache = val; return *this; }
	ShadeOpts& operator()(TextureCache* val) { _texCache = val; return *this; }
	ShadeOpts& texCache(TextureCache& val) { _texCache = &val; return *this; }
	ShadeOpts& operator()(TextureCache& val) { _texCache = &val; return *this; }
	ShadeOpts& scope(std::string name) { _scopeName = name; return *this; }
	ShadeOpts& targetTex(gl::TextureRef val) { _targetTex = val; return *this; }
	ShadeOpts& dstPos(ivec2 val) { _dstPos = val; return *this; }
	ShadeOpts& dstRectSize(ivec2 val) { _dstRectSize = val; return *this; }
	ShadeOpts& srcArea(Area val) {
		_area = val; return *this;
	}
	ShadeOpts& callback(function<void(gl::GlslProgRef)> val) {
		_callback = val; return *this;
	}
	ShadeOpts& enableResult(bool val) {
		_enableResult = val; return *this;
	}
	ShadeOpts& enableWrite(bool val) {
		_enableWrite = val; return *this;
	}
	ShadeOpts& fetchUpsideDown(bool val) {
		_fetchUpsideDown = val; return *this;
	}

	optional<GLenum> _ifmt;
	float _scaleX, _scaleY;
	TextureCache* _texCache = nullptr;
	std::string _scopeName;
	gl::TextureRef _targetTex = nullptr;
	Area _area = Area::zero();
	ivec2 _dstPos;
	ivec2 _dstRectSize;
	function<void(gl::GlslProgRef)> _callback = [&](gl::GlslProgRef) {};
	bool _enableResult = true;
	bool _enableWrite = true; // todo: rm this?
	bool _fetchUpsideDown = false; // todo: rm this
};

gl::TextureRef shade(vector<gl::TextureRef> const& texv, std::string const& fshader, ShadeOpts const& opts=ShadeOpts());
inline gl::TextureRef shade(vector<gl::TextureRef> const& texv, std::string const& fshader, float resScale);
