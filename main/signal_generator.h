#pragma once
#include "hal/dac_types.h"
#include "esp_err.h"


//const double time_per_tick = 0.000000025;
#ifndef time_per_tick
#define time_per_tick 0.000000025
#endif

double ecgAmplitudeTable[1000];

esp_err_t signal_start(dac_channel_t channel, uint32_t delay_ms, double phase, double offset, double amplitude, double frequency, double (*pSignal)[1000]);
esp_err_t signal_stop(dac_channel_t channel);

