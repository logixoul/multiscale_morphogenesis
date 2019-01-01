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
#include "sw.h"

static std::map<string, sw::Entry> times;

static std::chrono::high_resolution_clock::time_point startTime;

static double getElapsedMilliseconds()
{
	return (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime)).count();
}

void sw::start() { startTime = std::chrono::high_resolution_clock::now(); }

void sw::printElapsed(string desc) {
	cout << desc << " took " << getElapsedMilliseconds() << "ms" << endl;
}

void sw::timeit(string desc, std::function<void()> func) {
	start();
	func();
	if (times.find(desc) == times.end())
	{
		Entry entry;
		entry.index = times.size();
		entry.desc = desc;
		entry.time = 0.0f;
		times[desc] = entry;
	}
	times[desc].time += getElapsedMilliseconds();
}

void sw::beginFrame() {
	times.clear();
}

void sw::endFrame() {
	//qDebug() << "=== TIMINGS ===";
	vector<Entry> ordered(times.size());
	for (auto& pair : times) {
		//cout << pair.first << " took " << pair.second.time << "ms" << endl;
		Entry entry = pair.second;
		ordered[entry.index] = entry;
	}
	for (auto& entry : ordered) {
		cout << entry.desc << " took " << entry.time << "ms" << endl;
	}
}
