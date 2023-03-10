#include "signal_generator.h"
#include "esp_check.h"
#include "driver/dac_common.h"
#include "math.h"
//#include "esp_rom_sys.h"
#include "driver/timer.h"

static const char *TAG = "signal generator";

typedef uint8_t (*)(int16_t index) DataGenerarotor;


uint8 ECGSignalGenerator(int16_t index) {
    index = index %
}

uint8_t current_amp;
uint64_t next_alarm_value;
//double[1000] * const pSignal;
DataGeneratrotor pDataGenerator;
uint16_t i;
double freq;
double amp;
void IRAM_ATTR timer_update_dac(void *arg) {
    //printf("interupted!\n");
    //dac_output_voltage(current_channel, current_amp);
    dac_output_voltage(current_channel, current_amp);
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    
    if (i >= 0 && i < 128) {
        //next_alarm_value = round(t[i + 1] - t[i] / current_freq / 0.000000025);
        next_alarm_value = round(t[i] / current_freq / time_per_tick);
        current_amp++;
        if (current_amp > max_amp) {
            max_amp = current_amp;
        }

        
        if (current_amp > 255) {
            current_amp = 255;
        }
        
    } else if (i >= 128 && i < 256) {
        //next_alarm_value = round(t[128 - i + 1] - t[128 - i] / current_freq / 0.000000025);
        next_alarm_value = round(t[255 - i] / current_freq / time_per_tick); //127 - (i - 128) = 255 - i
        current_amp--;
    } else if (i >= 256 && i < 256 + 128) {
        //next_alarm_value = round(t[256 - i + 1] - t[256 - i] / current_freq / 0.000000025);
        next_alarm_value = round(t[i - 256] / current_freq / time_per_tick); //i - 256
        current_amp--;
        if (current_amp < 0) {
            current_amp = 0;
        }
    } else if (i >= 256 + 128 && i < 512) {
        //next_alarm_value = round(t[512 - i + 1] - t[512 - i] / current_freq / 0.000000025);
        next_alarm_value = round(t[511 - i] / current_freq / time_per_tick); // 127 - (i - 256 - 128) = 511 - i
        current_amp++;
    }

    i++;
    i = i % 512;
    
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, next_alarm_value);
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);
}

esp_err_t signal_start(dac_channel_t channel, uint32_t delay_ms, double phase, double offset, double amplitude, double frequency, DataGenerarotor pPDataGenrator);
{
    ESP_RETURN_ON_FALSE(channel < DAC_CHANNEL_MAX, ESP_ERR_INVALID_ARG, TAG, "DAC channel error");
    dac_output_enable(current_channel);
    
    pDataGenrator = pPDataGenerator;
    freq = frequecney;
    amp = amplitude;
    

    //TODO
    //通过phase计算启动相位，从而得出i的初始值和current_amp的初始值
    i = 0;
    
    timer_config_t timer_config = {
        .divider = 2,//2分频，APB时钟为80Mhz，2分频后周期为25ns
        .counter_dir = TIMECOR_UNT_UP,//向上计数
        .counter_en = TIMER_PAUSE,//等待手动开启
        .alarm_en = TIMER_ALARM_EN,//计数到达终点报警中断
        .auto_reload = TIMER_AUTORELOAD_EN,//自动重装重新计时
        .intr_type = TIMER_INTR_LEVEL,//电平中断
    };
    
    //next_alarm_value = 0;
    next_alarm_value = delay_ms * 0.001 / time_per_tick;//5ms延时后输出第一个数据
    printf("next_alarm_value to start the timer is : %d\n", (int)next_alarm_value);
    //current_amp = 128;
    esp_err_t result;
    result = timer_init(TIMER_GROUP_0, TIMER_0, &timer_config);
    if (result == ESP_OK) {
        //printf("timer_init ok!\n");
    } else {
        printf("timer_init failed!!!\n");
    }
    result = timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
    if (result == ESP_OK) {
        //printf("timer_set_counter_value ok!\n");
    } else {
        printf("timer_set_counter_value failed!!!\n");
        ESP_RETURN_ON_ERROR(result, TAG, "timer_set_counter_value failed!!!\n");
    }
    result = timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, next_alarm_value);
    if (result == ESP_OK) {
        //printf("timer_set_alarm_value ok!\n");
    } else {
        printf("timer_set_alarm_value failed!!!\n");
        ESP_RETURN_ON_ERROR(result, TAG, "timer_set_alarm_value failed!!!\n");
    }
    result = timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    if (result == ESP_OK) {
        //printf("timer_enable_intr ok!\n");
    } else {
        printf("timer_enable_intr failed!!!\n");
        ESP_RETURN_ON_ERROR(result, TAG, "timer_enable_intr failed!!!\n");
    }
    //result = timer_isr_register(TIMER_GROUP_0, TIMER_0, &timer_update_dac, NULL, ESP_INTR_FLAG_SHARED, NULL);
    result = timer_isr_register(TIMER_GROUP_0, TIMER_0, &timer_update_dac, NULL, ESP_INTR_FLAG_IRAM, NULL);
    if (result == ESP_OK) {
        //printf("timer_isr_register ok!\n");
    } else {
        printf("timer_isr_register failed!!!\n");
        ESP_RETURN_ON_ERROR(result, TAG, "timer_isr_register failed!!!\n");
    }
    result = timer_start(TIMER_GROUP_0, TIMER_0);
    if (result == ESP_OK) {
        printf("timer_start ok!\n");
    } else {
        printf("timer_start failed!!!\n");
        ESP_RETURN_ON_ERROR(result, TAG, "timer_start failed!!!\n");
    }
    return ESP_OK;
}

esp_err_t signal_stop(dac_channel_t channel)
{
    ESP_RETURN_ON_FALSE(channel < DAC_CHANNEL_MAX, ESP_ERR_INVALID_ARG, TAG, "DAC channel error");
    dac_output_disable(channel);
    return timer_pause(TIMER_GROUP_0, TIMER_0);
}