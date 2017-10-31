#include "precompiled.h"
//#include "ciextra.h"
#include "util.h"
//#include "shade.h"
#include "stuff.h"
#include "gpgpu.h"
#include "gpuBlur2_3.h"
#include "cfg1.h"
#include "sw.h"
#include "mainfunc_impl.h"
#include "stefanfw.h"

typedef WrapModes::GetWrapped WrapMode;

// before openMP: 18fps

//int wsx=800, wsy=800.0*(800.0/1280.0);
int wsx=512, wsy=512;
int scale=2;
int sx=wsx/scale;
int sy=wsy/scale;
Array2D<float> img(sx,sy);
bool pause2=false;
std::map<int, gl::Texture> texs;
auto imgadd_accum = Array2D<float>(sx,sy);

template<class T>
static Array2D<T> gauss3wr(Array2D<T> src) {
	T zero=::zero<T>();
	Array2D<T> dst1(src.w, src.h);
	Array2D<T> dst2(src.w, src.h);
	forxy(dst1)
		dst1(p) = .25f * (2 * getWrapped(src, p.x, p.y) + getWrapped(src, p.x-1, p.y) + getWrapped(src, p.x+1, p.y));
	forxy(dst2)
		dst2(p) = .25f * (2 * getWrapped(dst1, p.x, p.y) + getWrapped(dst1, p.x, p.y-1) + getWrapped(dst1, p.x, p.y+1));
	return dst2;
}

struct SApp : AppBasic {
	void setup()
	{
		createConsole();
		reset();
		_controlfp(_DN_FLUSH, _MCW_DN);
		setWindowSize(wsx, wsy);

		glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
		glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
		glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);
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

		Array2D<Vec2f> velocity(img.w, img.h, Vec2f::zero());
		//float abc = niceExpRangeX(mouseX, .001f, 40000.0f);
		auto perpLeft = [&](Vec2f v) { return Vec2f(-v.y, v.x); }; //correct
		auto perpRight = [&](Vec2f v) { return -perpLeft(v); }; //correct
		auto guidance = Array2D<float>(img.w, img.h);
		guidance = img;
		sw::timeit("guidance blurs", [&]() {
			/*float sumw=0.0f;
			for(int r = 0; r < 30; r+=9) {
				auto blurred = gaussianBlur(img, r * 2 + 1);
				//float w = r * r;
				float w = pow(1.0f / 4.0f, float(r));
				sumw+=w;
				forxy(guidance)
					guidance(p) += w * blurred(p);
			}
			forxy(guidance)
				guidance(p) /= sumw;*/
			//guidance = gaussianBlur(img, 6);
		});
		auto imgadd=Array2D<float>(img.w,img.h);
		Array2D<Vec2f> gradients;
		sw::timeit("calc velocities [get_gradients]", [&]() {
			gradients = get_gradients(guidance);
		});
		sw::timeit("calc velocities [the rest]", [&]() {
			for(int x = 0; x < img.w; x++)
			{
				for(int y = 0; y < img.h; y++)
				{
					Vec2f p = Vec2f(x,y);
					Vec2f grad = gradients(x, y).safeNormalized();

					Vec2f gradP = perpLeft(grad);
					
					float val = guidance(x, y);
					float valLeft = getBilinear<float, WrapModes::GetWrapped>(guidance, p+gradP);
					float valRight = getBilinear<float, WrapModes::GetWrapped>(guidance, p-gradP);
					float add = (val - (valLeft + valRight) * .5f);
					imgadd(x, y) = add * abc;
				}
			}
		});
		
		sw::timeit("imgadd calculations", [&]() {
			/*forxy(img) {
				imgadd_accum(p)+=imgadd(p);
				imgadd_accum(p)*=.9f;
			}
			forxy(img) {
				img(p)+=imgadd_accum(p);
			}*/
			forxy(img) {
				img(p)+=imgadd(p);
			}
		});
		img=::to01(img);
		sw::timeit("blur", [&]() {
			//auto imgb=gaussianBlur(img, 3);
			auto imgb = gauss3wr(img);
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
	Img pyrDown(Img src) {
		auto srcB = gaussianBlur(src, 3);
		auto halfSized = Array2D<float>(src.w/2, src.h/2);
		forxy(halfSized) {
			halfSized(p) = srcB(Vec2f(p) * 2 + Vec2f::one()*.5f);
		}
		return halfSized;
	}
	//Img resize
	Img multiscaleApply(Img src, function<Img(Img)> func) {
		int size = min(src.w, src.h);
		auto state = src.clone();
		vector<Img> scales;
		auto filter=ci::FilterGaussian();
		while(size > 1)
		{
			scales.push_back(state);
			state = ::resize(state, state.Size() / 2, filter);
			//state = pyrDown(state);
			size /= 2;
		}
		vector<Img> origScales=scales;
		foreach(auto& s, origScales) s = s.clone();
		int lastLevel = 0;
		for(int i = scales.size() - 1; i >= lastLevel; i--) {
			texs[i] = gtex(scales[i]);
			auto& thisScale = scales[i];
			auto& thisOrigScale = origScales[i];
			auto transformed = func(thisScale);
			auto diff = ::map(transformed, [&](Vec2i p) { return transformed(p) - thisOrigScale(p); });
			float w = 1.0f-pow(i/float(scales.size()-1), 10.0f);
			w = max(0.0f, min(1.0f, w));
			forxy(diff) {
				diff(p) *= w;
			}
			if(i == lastLevel)
			{
				scales[lastLevel] = ::map(transformed, [&](Vec2i p) { return thisOrigScale(p) + diff(p); });//.clone();
				break;
			}
			auto& nextScaleUp = scales[i-1];
			texs[i] = gtex(::resize(transformed, nextScaleUp.Size(), filter));
			auto upscaledDiff = ::resize(diff, nextScaleUp.Size(), filter);
			forxy(nextScaleUp) {
				nextScaleUp(p) += upscaledDiff(p);
			}
		}
		return scales[lastLevel];
	}

	void stefanUpdate() {
		if(pause2) {
			return;
		}
		img = multiscaleApply(img, update_1_scale);
		//img = update_1_scale(img);
	}
	void stefanDraw()
	{
		gl::clear(Color(0, 0, 0));
		cout <<"frame# "<<getElapsedFrames()<<endl;
		sw::timeit("draw", [&]() {
			if(1) {
				auto tex = gtex(img);
				gl::draw(tex, getWindowBounds());
			} else {
				vector<gl::Texture> ordered;
				do {
					foreach(auto& pair, texs) {
						ordered.push_back(pair.second);
					}
				}while(0);
		
				float my=max(0.0f,min(1.0f,mouseY));
				int i=(texs.size()-1)*my;
				auto tex=ordered[i];
				tex.bind();
				//tex.setMagFilter(GL_NEAREST);
				gl::draw(tex, getWindowBounds());
			}
		});
	}
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	return mainFuncImpl(new SApp());
}