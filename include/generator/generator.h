//
// Created by zwx on 23-8-30.
//
//
// Created by peng on 10/17/22.
//

#pragma once

#include "common/thread_local_store.h"
#include <random>

namespace utils {
    /**
     * An expression that generates a sequence of values, following some distribution (Uniform, Zipfian, Sequential, etc.).
     */
    template<typename V>
    class Generator {
    public:
        virtual ~Generator() = default;
        /**
         * Generate the next value in the distribution.
         */
        virtual V nextValue() = 0;
        /**
         * Return the previous value generated by the distribution; e.g., returned from the last {@link Generator#nextValue()}
         *  call.
         * Calling {@link #lastValue()} should not advance the distribution or have any side effects. If {@link #nextValue()}
         * has not yet been called, {@link #lastValue()} should return something reasonable.
         */
        virtual V lastValue() {
            return lastVal;
        };
        /**
         * Return the expected value (mean) of the values this generator will return.
         */
        virtual double mean() = 0;

    protected:
        /**
         * Set the last value generated. NumberGenerator subclasses must use this call
         * to properly set the last value, or the {@link #lastValue()} calls won't work.
         */
        void setLastValue(V last) {
            lastVal = last;
        }
    private:
        V lastVal;
    };

    static inline auto GetThreadLocalRandomGenerator() {
        return util::ThreadLocalStore<std::default_random_engine>::Get();
    }

    using NumberGenerator = Generator<uint64_t>;
    using DoubleGenerator = Generator<double>;
}

