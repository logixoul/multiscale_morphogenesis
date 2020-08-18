#pragma once
#include "util.h"

class RaymarchingRenderer
{
public:
	RaymarchingRenderer(ivec2 ws, ivec2 s);
	void render(Array2D<float> img);

private:
	ivec2 ws, s;
};
