#include "precompiled.h"
#include "stuff.h"

int denormal_check::num;

std::map<string,string> FileCache::db;

string FileCache::get(string filename) {
	if(db.find(filename)==db.end()) {
		std::vector<unsigned char> buffer;
		loadFile(buffer,filename);
		string bufferStr(&buffer[0],&buffer[buffer.size()]);
		db[filename]=bufferStr;
	}
	return db[filename];
}

Array2D<Vec3f> resize(Array2D<Vec3f> src, Vec2i dstSize, const ci::FilterBase &filter)
{
	ci::SurfaceT<float> tmpSurface(
		(float*)src.data, src.w, src.h, /*rowBytes*/sizeof(Vec3f) * src.w, ci::SurfaceChannelOrder::RGB);
	auto resizedSurface = ci::ip::resizeCopy(tmpSurface, tmpSurface.getBounds(), dstSize, filter);
	Array2D<Vec3f> resultArray = resizedSurface;
	return resultArray;
}

Array2D<float> resize(Array2D<float> src, Vec2i dstSize, const ci::FilterBase &filter)
{
#if 0
	ci::ChannelT<float> tmpSurface(
		(float*)src.data, src.w, src.h, /*rowBytes*/sizeof(float) * src.w, ci::SurfaceChannelOrder::ABGR);
	auto resizedSurface = ci::ip::resizeCopy(tmpSurface, tmpSurface.getBounds(), dstSize, filter);
	Array2D<float> resultArray = resizedSurface;
	return resultArray;
#endif
	auto srcRgb = ::merge(list_of(src)(src)(src));
	auto resized = resize(srcRgb, dstSize, filter);
	return ::split(resized)[0];
}
