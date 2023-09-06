#include <benchmark/benchmark.h>
#include <fmt/core.h>
#include <math.h>

#include <algorithm> // Always comes with regret
#include <array>
#include <stdio.h>
#include <string>

using namespace std::string_literals;
//

/* ssize_t is a terrible name. */
using size_s = long long int;

// using Real = double;
using Real = float;

namespace vecop {

template <typename T>
T* Tmalloc(size_s len)
{
    return (T*)malloc(sizeof(T) * len);
}

template <typename T>
void zero(T* vec, size_s len)
{
    for (size_s i = 0; i < len; ++i) {
        vec[i] = 0;
    }
}
template <typename T>
void dump_vec(T* vec, size_s len, std::string path)
{
    FILE* f = fopen(path.c_str(), "wb");
    fwrite(vec, sizeof(T), len, f);
    fclose(f);
}
} // namespace vecop



void triangle_sin_harmonics_3_v0(Real* buf, size_s nSamples, Real sampleHz, Real noteHz, Real amplitude)
{
    Real front_coef = 8 / (M_PI * M_PI);
    Real sin_coef = M_PI * 2 * noteHz;

    Real step = 1 / sampleHz;
    for (size_s idx = 0; idx < nSamples; ++idx) {
        Real t = idx * step;

        Real val = //
            /* Real(1) * */ sin(/* Real(1) * */ sin_coef * t) //
            - Real(3) * sin(Real(1.0 / 9.0) * sin_coef * t) //
            + Real(5) * sin(Real(1.0 / 25.0) * sin_coef * t);
        buf[idx] = val * front_coef;
    }
}

void triangle_sin_harmonics_n_v1(Real* buf, size_s nSamples, Real sampleHz, Real noteHz, Real amplitude, int nHarmonics)
{

    constexpr int n_stack = 8;
    Real stack[n_stack * 2] = {};

    Real* pre_n = stack;
    if (nHarmonics > n_stack) {
        pre_n = vecop::Tmalloc<Real>(nHarmonics * 2);
    }
    for (int i = 0; i < nHarmonics; ++i) {
        Real n = 2 * i + 1;
        pre_n[i * 2] = n;
        pre_n[i * 2 + 1] = 1 / (n * n);
    }

    Real front_coef = 8 / (M_PI * M_PI);
    Real sin_coef = M_PI * 2 * noteHz;

    Real step = 1 / sampleHz;
    for (size_s idx = 0; idx < nSamples; ++idx) {
        Real t = idx * step;

        Real val = 0;
        Real sign = 1;
        for (int i = 0; i < nHarmonics; ++i) {
            Real mode_number_n = pre_n[i * 2];
            Real n_inv_sq = pre_n[i * 2 + 1];

            Real val_i = sign * n_inv_sq * sin(sin_coef * mode_number_n * t);

            val += val_i;
            sign = -sign;
        }

        val *= front_coef;
        buf[idx] = val;
    }
    if (nHarmonics > n_stack) {
        free(pre_n);
    }
}

void triangle_sin_harmonics_n_v0(Real* buf, size_s nSamples, Real sampleHz, Real noteHz, Real amplitude, int nHarmonics)
{

    Real front_coef = 8 / (M_PI * M_PI);
    Real sin_coef = M_PI * 2 * noteHz;

    Real step = 1 / sampleHz;
    for (size_s idx = 0; idx < nSamples; ++idx) {
        Real t = idx * step;

        Real val = 0;
        Real sign = 1;
        for (int i = 0; i < nHarmonics; ++i) {
            Real mode_number_n = 2 * i + 1;

            Real val_i = sign / (mode_number_n * mode_number_n) * sin(sin_coef * mode_number_n * t);

            val += val_i;
            sign = -sign;
        }

        val *= front_coef;
        buf[idx] = val;
    }
}

void triangle_abs_fmod(Real* buf, size_s nSamples, Real sampleHz, Real noteHz, Real amplitude)
{

    Real notePeriod = 1 / noteHz;

    Real q = 4 * amplitude * noteHz;
    Real w = notePeriod / 4;
    Real e = notePeriod / 2;

    Real step = 1 / sampleHz;
    for (size_s i = 0; i < nSamples; ++i) {
        Real t = i * step;
        Real val = q * fabs(fmod(t - w, notePeriod) - e) - amplitude;
        buf[i] = val;
    }
}

void triangle_asin_sin(Real* buf, size_s nSamples, Real sampleHz, Real noteHz, Real amplitude)
{

    Real front_coef = 2 * amplitude * M_1_PI;
    Real inner_coef = 2 * noteHz * M_PI;

    Real step = 1 / sampleHz;
    for (size_s i = 0; i < nSamples; ++i) {
        Real t = i * step;
        Real val = front_coef * asin(sin(inner_coef * t));
        buf[i] = val;
    }
}

const double test_sampleHz = 48'000.0; // samples per sec
const double test_toneDuration = 10.0; // sec
const size_s test_nSamples = size_s(ceil(test_sampleHz * test_toneDuration));

const double test_toneHz = 440; // A
const double test_amplitude = 1;

constexpr bool DEBUG_DUMP = false;

using BaseSig = decltype(triangle_abs_fmod);
template <BaseSig FN>
void BM_base(benchmark::State& state)
{

    auto name = state.name();
    int a = name.find('<') + 1;
    int b = name.length() - a - 1;
    auto fnName = name.substr(a, b);
    std::string dbg_name = fmt::format("{}.bin", fnName);

    // Perform setup here
    Real* buf = vecop::Tmalloc<Real>(test_nSamples);
    vecop::zero(buf, test_nSamples);

    FN(buf, test_nSamples, test_sampleHz, test_toneHz, test_amplitude);

    if (DEBUG_DUMP)
        vecop::dump_vec(buf, test_nSamples, dbg_name);

    for (auto _ : state) {
        // This code gets timed
        FN(buf, test_nSamples, test_sampleHz, test_toneHz, test_amplitude);
    }

    free(buf);
}

const int test_nHarmonics = 3;
using VarySig = decltype(triangle_sin_harmonics_n_v0);

template <VarySig FN>
void BM_vary(benchmark::State& state)
{
    auto name = state.name();
    int a = name.find('<') + 1;
    int b = name.length() - a - 1;
    auto fnName = name.substr(a, b);
    std::string dbg_name = fmt::format("{}.bin", fnName);

    // Perform setup here

    Real* buf = vecop::Tmalloc<Real>(test_nSamples);
    vecop::zero(buf, test_nSamples);

    FN(buf, test_nSamples, test_sampleHz, test_toneHz, test_amplitude, test_nHarmonics);

    if (DEBUG_DUMP)
        vecop::dump_vec(buf, test_nSamples, dbg_name);

    for (auto _ : state) {
        // This code gets timed
        FN(buf, test_nSamples, test_sampleHz, test_toneHz, test_amplitude, test_nHarmonics);
    }

    free(buf);
}

BENCHMARK(BM_base<triangle_abs_fmod>);
BENCHMARK(BM_base<triangle_asin_sin>);
BENCHMARK(BM_vary<triangle_sin_harmonics_n_v0>);
BENCHMARK(BM_vary<triangle_sin_harmonics_n_v1>);
BENCHMARK(BM_base<triangle_sin_harmonics_3_v0>);

// Run the benchmark
BENCHMARK_MAIN();