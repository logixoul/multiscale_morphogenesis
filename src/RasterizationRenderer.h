#pragma once
#include "util.h"

class RasterizationRenderer
{
public:
	RasterizationRenderer(ivec2 ws, ivec2 s);
	void render(Array2D<float> img);

private:
	void updateMesh(Array2D<float> img);
	gl::VboMeshRef	vboMesh;
};

