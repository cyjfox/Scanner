#pragma once
#include "hal/dac_types.h"
#include "esp_err.h"
#include "signal_generator.h"


//const double pi = 3.1415926535;
#ifndef pi_defined
static const double pi = 3.14159265358979323846264338327950288;
#define pi_defined 1
#endif
esp_err_t sin_wave_start(dac_channel_t channel, double freq, double phase);

esp_err_t sin_wave_stop(dac_channel_t channel);

esp_err_t sin_wave(dac_channel_t channel, double freq);