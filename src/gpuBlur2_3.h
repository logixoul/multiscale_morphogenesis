#pragma once
#include "precompiled.h"
#include "cfg1.h"
#include "stuff.h"

extern float mouseX, mouseY;
extern bool keys[];

namespace gpuBlur2_3 {

	inline gl::Texture singleblur(gl::Texture src, float hscale, float vscale);
	inline gl::Texture run(gl::Texture src, int lvls, int postBlurs) {
		CHECK_GL_ERROR();
		auto state = Shade().tex(src).expr("fetch3()").run();
		CHECK_GL_ERROR();
		for(int i = 0; i < lvls; i++) {
			state = singleblur(state, .5, .5);
		}
		CHECK_GL_ERROR();
		for(int i = 0; i < postBlurs; i++)
		{
			state = singleblur(state, 1.0f, 1.0f);
		}
		CHECK_GL_ERROR();
		state = singleblur(state, src.getWidth() / (float)state.getWidth(), src.getHeight() / (float)state.getHeight());
		CHECK_GL_ERROR();
		return state;
	}
	inline gl::Texture singleblur(gl::Texture src, float hscale, float vscale) {
		// gauss(0.0) = 1.0
		// gauss(2.0) = 0.1
		//globaldict["gaussW0"]
		/*int ksizeR = 3;
		float gaussThres=.1f; // weight at last sample
		float e = exp(1.0f);
		float gaussW = 1.0f / (gaussThres * sqrt(twoPi * e));*/
		float gaussW=cfg1::getOpt("gaussW",2.0f, [&]() { return keys['/']; },
			[&]() { return exp(mouseY*10.0f); });
		auto gauss = [&](float f, float width) -> float {
			float nfactor = 1.0 / (width * sqrt(twoPi));
			return nfactor * exp(-f*f/(width*width));
		};
		float w0=gauss(0.0, gaussW);
		float w1=gauss(1.0, gaussW);
		float w2=gauss(2.0, gaussW);
		float w3=gauss(3.0, gaussW);
		float sum=2.0f*w3+2.0f*w2+2.0f*w1+w0;
		w3/=sum;
		w2/=sum;
		w1/=sum;
		w0/=sum;
		stringstream weights;
		weights << fixed << "float w0="<<w0 << ", w1=" << w1 << ", w2=" << w2 << ",w3=" << w3 << ";"<<endl;
		//system("pause");

		string shader =
			"void shade() {"
			"	vec2 offset = vec2(GB2_offsetX, GB2_offsetY);"
			"	vec3 aM3 = fetch3(tex, tc + (-3.0) * offset * tsize);"
			"	vec3 aM2 = fetch3(tex, tc + (-2.0) * offset * tsize);"
			"	vec3 aM1 = fetch3(tex, tc + (-1.0) * offset * tsize);"
			"	vec3 a0 = fetch3(tex, tc + (0.0) * offset * tsize);"
			"	vec3 aP1 = fetch3(tex, tc + (+1.0) * offset * tsize);"
			"	vec3 aP2 = fetch3(tex, tc + (+2.0) * offset * tsize);"
			"	vec3 aP3 = fetch3(tex, tc + (+3.0) * offset * tsize);"
			""
			//"	float w2=0.0294118, w1=0.235294, w0=0.470588;"
			"float w0=0.164872, w1=0.151377, w2=0.117165,w3=0.0764467;"
			//+ weights.str() +
			//"	_out = .05 * (a0 + a4) + .2 * (a1 + a3) + .5 * a2;"
			"	_out = w2 * (aM2 + aP2) + w1 * (aM1 + aP1) + w0 * a0;"
			//"	_out += w3 * (aM3 + aP3);"
			"}";

		globaldict["GB2_offsetX"] = 1.0;
		globaldict["GB2_offsetY"] = 0.0;
		auto hscaled = ::Shade().tex(src).src(shader).scale(hscale, 1.0f).run();
		globaldict["GB2_offsetX"] = 0.0;
		globaldict["GB2_offsetY"] = 1.0;
		auto vscaled = ::Shade().tex(hscaled).src(shader).scale(1.0f, vscale).run();
		return vscaled;
	}
}