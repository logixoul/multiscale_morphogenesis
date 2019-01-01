#include "precompiled.h"
//#include "ciextra.h"
#include "util.h"
//#include "shade.h"
#include "stuff.h"
#include "gpgpu.h"
#include "cfg1.h"
#include "sw.h"

#include "stefanfw.h"

typedef WrapModes::GetWrapped WrapMode;

// baseline 7fps
// now 9fps

int count1, count2; // todo: rm this

//int wsx=800, wsy=800.0*(800.0/1280.0);
int wsx=1280, wsy=720;
int scale=2;
int sx=wsx/::scale;
int sy=wsy/::scale;
Array2D<float> img(sx,sy);
bool pause2=false;
std::map<int, gl::TextureRef> texs;
auto imgadd_accum = Array2D<float>(sx,sy);

struct SApp : App {
	void setup()
	{
		createConsole();
		reset();
		enableDenormalFlushToZero();
		setWindowSize(wsx, wsy);

		createConsole();
		disableGLReadClamp();
		stefanfw::eventHandler.subscribeToEvents(*this);
	}
	void update()
	{
		stefanfw::beginFrame();
		stefanUpdate();
		stefanDraw();
		stefanfw::endFrame();
	}
	void keyDown(KeyEvent e)
	{
		if(keys['p'] || keys['2'])
		{
			pause2 = !pause2;
		}
		if(keys['r'])
		{
			reset();
		}
	}
	void reset() {
		forxy(img) {
			img(p)=ci::randFloat();
		}
		forxy(imgadd_accum) {
			imgadd_accum(p)=0.0f;
		}
	}
	
	typedef Array2D<float> Img;
	static Img update_1_scale(Img aImg)
	{
		auto img = aImg.clone();
		auto abc=cfg1::getOpt("abc", 1.0f,
			[&]() { return true; },
			[&]() { return niceExpRangeX(mouseX, .05f, 1000.0f); });

		auto tex = gtex(img);
		gl::TextureRef gradientsTex;
		sw::timeit("calc velocities [get_gradients]", [&]() {
			gradientsTex = get_gradients_tex(tex);
		});
		sw::timeit("calc velocities [the rest]", [&]() {
			globaldict["abc"] = abc;
			cout << "!!!!!!!!!!!!!! abc=" <<abc << endl;
			tex = shade2(tex, gradientsTex,
				"vec2 grad = fetch2(tex2);"
				"vec2 dir = perpLeft(safeNormalized(grad));"
				""
				"float val = fetch1();"
				"float valLeft = fetch1(tex, tc + tsize * dir);"
				"float valRight = fetch1(tex, tc - tsize * dir);"
				"float add = (val - (valLeft + valRight) * .5f);"
				//"add = max(add, 0);"
				"_out.r = val + add * abc;"
				//"_out.r = fetch1();"
				, ShadeOpts(),
				"vec2 perpLeft(vec2 v) {"
				"	return vec2(-v.y, v.x);"
				"}"
			);
			CHECK_GL_ERROR();
			img = gettexdata<float>(tex, GL_RED, GL_FLOAT);
		});
		
		img=::to01(img);
		sw::timeit("blur", [&]() {
			auto imgb=gaussianBlur(img, 3);
			//img=imgb;
			forxy(img) {
				img(p) = lerp(img(p), imgb(p), .8f);
			}
		});
		sw::timeit("restore avg", [&]() {
			float sum = std::accumulate(img.begin(), img.end(), 0.0f);
			float avg = sum / (float)img.area;
			forxy(img)
			{
				img(p) += .5f - avg;
			}
		});
		sw::timeit("threshold", [&]() {
			forxy(img) {
				auto& c=img(p);
				c = 3.0f*c*c-2.0f*c*c*c;
			}
		});
		//forxy(img) img(p)=max(0.0f, min(1.0f, img(p)));
		//mm(img, "img");
		return img;
	}
	Img multiscaleApply(Img src, function<Img(Img)> func) {
		int size = min(src.w, src.h);
		auto state = src.clone();
		vector<Img> scales;
		auto filter=ci::FilterGaussian();
		while(size > 1)
		{
			scales.push_back(state);
			state = ::resize(state, state.Size() / 2, filter);
			size /= 2;
		}
		vector<Img> origScales=scales;
		for(auto& s: origScales) s = s.clone();
		int lastLevel = 0;
		for(int i = scales.size() - 1; i >= lastLevel; i--) {
			//texs[i] = gtex(scales[i]);
			auto& thisScale = scales[i];
			auto& thisOrigScale = origScales[i];
			auto transformed = func(thisScale);
			auto diff = empty_like(transformed);
			sw::timeit("::map", [&]() {
#pragma omp parallel for
				for(int j = 0; j < diff.area; j++) {
					diff(j) = transformed(j) - thisOrigScale(j);
				}
			});
			float w = 1.0f-pow(i/float(scales.size()-1), 10.0f);
			w = max(0.0f, min(1.0f, w));
			sw::timeit("2 loops", [&]() {
				forxy(diff) {
					diff(p) *= w;
				}
			});
			if(i == lastLevel)
			{
				sw::timeit("::map", [&]() {
#pragma omp parallel for
					for (int j = 0; j < transformed.area; j++) {
						scales[lastLevel](j) = thisOrigScale(j) + diff(j);//.clone();
					}
				});
				break;
			}
			auto& nextScaleUp = scales[i-1];
			//texs[i] = gtex(::resize(transformed, nextScaleUp.Size(), filter));
			auto upscaledDiff = ::resize(diff, nextScaleUp.Size(), filter);
			sw::timeit("2 loops", [&]() {
				forxy(nextScaleUp) {
					nextScaleUp(p) += upscaledDiff(p);
				}
			});
		}
		return scales[lastLevel];
	}

	void stefanUpdate() {
		glDisable(GL_BLEND);
		//return;
		if(pause2) {
			return;
		}
		img = multiscaleApply(img, update_1_scale);
		//img = update_1_scale(img);
	}
	void stefanDraw()
	{
		gl::clear(Color(0, 0, 0));
		glDisable(GL_BLEND);
		cout <<"frame# "<<getElapsedFrames()<<endl;
		sw::timeit("draw", [&]() {
			if(1) {
				auto tex = gtex(img);
				drawAsLuminance(tex, getWindowBounds());
				//gl::draw(redToLuminance(tex), getWindowBounds());
			} else {
				vector<gl::TextureRef> ordered;
				do {
					for(auto& pair: texs) {
						ordered.push_back(pair.second);
					}
				}while(0);
		
				float my=max(0.0f,min(1.0f,mouseY));
				int i=(texs.size()-1)*my;
				auto tex=ordered[i];
				tex->bind();
				//tex.setMagFilter(GL_NEAREST);
				drawAsLuminance(tex, getWindowBounds());
				//gl::draw(redToLuminance(tex), getWindowBounds());
			}
		});
	}
};

CINDER_APP(SApp, RendererGl)