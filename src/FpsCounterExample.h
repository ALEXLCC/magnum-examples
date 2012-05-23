#ifndef MAGNUM_EXAMPLES_FpsCounterExample_h
#define MAGNUM_EXAMPLES_FpsCounterExample_h
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

#include "Contexts/GlutContext.h"

#include <chrono>

#include "Query.h"

namespace Magnum { namespace Examples {

/** @brief Base class for examples with FPS counter */
class FpsCounterExample : public Contexts::GlutContext {
    public:
        inline FpsCounterExample(int& argc, char** argv, const std::string& name = "Magnum Example", const Math::Vector2<GLsizei>& size = Math::Vector2<GLsizei>(800, 600)): GlutContext(argc, argv, name, size), frames(0), totalFrames(0), primitives(0), totalPrimitives(0), samples(0), totalSamples(0), minimalDuration(3.5), totalDuration(0.0), fpsEnabled(false), primitiveEnabled(false), sampleEnabled(false) {}

        /**
         * @brief Minimal duration between printing FPS to console
         *
         * Default value is 3.5 seconds.
         */
        inline double minimalCounterDuration() const { return minimalDuration; }

        /**
         * @brief Set minimal duration between printing FPS to console output
         *
         * The larger duration, the more precise and steady FPS information
         * will be.
         */
        inline void setMinimalCounterDuration(double value) {
            minimalDuration = value;
        }

        /** @brief Whether FPS counter is enabled */
        inline bool fpsCounterEnabled() const { return fpsEnabled; }

        /**
         * @brief Enable or disable FPS counter
         *
         * The counter is off by default. If enabled, calls reset().
         */
        void setFpsCounterEnabled(bool enabled);

        /** @brief Whether primitive counter is enabled */
        inline bool primitiveCounterEnabled() const { return primitiveEnabled; }

        /**
         * @brief Enable or disable primitive counter
         *
         * The counter is off by default. If enabled, calls resetCounter().
         * If disabled, disables also sample counter.
         */
        void setPrimitiveCounterEnabled(bool enabled);

        /** @brief Whether sample counter is enabled */
        inline bool sampleCounterEnabled() const { return sampleEnabled; }

        /**
         * @brief Enable or disable sample counter
         *
         * Counts number of samples per primitive. The counter is off by
         * default. If enabled, calls resetCounter() and enables also
         * primitive counter.
         */
        void setSampleCounterEnabled(bool enabled);

        /**
         * @brief Reset counter
         *
         * Call after operations which would spoil FPS information, such as
         * very long frames, scene changes etc. This function is called
         * automatically in viewportEvent().
         */
        void resetCounter();

        /**
         * @brief Print FPS statistics for longer duration
         *
         * This function prints FPS statistics counted from the beginning or
         * previous time statistics() was called and then calls reset().
         */
        void printCounterStatistics();

    protected:
        /**
         * @copydoc GlutContext::viewportEvent()
         *
         * Calls reset() and saves viewport size.
         * @attention You have to call this function from your viewportEvent()
         * reimplementation!
         */
        void viewportEvent(const Math::Vector2<GLsizei>& size) {
            resetCounter();
            viewport = size;
        }

        /**
         * @copydoc GlutContext::redraw()
         *
         * Measures FPS in given duration and prints statistics to
         * console.
         */
        void redraw();

    private:
        size_t frames, totalFrames;
        GLuint primitives, totalPrimitives, samples, totalSamples;
        double minimalDuration, totalDuration;
        bool fpsEnabled, primitiveEnabled, sampleEnabled;
        Query primitiveQuery;
        SampleQuery sampleQuery;
        std::chrono::high_resolution_clock::time_point before;
        Math::Vector2<GLsizei> viewport;
};

}}

#endif
