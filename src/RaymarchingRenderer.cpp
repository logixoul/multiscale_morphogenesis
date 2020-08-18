#include "precompiled.h"
#include "RaymarchingRenderer.h"
#include "stuff.h"
#include "gpgpu.h"

RaymarchingRenderer::RaymarchingRenderer(ivec2 ws, ivec2 s)
{
	this->ws = ws;
	this->s = s;
}

void RaymarchingRenderer::render(Array2D<float> img)
{
	auto tex = gtex(img);
	auto raymarched = shade2(tex,
		""
	);
}
