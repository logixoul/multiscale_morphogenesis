#include <cmath>
#include <iostream>
#include <string>
#include <cinder/app/AppBasic.h>
#include <cinder/gl/gl.h>
#include <cinder/Vector.h>
#include <numeric>
#include <boost/foreach.hpp>
#include <fftw3.h>
#include <float.h>
#include <cinder/ip/Resize.h>
#include "util.h"
#include "shade.h"
#include <cinder/gl/Texture.h>
//#include <cinder/gl/Fbo.h>
#include <cinder/ImageIo.h>
#include <cinder/gl/GlslProg.h>
#include <cinder/Rand.h>
#define foreach BOOST_FOREACH
using namespace ci;
using namespace std;
using namespace ci::app;
//using namespace boost::assign;
//using boost::irange;