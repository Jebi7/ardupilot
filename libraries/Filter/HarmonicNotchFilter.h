/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <AP_Math/AP_Math.h>
#include <cmath>
#include <AP_Param/AP_Param.h>
#include "NotchFilter.h"

#define HNF_MAX_HARMONICS 8

/*
  a filter that manages a set of notch filters targetted at a fundamental center frequency
  and multiples of that fundamental frequency
 */
template <class T>
class HarmonicNotchFilter {
public:
    ~HarmonicNotchFilter();
    // allocate a bank of notch filters for this harmonic notch filter
    void allocate_filters(uint8_t num_notches, uint8_t harmonics, bool double_notch);
    // initialize the underlying filters using the provided filter parameters
    void init(float sample_freq_hz, float center_freq_hz, float bandwidth_hz, float attenuation_dB);
    // update the underlying filters' center frequencies using center_freq_hz as the fundamental
    void update(float center_freq_hz);
    // update all o fthe underlying center frequencies individually
    void update(uint8_t num_centers, const float center_freq_hz[]);
    // apply a sample to each of the underlying filters in turn
    T apply(const T &sample);
    // reset each of the underlying filters
    void reset();

private:
    // underlying bank of notch filters
    NotchFilter<T>*  _filters;
    // sample frequency for each filter
    float _sample_freq_hz;
    // base double notch bandwidth for each filter
    float _notch_spread;
    // attenuation for each filter
    float _A;
    // quality factor of each filter
    float _Q;
    // a bitmask of the harmonics to use
    uint8_t _harmonics;
    // whether to use double-notches
    bool _double_notch;
    // number of allocated filters
    uint8_t _num_filters;
    // pre-calculated number of harmonics
    uint8_t _num_harmonics;
    // number of enabled filters
    uint8_t _num_enabled_filters;
    bool _initialised;
};

// Harmonic notch update mode
enum class HarmonicNotchDynamicMode {
    Fixed           = 0,
    UpdateThrottle  = 1,
    UpdateRPM       = 2,
    UpdateBLHeli    = 3,
    UpdateGyroFFT   = 4,
    UpdateRPM2      = 5,
};

/*
  harmonic notch filter configuration parameters
 */
class HarmonicNotchFilterParams : public NotchFilterParams {
public:
    enum class Options {
        DoubleNotch = 1<<0,
        DynamicHarmonic = 1<<1,
        LoopRateUpdate = 1<<2,
        EnableOnAllIMUs = 1<<3,
    };

    HarmonicNotchFilterParams(void);
    // set the fundamental center frequency of the harmonic notch
    void set_center_freq_hz(float center_freq) { _center_freq_hz.set(center_freq); }
    // set the bandwidth of the harmonic notch
    void set_bandwidth_hz(float bandwidth_hz) { _bandwidth_hz.set(bandwidth_hz); }
    // harmonics enabled on the harmonic notch
    uint8_t harmonics(void) const { return _harmonics; }
    // has the user set the harmonics value
    void set_default_harmonics(uint8_t hmncs) { _harmonics.set_default(hmncs); }
    // reference value of the harmonic notch
    float reference(void) const { return _reference; }
    void set_reference(float ref) { _reference = ref; }
    // notch options
    bool hasOption(Options option) const { return _options & uint16_t(option); }
    // notch dynamic tracking mode
    HarmonicNotchDynamicMode tracking_mode(void) const { return HarmonicNotchDynamicMode(_tracking_mode.get()); }
    static const struct AP_Param::GroupInfo var_info[];

    // save parameters
    void save_params();

private:
    // configured notch harmonics
    AP_Int8 _harmonics;
    // notch reference value
    AP_Float _reference;
    // notch dynamic tracking mode
    AP_Int8 _tracking_mode;
    // notch options
    AP_Int16 _options;
};

typedef HarmonicNotchFilter<Vector3f> HarmonicNotchFilterVector3f;
typedef HarmonicNotchFilter<float> HarmonicNotchFilterFloat;

