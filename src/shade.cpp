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
#include "shade.h"
#include "util.h"
#include "qdebug.h"
#include "stuff.h"
#include "stefanfw.h"

bool fboBound = false;

void beginRTT(gl::TextureRef fbotex)
{
	static unsigned int fboid = 0;
	if(fboid == 0)
	{
		glGenFramebuffersEXT(1, &fboid);
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboid);
	if (fbotex != nullptr)
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fbotex->getId(), 0);
	fboBound = true;
}
void endRTT()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	fboBound = false;
}

void drawRect() {
	auto ctx = gl::context();

	ctx->getDrawTextureVao()->bind();
	//ctx->getDrawTextureVbo()->bind(); // this seems to be unnecessary

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

std::map<string, float> globaldict;
std::map<string, int> globaldictI;
void globaldict_default(string s, float f) { // todo: rm this func
	if(globaldict.find(s) == globaldict.end())
	{
		globaldict[s] = f;
	}
}

auto samplerSuffix = [&](int i) -> string {
	return (i == 0) ? "" : std::to_string(1 + i);
};

auto samplerName = [&](int i) -> string {
	return "tex" + samplerSuffix(i);
};

bool dbgDisableSamplers = false;

std::string getCompleteFshader(vector<gl::TextureRef> const& texv, std::string const& fshader, string* uniformDeclarationsRet) {
	auto texIndex = [&](gl::TextureRef t) {
		return std::to_string(
			1 + (std::find(texv.begin(), texv.end(), t) - texv.begin())
			);
	};
	stringstream uniformDeclarations;
	int location = 0;
	uniformDeclarations << "layout(location=" << location++ << ") uniform vec2 mouse;\n";
	//uniformDeclarations << "layout(location=" << location++ << ") uniform vec2 resultSize;\n";
	//uniformDeclarations << "vec2 my_FragCoord;\n";
	if(!dbgDisableSamplers)
	for(int i = 0; i < texv.size(); i++)
	{
		string samplerType = "sampler2D";
		GLenum fmt, type;
		texv[i]->getInternalFormatInfo(texv[i]->getInternalFormat(), &fmt, &type);
		if (type == GL_UNSIGNED_INT) samplerType = "usampler2D";
		uniformDeclarations << "layout(location=" << location++ << ") uniform " + samplerType + " " + samplerName(i) + ";\n";
		uniformDeclarations << "layout(location=" << location++ << ") uniform vec2 " + samplerName(i) + "Size;\n";
		uniformDeclarations << "layout(location=" << location++ << ") uniform vec2 tsize" + samplerSuffix(i) + ";\n";
	}
	for(auto& p: globaldict)
	{
		uniformDeclarations << "layout(location=" << location++ << ") uniform float " + p.first + ";\n";
	}
	for (auto& p : globaldictI)
	{
		uniformDeclarations << "layout(location=" << location++ << ") uniform int " + p.first + ";\n";
	}
	uniformDeclarations << "layout(binding=0, rg32f) uniform coherent image2D image;";
	*uniformDeclarationsRet = uniformDeclarations.str();
	string intro =
		Str()
		<< "#version 150"
		<< "#extension GL_ARB_explicit_uniform_location : enable"
		<< "#extension GL_ARB_gpu_shader_fp64 : enable" // todo: is this needed?
		<< "#extension GL_ARB_shader_image_load_store : enable"
		<< "#extension GL_ARB_shading_language_420pack : enable"
		<< uniformDeclarations.str()
		<< "in vec2 tc;"
		<< "in highp vec2 relOutTc;"
		<< "out vec4 _out;"
		//<< "layout(origin_upper_left) in vec4 gl_FragCoord;"
		;
	if(!dbgDisableSamplers)
		intro += Str()
		<< "vec4 fetch4(sampler2D tex_, vec2 tc_) {"
		<< "	return texture2D(tex_, tc_).rgba;"
		<< "}"
		<< "vec3 fetch3(sampler2D tex_, vec2 tc_) {"
		<< "	return texture2D(tex_, tc_).rgb;"
		<< "}"
		<< "vec2 fetch2(sampler2D tex_, vec2 tc_) {"
		<< "	return texture2D(tex_, tc_).rg;"
		<< "}"
		<< "float fetch1(sampler2D tex_, vec2 tc_) {"
		<< "	return texture2D(tex_, tc_).r;"
		<< "}"
		<< "vec4 fetch4(sampler2D tex_) {"
		<< "	return texture2D(tex_, tc).rgba;"
		<< "}"
		<< "vec3 fetch3(sampler2D tex_) {"
		<< "	return texture2D(tex_, tc).rgb;"
		<< "}"
		<< "vec2 fetch2(sampler2D tex_) {"
		<< "	return texture2D(tex_, tc).rg;"
		<< "}"
		<< "float fetch1(sampler2D tex_) {"
		<< "	return texture2D(tex_, tc).r;"
		<< "}"
		<< "vec4 fetch4() {"
		<< "	return texture2D(tex, tc).rgba;"
		<< "}"
		<< "vec3 fetch3() {"
		<< "	return texture2D(tex, tc).rgb;"
		<< "}"
		<< "vec2 fetch2() {"
		<< "	return texture2D(tex, tc).rg;"
		<< "}"
		<< "float fetch1() {"
		<< "	return texture2D(tex, tc).r;"
		<< "}"
		<< "vec2 safeNormalized(vec2 v) { return length(v)==0.0 ? v : normalize(v); }"
		<< "vec3 safeNormalized(vec3 v) { return length(v)==0.0 ? v : normalize(v); }"
		<< "vec4 safeNormalized(vec4 v) { return length(v)==0.0 ? v : normalize(v); }";
	string outro =
		Str()
		<< "void main()"
		<< "{"
		<< "	_out = vec4(0.0f);"
		<< "	shade();"
		<< "}";
	return intro + fshader + outro;
}

void setUniform(int location, float val) {
	glUniform1f(location, val);
}

void setUniform(int location, vec2 val) {
	glUniform2f(location, val.x, val.y);
}

void setUniform(int location, int val) {
	glUniform1i(location, val);
}

gl::TextureRef shade(vector<gl::TextureRef> const& texv, std::string const& fshader, ShadeOpts const& opts)
{
	shared_ptr<GpuScope> gpuScope;
	if (opts._scopeName != "") {
		gpuScope = make_shared<GpuScope>(opts._scopeName);
	}
	//const string fshader = "void shade() { }";
	static std::map<string, gl::GlslProgRef> shaders;
	gl::GlslProgRef shader;
	if(shaders.find(fshader) == shaders.end())
	{
		string uniformDeclarations;
		std::string completeFshader = getCompleteFshader(texv, fshader, &uniformDeclarations);
		try{
			auto fmt = gl::GlslProg::Format()
				.vertex(
					Str()
					<< "#version 150"
					<< "#extension GL_ARB_explicit_uniform_location : enable"
					<< "#extension GL_ARB_gpu_shader_fp64 : enable" // todo: is this needed?
					<< "#extension GL_ARB_shader_image_load_store : enable"
					<< "#extension GL_ARB_shading_language_420pack : enable"
					<< "in vec4 ciPosition;"
					<< "in vec2 ciTexCoord0;"
					<< "out highp vec2 tc;"
					<< "out highp vec2 relOutTc;" // relative out texcoord
					<< "uniform vec2 uTexCoordOffset, uTexCoordScale;"
					<< uniformDeclarations

					<< "void main()"
					<< "{"
					<< "	gl_Position = ciPosition * 2 - 1;"
					<< "	tc = ciTexCoord0;"
					<< "	relOutTc = tc;"
					<< "	tc = uTexCoordOffset + uTexCoordScale * tc;"
					<< "}")
				.fragment(completeFshader)
				.attribLocation("ciPosition", 0)
				.attribLocation("ciTexCoord0", 1)
				.preprocess(false);
			shader = gl::GlslProg::create(fmt);
			shaders[fshader] = shader;
		} catch(gl::GlslProgCompileExc const& e) {
			cout << "gl::GlslProgCompileExc: " << e.what() << endl;
			cout << "source:" << endl;
			cout << completeFshader << endl;
			string s; cin >> s;
			throw;
		}
	} else {
		shader = shaders[fshader];
	}
	auto tex0 = texv[0];
	//auto prevGlslProg = gl::Context::getCurrent()->getGlslProg();
	//glUseProgram(shader->getHandle());
	gl::Context::getCurrent()->bindGlslProg(shader);
	ivec2 viewportSize(tex0->getWidth() * opts._scaleX, tex0->getHeight() * opts._scaleY);
	if (opts._dstRectSize != ivec2()) {
		viewportSize = opts._dstRectSize;
	}
	gl::TextureRef result;
	if (opts._enableResult) {
		if (opts._targetTex != nullptr)
		{
			result = opts._targetTex;
		}
		else {
			GLenum ifmt = opts._ifmt.exists ? opts._ifmt.val : tex0->getInternalFormat();
			result = maketex(viewportSize.x, viewportSize.y, ifmt, opts._texCache);
		}
	}
	
	int location = 0;
	::setUniform(location++, vec2(mouseX, mouseY));
	//::setUniform(location++, vec2(result->getSize()));
	if (!dbgDisableSamplers) {
		::setUniform(location++, 0); bindTexture(tex0, GL_TEXTURE0 + 0);
		::setUniform(location++, vec2(tex0->getSize()));
		::setUniform(location++, vec2(1.0) / vec2(tex0->getSize()));
	for (int i = 1; i < texv.size(); i++) {
		//shader.
		//string index = texIndex(texv[i]);
		::setUniform(location++, i); bindTexture(texv[i], GL_TEXTURE0 + i);
		::setUniform(location++, vec2(texv[i]->getSize()));
		::setUniform(location++, vec2(1)/vec2(texv[i]->getSize()));
	}
	}
	for (auto& p : globaldict)
	{
		//::setUniform(location++, p.second);
		//glUniform1f(glGetUniformLocation(p.first.c_str()), p.second);
		// todo: when I remove globaldict, this slow way won't be necessary anymore
		shader->uniform(p.first, p.second);
	}
	for (auto& p : globaldictI)
	{
		//::setUniform(location++, p.second);
		//glUniform1f(glGetUniformLocation(p.first.c_str()), p.second);
		// todo: when I remove globaldict, this slow way won't be necessary anymore
		shader->uniform(p.first, p.second);
	}
	shader->uniform("image", 0); // todo: rm this?
	auto srcArea = opts._area;
	if (srcArea == Area::zero()) {
		srcArea = tex0->getBounds();
	}
	if(!opts._fetchUpsideDown)
		tex0->setTopDown(true);
	Rectf texRect = tex0->getAreaTexCoords(srcArea);
	tex0->setTopDown(false);
	shader->uniform("uTexCoordOffset", texRect.getUpperLeft());
	shader->uniform("uTexCoordScale", texRect.getSize());
	opts._callback(shader);

	bool fastPath = fboBound;
	if (opts._enableResult) {
		if (fastPath) {
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, result->getId(), 0);
		}
		else {
			beginRTT(result);
		}
	}
	else {
		if(!opts._enableWrite)
			glColorMask(false, false, false, false);
	}
	 
	gl::pushMatrices();
	{
		gl::ScopedViewport sv(opts._dstPos, viewportSize);
		gl::setMatricesWindow(ivec2(1, 1), true);
		::drawRect();
		gl::popMatrices();
	}
	if (opts._enableResult) {
		if (!fastPath)
			endRTT();
	}
	else {
		if (!opts._enableWrite)
			glColorMask(true, true, true, true);
	}
	//glUseProgram(0); // as in gl::Context::pushGlslProg
	//gl::Context::getCurrent()->bindGlslProg(prevGlslProg);
	return result;
}

inline gl::TextureRef shade(vector<gl::TextureRef> const & texv, std::string const & fshader, float resScale)
{
	return shade(texv, fshader, ShadeOpts().scale(resScale));
}

GpuScope::GpuScope(string name) {
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name.c_str());
}

GpuScope::~GpuScope() {
	glPopDebugGroup();
}

ShadeOpts::ShadeOpts() {
	//_ifmt=GL_RGBA16F;
	_scaleX = _scaleY = 1.0f;
}
