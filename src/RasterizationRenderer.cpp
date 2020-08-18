#include "precompiled.h"
#include "RasterizationRenderer.h"
#include "stuff.h"

static const string vs = CI_GLSL(150,
	uniform mat4 ciModelViewProjection;
	uniform mat3 ciNormalMatrix;

	in vec4 ciPosition;
	in vec3 ciNormal;
	out highp vec3 Normal;
	out vec3 vertPos;
	// https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_reflection_model#OpenGL_Shading_Language_code_sample
	void main(void)
	{
		gl_Position = ciModelViewProjection * ciPosition;
		vec4 vertPos4 = gl_Position;
		vertPos = vec3(vertPos4) / vertPos4.w;
		Normal = ciNormalMatrix * ciNormal;
	}
);
static const string fs = CI_GLSL(150,
	out vec4 oColor;
	in vec3 Normal;
	in vec3 vertPos;
	void main(void)
	{
		const vec3 L = normalize(vec3(0, .2, 1));
		vec3 N = normalize(Normal);
		// diffuse
		float lambert = max(0.0, dot(N, L));

		// specular
		vec3 viewDir = normalize(-vertPos);
		vec3 halfDir = normalize(L + viewDir);
		float specAngle = max(dot(halfDir, N), 0.0);
		float specular = pow(specAngle, 40.0f) * 10.0f;
		oColor = vec4(vec3(.01, .01, .2) * (lambert + .1) + vec3(specular), 1.0);
		oColor.rgb /= oColor.rgb + 1;
		oColor.rgb = pow(oColor.rgb, vec3(1.0 / 2.2));
	}
);

void RasterizationRenderer::render(Array2D<float> img)
{
	updateMesh(img);

	gl::disableBlending();
	gl::color(Colorf(1, 1, 1));
	static auto prog = gl::GlslProg::create(::vs, ::fs);
	gl::ScopedGlslProg glslScope(prog);
	gl::draw(vboMesh);
}
RasterizationRenderer::RasterizationRenderer(ivec2 ws, ivec2 s)
{
	this->ws = ws;
	this->s = s;
	auto plane = geom::Plane().size(vec2(ws.x, ws.y)).subdivisions(ivec2(s.x, s.y))
		.axes(vec3(1, 0, 0), vec3(0, 1, 0));
	vector<gl::VboMesh::Layout> bufferLayout = {
		gl::VboMesh::Layout().usage(GL_DYNAMIC_DRAW).attrib(geom::Attrib::POSITION, 3),
		gl::VboMesh::Layout().usage(GL_DYNAMIC_DRAW).attrib(geom::Attrib::NORMAL, 3)
	};

	vboMesh = gl::VboMesh::create(plane, bufferLayout);
}
void RasterizationRenderer::updateMesh(Array2D<float> img) {
	const float h = 20.0f;
	auto mappedPosAttrib = vboMesh->mapAttrib3f(geom::Attrib::POSITION, false);
	for (int i = 0; i <= s.x; i++) {
		for (int j = 0; j <= s.y; j++) {
			mappedPosAttrib->x = i;// +sin(pos.x) * 50;
			mappedPosAttrib->y = j;// +sin(pos.y) * 50;
			mappedPosAttrib->z = img(i, j) * h;
			++mappedPosAttrib;
		}
	}
	mappedPosAttrib.unmap();

	auto mappedNormalAttrib = vboMesh->mapAttrib3f(geom::Attrib::NORMAL, false);
	for (int i = 0; i <= s.x; i++) {
		for (int j = 0; j <= s.y; j++) {
			/*
			const ivec3 off = ivec3(-1,0,1);
				float s01 = textureOffset(unit_wave, tex_coord, ivec2(-1,0)).x;
				float s21 = textureOffset(unit_wave, tex_coord, ivec2(1,0)).x;
				float s10 = textureOffset(unit_wave, tex_coord, ivec2(0,-1)).x;
				float s12 = textureOffset(unit_wave, tex_coord, ivec2(0,1)).x;
				vec3 va = normalize(vec3(size.xy,s21-s01));
				vec3 vb = normalize(vec3(size.yx,s12-s10));
				vec4 bump = vec4( cross(va,vb), s11 );
			*/
			float s01 = img.wr(i - 1, j) * h;
			float s21 = img.wr(i + 1, j) * h;
			float s10 = img.wr(i, j - 1) * h;
			float s12 = img.wr(i, j + 1) * h;
			vec3 va = normalize(vec3(vec2(2, 0), s21 - s01));
			vec3 vb = normalize(vec3(vec2(0, 2), s12 - s10));
			*mappedNormalAttrib = cross(va, vb);
			*mappedNormalAttrib = safeNormalized(*mappedNormalAttrib);
			//*mappedNormalAttrib = ci::randVec3();
			++mappedNormalAttrib;
		}
	}
	mappedNormalAttrib.unmap();
}