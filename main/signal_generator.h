#include "hal/dac_types.h"
#include "esp_err.h"

const double time_per_tick = 0.000000025;

esp_err_t signal_start(dac_channel_t channel, uint32_t delay_ms, double phase, double offset, double amplitude, double frequency, const double[1000] * const signal);
esp_err_t signal_stop(dac_channel_t channel);

