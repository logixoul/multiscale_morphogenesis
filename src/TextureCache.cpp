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
#include "TextureCache.h"
#include "stuff.h"

extern int count1;
extern int count2;

std::vector<TextureCache*> TextureCache::instances;

std::map<int, string> fmtMap = {
	{ GL_R16F, "GL_R16F" },
	{ GL_RGB16F, "GL_RGB16F" },
	{ GL_RG32F, "GL_RG32F" },
	{ GL_RGBA16F, "GL_RGBA16F" },
	{ GL_RGB8, "GL_RGB8" },
};

std::map<int, int> fmtMapBpp = {
	{ GL_R16F, 2 },
	{ GL_RGB16F, 6 },
	{ GL_RG32F, 8 },
	{ GL_RGBA16F, 8 },
	{ GL_RGB8, 3 },
};

TextureCache::TextureCache() {
	instances.push_back(this);
}

static gl::TextureRef allocTex(TextureCacheKey const& key) {
	if (key.ifmt == GL_RGB8 && key.size.x == 4096) {
		cout << "hey" << endl;
	}
	cout << "allocTex\t" << key.size << "\t" << fmtMap[key.ifmt] << endl;
	gl::Texture::Format fmt;
	fmt.setInternalFormat(key.ifmt);
	GLuint texId;
	glGenTextures(1, &texId);
	auto tex = gl::Texture::create(GL_TEXTURE_2D, texId, key.size.x, key.size.y, false);
	bindTexture(tex);
	glTexStorage2D(GL_TEXTURE_2D, 1, key.ifmt, key.size.x, key.size.y);
	//gl::Texture::create(
	return tex;
}

gl::TextureRef TextureCache::get(TextureCacheKey const & key)
{
	auto it = cache.find(key);
	if (it == cache.end()) {
		//gl::TextureRef tex = maketex(key.size.x, key.size.y, key.ifmt);
		auto tex = allocTex(key);
		vector<gl::TextureRef> vec{ tex };
		cache.insert(std::make_pair(key, vec));
		return tex;
	}
	else {
		auto& vec = it->second;
		for (auto& texRef : vec) {
			if (texRef.use_count() == 1) {
				count2++;
				//cout << "returning existing texture" << endl;
				return texRef;
			}
			else if (key.ifmt == GL_RGB8) {
				cout << "hey" << endl;
			}
		}
		//cout << "requesting utilized texture; returning new texture." << endl;
		count1++;
		auto tex = allocTex(key);
		vec.push_back(tex);
		return tex;
	}
}

// todo: rename this func
void TextureCache::clearCaches()
{
	for (auto& instance : instances) {
		auto& cache = instance->cache;
		for (auto& pair : cache) {
			vector<gl::TextureRef> remaining;
			for (auto& tex : pair.second) {
				if (tex.use_count() > 1) {
					remaining.push_back(tex);
				}
			}
			//pair.second = remaining;
			cache[pair.first] = remaining;
		}
	}
}

void TextureCache::printTextures()
{
	return;
	cout << "==============" << endl;
	int bytes = 0;
	for (auto& instance : instances) {
		cout << "====== instance of TextureCache" << endl;
		for (auto& pair : instance->cache) {
			for (auto& tex : pair.second) {
				auto ifmt = tex->getInternalFormat();
				cout << tex->getSize() << " " << fmtMap[ifmt] << endl;
				bytes += tex->getBounds().calcArea() * fmtMapBpp[ifmt];
			}
		}
	}
	cout << "bytes = " << bytes << endl;
}