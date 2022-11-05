/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "hal/dac_types.h"
#include "driver/dac_common.h"
#include "sin_generator.h"
//include "esp_blufi_api.h"
/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/



#define EXAMPLE_ESP_WIFI_SSID      "TP-LINK_69CC"
#define EXAMPLE_ESP_WIFI_PASS      "2253501gongxifacai"
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

int max_amp = 0;
//计算能达到DA最小分辨率1/255 * VCC毫伏的最小刷新率。
//在导数最大的地方，对于正弦波来说是在0时处，将有最大的变化率
//在导数最小的地方，对于正弦波来说时在1/4pi处，将有最小的变化率
//将正弦波分为255段，每段比前一段大1/255*max_amplitude。这样在变化快的地方采样多，在变化小的地方采样少，能使每个可以检测到的变化都被采样出来


esp_netif_ip_info_t ipInfo;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ipInfo = &event->ip_info;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}
/*
esp_err_t blufi() {
    //esp_bt_controller_mem_release();
    esp_err_t ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        BLUFI_ERROR("%s initialized bt controller failed!\nerror name : %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        BLUFI_ERROR("%s enable bt controller failed!\nerror name : %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }
    ret = esp_blufi_host_and_cb_init(&f);
    if (ret) {
        BLUFI_ERROR("%s initilized failed!\nerror name : %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }

    BLUFI_INFO("BLUFI VERSION : %04X\n", esp_blufi_get_version());

    return ESP_OK;
}
*/

#define SERVER_LISTEN_UDP_PORT 48235
#define LOCAL_UDP_PORT 48230
#define MAX_UDP_CONNECT_RETRY 100
void controllerTask(void * parm) {
    const char * TAG = "controllerTask";
    socket udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        ESP_LOGI(TAG, "create udp socket failed!\n");
        close(udpSocket);
    } else {
        struct sockaddr_in localAddr;
        bzero(&localAddr, sizeof(sockaddr_in));
        localAddr.sin_family = AF_INET;
        localAddr.sin_port = htons(LOCAL_UDP_PORT);
        localAddr.sin_addr = htonl(INADDR_ANY);

        uint8_t result = bind(udpSocket, &localAddr, sizeof(localAddr));
        if (result != 0) {
            ESP_LOGI(TAG, "bind to udp port failed!!!check if the port is used!!!\n")
        } else {
            struct sockaddr_in remoteAddr;
            bzero(&remoteAddr, sizeof(sockaddr_in));
            remoteAddr.sin_family = AF_INET;
            remoteAddr.sin_port = htons(SERVER_LISTEN_UDP_PORT);
            remoteAddr.sin_addr.s_addr = inet_addr("");
            setsockopt(udpSocket, )

            int len = 0;
            char [1024] buf;
            for (int i == 0; i < MAX_UDP_CONNECT_RETRY; i++) {
                len = sendto(udpSocket, buf, 1024, 0, remoteAddr, sizeof(remoteAddr));
            }
        }
        
    }

}

void app_main(void)
{
    printf("preparing to run main......\n");
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    
    //sin_wave_start(DAC_CHANNEL_1, 110.0, 0.0);
    //printf("start sin wave done!\n");
    //vTaskDelay(1);
    printf("test point!\n");

    xHandler_t controllerTaskHandle = NULL;
    BaseType_t xReturn;
    xReturn = xTaskCreate(controllerTask, "Contorller", 256, NULL, 2, &controllerTaskHandle);
    if (xReturn != pdPass) {
        printf("create controller task failed...\n");
    }

    vTaskStartScheduler();

    while (true) {
        /*
        dac_output_voltage(DAC_CHANNEL_1, 128);
        printf("Hello!cyjfox!\n");
        sleep(1);
        dac_output_voltage(DAC_CHANNEL_1, 0);
        printf("forward!\n");
        sleep(1);
        */
       //sin_wave(DAC_CHANNEL_1, 85.0);
       //printf("test point2!\n");
       //sleep(1);
       //printf("max_amp is : %d\n", max_amp);
       //printf("again!!!\n");
       //printf("456 Hz\n");
       printf("system error!!!\n");
       sleep(1);
       
    }
}
