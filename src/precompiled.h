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

#define BOOST_RESULT_OF_USE_DECLTYPE 
#include <cmath>
#include <iostream>
#include <string>
#include <sstream>
#include <map>
//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h> // tmp for AllocConsole
#include <cinder/ip/Resize.h>
#include <cinder/app/App.h>
#include <cinder/app/RendererGl.h>
#include <cinder/gl/GlslProg.h>
#include <cinder/gl/Texture.h>
#include <cinder/gl/gl.h>
#include <cinder/ImageIo.h>
#include <cinder/Vector.h>
#include <cinder/Rand.h>
//#include <boost/signals2.hpp>
#include <cfloat>
#include <fftw3.h>
#include <numeric>
#include <libraw/libraw.h>
#include <thread>
#include <mutex>
#include <tuple> // for std::tie
#include <chrono>
#include <queue>
#include <experimental/filesystem>
#include <opencv2/core.hpp>
#include <opencv2/photo.hpp>
#define __CL_ENABLE_EXCEPTIONS
//#include <CL/cl.hpp>
//#include <clFFT.h>

using namespace ci;
using namespace std;
using namespace ci::app;
using namespace std::experimental;
//using namespace boost::signals2;
using path = filesystem::path;