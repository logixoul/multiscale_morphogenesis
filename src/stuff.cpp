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

#include "precompiled.h"
#include "stuff.h"
#include "gpgpu.h"
#include "TextureCache.h"

int denormal_check::num;

std::map<string,string> FileCache::db;

string FileCache::get(string filename) {
	if(db.find(filename)==db.end()) {
		//std::vector<unsigned char> buffer;
		auto dataSource = loadAsset(filename);
		auto buffer = dataSource->getBuffer();
		auto data = (char*)buffer->getData();
		string bufferStr(data, data + buffer->getSize());
		db[filename]=bufferStr;
	}
	return db[filename];
}

Array2D<vec3> resize(Array2D<vec3> src, ivec2 dstSize, const ci::FilterBase &filter)
{
	ci::SurfaceT<float> tmpSurface(
		(float*)src.data, src.w, src.h, /*rowBytes*/sizeof(vec3) * src.w, ci::SurfaceChannelOrder::RGB);
	auto resizedSurface = ci::ip::resizeCopy(tmpSurface, tmpSurface.getBounds(), dstSize, filter);
	Array2D<vec3> resultArray = resizedSurface;
	return resultArray;
}

Array2D<float> resize(Array2D<float> src, ivec2 dstSize, const ci::FilterBase &filter)
{
	ci::ChannelT<float> tmpSurface(
		src.w, src.h, /*rowBytes*/sizeof(float) * src.w, 1, src.data);
	ci::ChannelT<float> resizedSurface(dstSize.x, dstSize.y);
	ci::ip::resize(tmpSurface, &resizedSurface, filter);
	Array2D<float> resultArray = resizedSurface;
	return resultArray;
}

void my_assert_func(bool isTrue, string desc) {
	if (!isTrue) {
		cout << "assert failure: " << desc << endl;
		system("pause");
		throw std::runtime_error(desc.c_str());
	}
}

void bind(gl::TextureRef & tex) {
	glBindTexture(tex->getTarget(), tex->getId());
}
void bindTexture(gl::TextureRef & tex) {
	glBindTexture(tex->getTarget(), tex->getId());
}

void bindTexture(gl::TextureRef tex, GLenum textureUnit)
{
	glActiveTexture(textureUnit);
	bindTexture(tex);
	glActiveTexture(GL_TEXTURE0); // todo: is this necessary?
}

gl::TextureRef gtex(Array2D<float> a)
{
	gl::TextureRef tex = maketex(a.w, a.h, GL_R16F);
	bindTexture(tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RED, GL_FLOAT, a.data);
	return tex;
}

gl::TextureRef gtex(Array2D<vec2> a)
{
	gl::TextureRef tex = maketex(a.w, a.h, GL_RG16F);
	bind(tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RG, GL_FLOAT, a.data);
	return tex;
}

gl::TextureRef gtex(Array2D<vec3> a)
{
	gl::TextureRef tex = maketex(a.w, a.h, GL_RGB16F);
	bind(tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RGB, GL_FLOAT, a.data);
	return tex;
}
gl::TextureRef gtex(Array2D<bytevec3> a)
{
	gl::TextureRef tex = maketex(a.w, a.h, GL_RGB8);
	bind(tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RGB, GL_UNSIGNED_BYTE, a.data);
	return tex;
}

gl::TextureRef gtex(Array2D<vec4> a)
{
	gl::TextureRef tex = maketex(a.w, a.h, GL_RGBA16F);
	bind(tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RGBA, GL_FLOAT, a.data);
	return tex;
}

gl::TextureRef gtex(Array2D<uvec4> a)
{
	gl::Texture::Format fmt;
	fmt.setInternalFormat(GL_RGBA32UI);
	gl::TextureRef tex = gl::Texture2d::create(a.w, a.h, fmt);
	bind(tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a.w, a.h, GL_RGBA_INTEGER, GL_UNSIGNED_INT, a.data);
	return tex;
}

ivec2 clampPoint(ivec2 p, int w, int h)
{
	ivec2 wp = p;
	if (wp.x < 0) wp.x = 0;
	if (wp.x > w - 1) wp.x = w - 1;
	if (wp.y < 0) wp.y = 0;
	if (wp.y > h - 1) wp.y = h - 1;
	return wp;
}

int sign(float f)
{
	if (f < 0)
		return -1;
	if (f > 0)
		return 1;
	return 0;
}

float expRange(float x, float min, float max) {
	return exp(lerp(log(min), log(max), x));
}

float niceExpRangeX(float mouseX, float min, float max) {
	float x2 = sign(mouseX)*std::max(0.0f, abs(mouseX) - 40.0f / (float)App::get()->getWindowWidth());
	return sign(x2)*expRange(abs(x2), min, max);
}

float niceExpRangeY(float mouseY, float min, float max) {
	float y2 = sign(mouseY)*std::max(0.0f, abs(mouseY) - 40.0f / (float)App::get()->getWindowHeight());
	return sign(y2)*expRange(abs(y2), min, max);
}

void mm(Array2D<float> arr, string desc) {
	std::stringstream prepend;
	if (desc != "") {
		prepend << "[" << desc << "] ";
	}
	qDebug() << prepend.str() << "min: " << *std::min_element(arr.begin(), arr.end()) << ", "
		<< "max: " << *std::max_element(arr.begin(), arr.end());
}
void mm(Array2D<vec3> arr, string desc) {
	std::stringstream prepend;
	if (desc != "") {
		prepend << "[" << desc << "] ";
	}
	auto data = (float*)arr.data;
	qDebug() << prepend.str() << "min: " << *std::min_element(data, data + arr.area + 2) << ", "
		<< "max: " << *std::max_element(data, data + arr.area + 2);
}
void mm(Array2D<vec2> arr, string desc) {
	std::stringstream prepend;
	if (desc != "") {
		prepend << "[" << desc << "] ";
	}
	auto data = (float*)arr.data;
	qDebug() << prepend.str() << "min: " << *std::min_element(data, data + arr.area + 1) << ", "
		<< "max: " << *std::max_element(data, data + arr.area + 1);
}

vector<Array2D<float> > split(Array2D<vec3> arr) {
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
void setWrapBlack(gl::TextureRef tex) {
	// I think the border color is transparent black by default. It doesn't hurt that it is transparent.
	bind(tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	//float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, black);
	//tex->setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
}

void setWrap(gl::TextureRef tex, GLenum wrap) {
	// I think the border color is transparent black by default. It doesn't hurt that it is transparent.
	bind(tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
}

Array2D<vec3> merge(vector<Array2D<float> > channels) {
	Array2D<float>& r = channels[0];
	Array2D<float>& g = channels[1];
	Array2D<float>& b = channels[2];
	Array2D<vec3> result(r.w, r.h);
	forxy(result) {
		result(p) = vec3(r(p), g(p), b(p));
	}
	return result;
}

Array2D<float> div(Array2D<vec2> a) {
	return ::map(a, [&](ivec2 p) -> float {
		auto dGx_dx = (a.wr(p.x + 1, p.y).x - a.wr(p.x - 1, p.y).x) / 2.0f;
		auto dGy_dy = (a.wr(p.x, p.y + 1).y - a.wr(p.x, p.y - 1).y) / 2.0f;
		return dGx_dx + dGy_dy;
	});
}

Array2D<float> to01(Array2D<float> a) {
	auto minn = *std::min_element(a.begin(), a.end());
	auto maxx = *std::max_element(a.begin(), a.end());
	auto b = a.clone();
	forxy(b) {
		b(p) -= minn;
		b(p) /= (maxx - minn);
	}
	return b;
}
Array2D<vec3> to01(Array2D<vec3> a, float min, float max) {
	auto b = a.clone();
	forxy(b) {
		b(p) -= vec3(min);
		b(p) /= vec3(max - min);
	}
	return b;
}

TextureCache gTexCache;
gl::TextureRef maketex(int w, int h, GLint ifmt, TextureCache * texCache) {
	if(texCache == nullptr)
		texCache = &gTexCache; // tmp
	if (texCache != nullptr) {
		TextureCacheKey key;
		key.ifmt = ifmt;
		key.size = ivec2(w, h);
		return texCache->get(key);
	}
	/*else {
		gl::Texture::Format fmt;
		fmt.setInternalFormat(ifmt);
		return gl::Texture::create(w, h, fmt);
	}*/
}

void checkGLError(string place)
{
	GLenum errCode;
	if ((errCode = glGetError()) != GL_NO_ERROR)
	{
		qDebug() << "GL error 0x" << hex << errCode << dec << " at " << place;
	}
	else {
		qDebug() << "NO error at " << place;
	}
}

float sq(float f) {
	return f * f;
}

vector<float> getGaussianKernel(int ksize, float sigma) {
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

float sigmaFromKsize(float ksize) {
	float sigma = 0.3*((ksize-1)*0.5 - 1) + 0.8;
	return sigma;
}

float ksizeFromSigma(float sigma) {
	// ceil just to be sure
	int ksize = ceil(((sigma - 0.8) / 0.3 + 1) / 0.5 + 1);
	if(ksize % 2 == 0)
		ksize++;
	return ksize;
}

Array2D<vec2> gradientForward(Array2D<float> a) {
	return ::map(a, [&](ivec2 p) -> vec2 {
		return vec2(
			(a.wr(p.x + 1, p.y) - a.wr(p.x, p.y)) / 1.0f,
			(a.wr(p.x, p.y + 1) - a.wr(p.x, p.y)) / 1.0f
		);
	});
}

Array2D<float> divBackward(Array2D<vec2> a) {
	return ::map(a, [&](ivec2 p) -> float {
		auto dGx_dx = (a.wr(p.x, p.y).x - a.wr(p.x - 1, p.y).x);
		auto dGy_dy = (a.wr(p.x, p.y).y - a.wr(p.x, p.y - 1).y);
		return dGx_dx + dGy_dy;
	});
}

void disableGLReadClamp() {
	glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
}

void enableDenormalFlushToZero() {
	_controlfp(_DN_FLUSH, _MCW_DN);
}

void drawAsLuminance(gl::TextureRef const& in, const Rectf &dstRect) {
	shade2(in,
		"_out.rgb = vec3(fetch1());"
		, ShadeOpts()
		.enableResult(false)
		.dstPos(dstRect.getUpperLeft())
		.dstRectSize(dstRect.getSize())
		.fetchUpsideDown(true)
	);
}

float nan_to_num_(float f) {
	if (isnan_(f)) {
		return 0.0f;
	}
	return f;
}

unsigned int ilog2(unsigned int val) {
	unsigned int ret = -1;
	while (val != 0) {
		val >>= 1;
		ret++;
	}
	return ret;
}

vec2 compdiv(vec2 const & v1, vec2 const & v2) {
	float a = v1.x, b = v1.y;
	float c = v2.x, d = v2.y;
	float cd = sq(c) + sq(d);
	return vec2(
		(a*c + b * d) / cd,
		(b*c - a * d) / cd);
}

void draw(gl::TextureRef const& tex, ci::Rectf const& bounds) {
	throw "this code hasn't been updated";
	/*gl::TextureRef tex2 = tex;
	if (tex->getInternalFormat() == GL_R16F) {
		tex2 = redToLuminance(tex);
	}
	tex2->setTopDown(true);
	gl::draw(tex2, bounds);*/
}

void drawBetter(gl::TextureRef &texture, const Area &srcArea, const Rectf &dstRect, gl::GlslProgRef glslArg)
{
	texture->setTopDown(true);
	auto ctx = gl::context();

	Rectf texRect = texture->getAreaTexCoords(srcArea);

	gl::ScopedVao vaoScp(ctx->getDrawTextureVao());
	//ScopedBuffer vboScp(ctx->getDrawTextureVbo());
	glBindTexture(texture->getTarget(), texture->getId());

	gl::GlslProgRef glsl;
	if (glslArg != nullptr) {
		glsl = glslArg;
	}
	else {
		glsl = gl::getStockShader(gl::ShaderDef().color().texture(texture));
	}
	gl::ScopedGlslProg glslScp(glsl);
	glsl->uniform("uTex0", 0);

	ctx->setDefaultShaderVars();
	ctx->drawArrays(GL_TRIANGLE_STRIP, 0, 4);

	texture->setTopDown(false);
}

void drawBetter(gl::TextureRef &texture, const Rectf &dstRect, gl::GlslProgRef glslArg)
{
	drawBetter(texture, texture->getBounds(), dstRect, glslArg);
}

inline void denormal_check::begin_frame() {
	num = 0;
}

inline void denormal_check::check(float f) {
	if (f != 0 && fabsf(f) < numeric_limits<float>::min()) {
		// it's denormalized
		num++;
	}
}

inline void denormal_check::end_frame() {
	qDebug() << "denormals detected: " << num;
}

template<> Array2D<bytevec3> dl<bytevec3>(gl::TextureRef tex) {
	return gettexdata<bytevec3>(tex, GL_RGB, GL_UNSIGNED_BYTE);
}

template<> Array2D<float> dl<float>(gl::TextureRef tex) {
	return gettexdata<float>(tex, GL_RED, GL_FLOAT);
}

template<> Array2D<vec2> dl<vec2>(gl::TextureRef tex) {
	return gettexdata<vec2>(tex, GL_RG, GL_FLOAT);
}

template<> Array2D<vec3> dl<vec3>(gl::TextureRef tex) {
	return gettexdata<vec3>(tex, GL_RGB, GL_FLOAT);
}

template<> Array2D<vec4> dl<vec4>(gl::TextureRef tex) {
	return gettexdata<vec4>(tex, GL_RGBA, GL_FLOAT);
}
