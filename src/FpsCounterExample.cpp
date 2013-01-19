/*
    Copyright © 2010, 2011, 2012 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include "FpsCounterExample.h"

#include <iostream>
#include <iomanip>

using namespace Magnum;

namespace Magnum { namespace Examples {

FpsCounterExample::FpsCounterExample(int& argc, char** argv, const std::string& name, const Vector2i& size): Application(argc, argv, name, size), frames(0), totalFrames(0), minimalDuration(3.5), totalDuration(0.0), fpsEnabled(false)
    #ifndef MAGNUM_TARGET_GLES
    , primitives(0), totalPrimitives(0), samples(0), totalSamples(0), primitiveEnabled(false), sampleEnabled(false)
    #endif
{}

void FpsCounterExample::redraw() {
    #ifndef MAGNUM_TARGET_GLES
    if(fpsEnabled || primitiveEnabled || sampleEnabled) {
    #else
    if(fpsEnabled) {
    #endif
        if(fpsEnabled)
            ++frames;
        #ifndef MAGNUM_TARGET_GLES
        if(primitiveEnabled) {
            primitiveQuery.end();
            primitives += primitiveQuery.result<GLuint>();
            primitiveQuery.begin(Query::Target::PrimitivesGenerated);
        }
        if(sampleEnabled) {
            sampleQuery.end();
            samples += sampleQuery.result<GLuint>();
            sampleQuery.begin(SampleQuery::Target::SamplesPassed);
        }
        #endif

        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double>(now-before).count();
        if(duration > minimalDuration) {
            std::streamsize precision = std::cout.precision();
            std::ostream::fmtflags flags = std::cout.flags();
            std::cout.precision(1);
            std::cout.setf(std::ostream::floatfield, std::ostream::fixed);
            if(fpsEnabled) {
                std::cout << std::setw(10) << frames/duration << " FPS ";
                totalFrames += frames;
            }
            #ifndef MAGNUM_TARGET_GLES
            if(primitiveEnabled) {
                std::cout << std::setw(10) << double(primitives)/frames << " tris/frame ";
                std::cout << std::setw(10) << primitives/(duration*1000) << "k tris/s ";
                totalPrimitives += primitives;
            }
            if(sampleEnabled) {
                std::cout << std::setw(10) << double(samples)/primitives << " samples/tri";
                totalSamples += samples;
            }
            #endif

            std::cout << '\r';
            std::cout.precision(precision);
            std::cout.flags(flags);
            std::cout.flush();

            frames = 0;
            #ifndef MAGNUM_TARGET_GLES
            primitives = 0;
            samples = 0;
            #endif
            before = now;
            totalDuration += duration;
        }
    }

    Application::redraw();
}

void FpsCounterExample::setFpsCounterEnabled(bool enabled) {
    if(fpsEnabled == enabled) return;

    resetCounter();
    fpsEnabled = enabled;
}

#ifndef MAGNUM_TARGET_GLES
void FpsCounterExample::setPrimitiveCounterEnabled(bool enabled) {
    if(primitiveEnabled == enabled) return;

    resetCounter();

    if(enabled)
        primitiveQuery.begin(Query::Target::PrimitivesGenerated);
    else {
        setSampleCounterEnabled(false);
        primitiveQuery.end();
    }

    primitiveEnabled = enabled;
}

void FpsCounterExample::setSampleCounterEnabled(bool enabled) {
    if(sampleEnabled == enabled) return;

    resetCounter();

    if(enabled) {
        setPrimitiveCounterEnabled(true);
        sampleQuery.begin(SampleQuery::Target::SamplesPassed);
    } else sampleQuery.end();

    sampleEnabled = enabled;
}
#endif

void FpsCounterExample::resetCounter() {
    before = std::chrono::high_resolution_clock::now();
    frames = totalFrames = 0;
    #ifndef MAGNUM_TARGET_GLES
    primitives = totalPrimitives = samples = totalSamples = 0;
    #endif
    totalDuration = 0.0;
}

void FpsCounterExample::printCounterStatistics() {
    if(totalDuration != 0) {
        std::streamsize precision = std::cout.precision();
        std::ostream::fmtflags flags = std::cout.flags();
        std::cout.precision(1);
        std::cout.setf(std::ostream::floatfield, std::ostream::fixed);
        std::cout << "Average values on " << viewport.x()
             << "x" << viewport.y() << " during " << totalDuration
             << " seconds:                                 \n";
        if(fpsEnabled)
            std::cout << std::setw(10) << totalFrames/totalDuration << " FPS ";

        #ifndef MAGNUM_TARGET_GLES
        if(primitiveEnabled) {
            std::cout << std::setw(10) << double(totalPrimitives)/totalFrames << " tris/frame ";
            std::cout << std::setw(10) << totalPrimitives/(totalDuration*1000) << "k tris/s ";
        }
        if(sampleEnabled)
            std::cout << std::setw(10) << double(totalSamples)/totalPrimitives << " samples/tri";
        #endif

        std::cout.precision(precision);
        std::cout.flags(flags);
        std::cout << std::endl;
    }

    resetCounter();
}

}}
