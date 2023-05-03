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
#include "lwip/sockets.h"
#include "signal_generator.h"
#include "esp_clk.h"
#include "esp32_golbal.h"
#include "wifi_control_center.h"

//include "esp_blufi_api.h"
/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

extern uint8_t current_amp;
extern uint8_t err_code;
extern uint16_t bbb;
extern double aaa;
extern uint32_t phase_count;
extern double ecgAmplitudeTable[1000];
extern uint32_t g_k;
extern uint64_t next_alarm_value;
int apb_freq;
#define EXAMPLE_ESP_WIFI_SSID      "TP-LINK_69CC"
#define EXAMPLE_ESP_WIFI_PASS      "2253501gongxifacai"
//#define EXAMPLE_ESP_WIFI_SSID           "30max"
//#define EXAMPLE_ESP_WIFI_PASS           "2253501fox"
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1




char globalBuf[1024];
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
        //ipInfo = &event->ip_info;
        memcpy(&ipInfo, &event->ip_info, sizeof(esp_netif_ip_info_t));
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "cyjfox copy ip : " IPSTR, IP2STR(&ipInfo.ip));
        ESP_LOGI(TAG, "cyjfox copy mask : " IPSTR, IP2STR(&ipInfo.netmask));
        ESP_LOGI(TAG, "cyjfox copy gateway : " IPSTR, IP2STR(&ipInfo.gw));
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
void debugTask(void * parameter) {
    while (true) {
        vTaskGetRunTimeStats((char *)&globalBuf);
        printf("task status : \n");
        printf("%s\n", globalBuf);
        vTaskList((char *)&globalBuf);
        printf("task list : \n");
        printf("%s\n", globalBuf);
        vTaskDelay(MS_TO_TICK(300));
    }
}


void monitorTask(void * parameter) {
    while (true) {
        /*
        printf("current amp in moniter task is : %d\n", current_amp);
        printf("err_code is : %d\n", err_code);
        printf("aaa is : %f\n", aaa);
        printf("bbb is : %d\n", bbb);
        
        printf("index 999 is : %f\n", ecgAmplitudeTable[999]);
        
        printf("g_k is : %d\n", g_k);
        printf("next_alarm_value is : %lld\n", next_alarm_value);
        printf("apb_freq is : %d\n", apb_freq);
        */
        printf("phase_count is : %d\n", phase_count);
        vTaskDelay(5);
        //vTaskDelay(MS_TO_TICK(2));
        //sleep(2);
    }
}

//#define SERVER_LISTEN_UDP_PORT 48235
//#define LOCAL_UDP_PORT 48230

void controllerTask(void * parameter) {
    const char * TAG = "controllerTask";
    int len = 0;
    char sendBuf[1024] = "abcdefg123";
    int count = 0;
    int result;
    bool sendDataSucceeded = false;
    struct sockaddr_in remoteAddr;
    /*
    while (true) {
        ESP_LOGI(TAG, "running controller task!\n");//ESP_LOGI版本大概需要1700单位最小堆栈
        //printf("printing : running controller task!\n");//printf版本大概用700单位堆栈
        //vTaskDelay(500 / portTICK_PERIOD_MS);
        vTaskDelay(MS_TO_TICK(1000));
    }
    */
    
    int udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket < 0) {
        ESP_LOGI(TAG, "create udp socket failed!\n");
        close(udpSocket);
    } else {
        ESP_LOGI(TAG, "create udp socket succeeded!\n");
        printf("udp socket is %d\n", udpSocket);
        struct sockaddr_in localAddr;
        bzero(&localAddr, sizeof(struct sockaddr_in));
        localAddr.sin_family = AF_INET;
        localAddr.sin_port = htons(LOCAL_UDP_PORT);
        //localAddr.sin_addr = htonl(INADDR_ANY);
         
        //即unsigned int a1 =  *(unsigned int *)&((struct in_addr) a2);
        localAddr.sin_addr.s_addr = *(unsigned int *)&ipInfo.ip;
        //localAddr.sin_addr.s_addr = inet_addr("192.168.1.4");
        printf("local ip is %08X\n", localAddr.sin_addr.s_addr);
        int opt = 1;
        result = setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
        
        //result = 0;
        if (result != 0) {
            
            printf("enable socket broadcast option failed!!!\n");
            close(udpSocket);
            vTaskDelay(MS_TO_TICK(500));
        } else {
            ESP_LOGI(TAG, "enable socket broadcast option succeeded!\n");
            
            
            result = bind(udpSocket, &localAddr, sizeof(struct sockaddr_in));
            
            //result = 0;
            if (result < 0) {
                ESP_LOGI(TAG, "bind to udp port failed!!!check if the port is used!!!\n");
                close(udpSocket);
                vTaskDelay(MS_TO_TICK(500));
            } else {
                ESP_LOGI(TAG, "bind to local ip succeeded!\n");
                //struct sockaddr_in remoteAddr;
                bzero(&remoteAddr, sizeof(struct sockaddr_in));
                remoteAddr.sin_family = AF_INET;
                remoteAddr.sin_port = htons(SERVER_LISTEN_UDP_PORT);
                //remoteAddr.sin_addr.s_addr = inet_addr("192.168.1.7");
                //remoteAddr.sin_addr.s_addr = inet_addr("192.168.1.2");
                remoteAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
                //remoteAddr.sin_addr.s_addr = htonl(INADDR_ANY);
                //emoteAddr.sin_addr.s_addr = inet_addr("192.168.1.255");
                //remoteAddr.sin_addr.s_addr = *(unsigned int *)&ipInfo.gw;
                count = 3;
                
                //for (int i = 0; i < MAX_UDP_CONNECT_RETRY; ) {
                for (int i = count; i > 0; i--) {
                    //count++;
                    sprintf(sendBuf, "preparing to receive, count %d\n", count--);
                    //printf("gonna send upd data!\n");
                    //printf("udpSocket is %d, sendBuf is %s, length of sendBuf is :%d, pointer to remoteAddr is : %08x, size of remoteAddr is : %d\n", udpSocket, sendBuf, strlen(sendBuf), (unsigned int)&remoteAddr, sizeof(struct sockaddr_in));
                    len = sendto(udpSocket, &sendBuf, strlen(sendBuf), 0, &remoteAddr, sizeof(struct sockaddr_in));
                    //printf("udp broadcast done!!!\n");
                    if (len < 0) {
                        printf("udp broadcast failed!!!\n");
                        vTaskDelay(MS_TO_TICK(1000));
                    } else {
                        //printf("send udp data succeeded!!!\n");
                        printf("count is %d, length of data send is : %d, original data length is : %d\n", count, len, strlen(sendBuf));
                        sendDataSucceeded = true;
                        //printf("test point2\n");
                        //flush();
                        vTaskDelay(MS_TO_TICK(1000));
                        //break;
                    }
                }
                
                while (sendDataSucceeded) {
                    
                    len = 0;
                    //char recvBuf[1024];
                    //printf("size of recvBuf is : %d\n", sizeof(recvBuf));
                    //bzero(&recvBuf, sizeof(recvBuf));
                    bzero(&globalBuf, sizeof(globalBuf));
                    //printf("test point2!!!\n");
                    struct sockaddr_in serverAddr;
                    bzero(&serverAddr, sizeof(struct sockaddr_in));
                    //serverAddr.sin_family = AF_INET;
                    //serverAddr.sin_port = htons(SERVER_LISTEN_UDP_PORT);
                    //serverAddr.sin_addr.s_addr = inet_addr(INADDR_ANY);
                    //serverAddr.sin_addr.s_addr = inet_addr("192.168.1.7");
                    //printf("test point3!!!\n");
                    socklen_t socklen = sizeof(struct sockaddr_in);
                    //len = recvfrom(udpSocket, &recvBuf, sizeof(recvBuf), 0, &serverAddr, &socklen);
                    len = recvfrom(udpSocket, &globalBuf, sizeof(globalBuf), 0, &serverAddr, &socklen);
                    printf("data received!!!\n");
                    //""IP2STR((ip4_addr_t *)&serverAddr.sin_addr.s_addr)""
                    //IP2STR((ip4_addr_t *)&serverAddr.sin_addr.s_addr)
                    //printf("receive data : %s, length : %d, server ip : %s\n", recvBuf, len, inet_ntoa(serverAddr.sin_addr.s_addr));
                    printf("receive data : %s, length : %d, server ip : %s\n", globalBuf, len, inet_ntoa(serverAddr.sin_addr.s_addr));
                    
                    //vTaskDelay(MS_TO_TICK(300));
                    /*
                    //use tcp to test
                    struct sockaddr_in tcpServerAddr;
                    bzero(&tcpServerAddr, sizeof(struct sockaddr_in));
                    tcpServerAddr.sin_family = AF_INET;
                    tcpServerAddr.sin_port = htons(SERVER_LISTEN_TCP_PORT);
                    tcpServerAddr.sin_addr.s_addr = inet_addr("192.168.1.7");
                    //IPPROTO_TCP
                    int tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
                    if (tcpSocket < 0) {
                        close(tcpSocket);
                        printf("create tcp socket failed!!!\n");
                        continue;
                    }
                    //EADDRINUSE
                    printf("tcp socket is : %d\n", tcpSocket);
                    struct sockaddr_in tcpLocalAddr;
                    bzero(&tcpServerAddr, sizeof(struct sockaddr_in));
                    tcpLocalAddr.sin_family = AF_INET;
                    tcpLocalAddr.sin_port = htons(LOCAL_TCP_PORT);
                    //tcpLocalAddr.sin_addr.s_addr = inet_addr("192.168.1.4");
                    tcpLocalAddr.sin_addr.s_addr = *(unsigned int *)&ipInfo.ip;
                    //result;
                    result = bind(tcpSocket, &tcpLocalAddr, sizeof(struct sockaddr_in));
                    //result = 0;
                    if (result != 0) {
                        close(tcpSocket);
                        printf("bind tcp local address failed!!! error code : %d\n", result);
                        vTaskDelay(MS_TO_TICK(500));
                    } else {
                        printf("tcp bind succeeded!!!\n");
                        socklen_t socklen = sizeof(struct sockaddr_in);

                        int clientSocket = accept(tcpSocket, &remoteAddr, &socklen);
                        if (clientSocket == -1) {
                            printf("accept failed!!!\n");
                            close(tcpSocket);
                            vTaskDelay(MS_TO_TICK(500));
                            continue;
                        } else {
                            printf("accept succeeded!!!got a client!!!\n");
                            printf("client socket %d, client ip : %s\n", clientSocket, inet_ntoa(remoteAddr.sin_addr.s_addr));
                            while (true) {
                                vTaskDelay(MS_TO_TICK(500));
                            }
                        }
                        */
                        /*
                        if (0 == (result = connect(tcpSocket, &tcpServerAddr, sizeof(struct sockaddr_in)))) {
                            printf("connect to tcp server succeeded!!!\n");
                            while (true) {
                                sprintf(sendBuf, "tcp count is %d\n", ++count);
                                len = send(tcpSocket, sendBuf, strlen(sendBuf), 0);
                                printf("tcp send data length : %d, data : %s\n", len, sendBuf);
                                vTaskDelay(MS_TO_TICK(500));
                            }
                        } else {
                            printf("connect tot tcp server failed!!!, result code is %d\n", result);
                            close(tcpSocket);
                            vTaskDelay(MS_TO_TICK(1000));
                            continue;
                        }
                        

                    }
                    */
                }
                
                
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
    //tcpip_adapter_init();
    wifi_init_sta();
    esp_wifi_set_ps(WIFI_PS_NONE);
    //wifi_set_broadcast_if(STATIONAP_MODE);
    //sin_wave_start(DAC_CHANNEL_1, 110.0, 0.0);
    //printf("start sin wave done!\n");
    //vTaskDelay(1);
    apb_freq = esp_clk_apb_freq();
    
    printf("test point!\n");
    
    TaskHandle_t controllerTaskHandle = NULL;
    //BaseType_t xReturn;
    //UBaseType_t maxStack = 8192;
    UBaseType_t maxStack = 8192;
    //xTaskCreateStatic
    //xTaskCreate(controllerTask, "ContorllerTask", maxStack, NULL, 2, &controllerTaskHandle);
    //xTaskCreatePinnedToCore(controllerTask, "ContorllerTask", maxStack, NULL, 3, &controllerTaskHandle, 1);
    //xTaskCreatePinnedToCore(debugTask, "DebugTask", 4096, NULL, 2, NULL, 1);
    /*
    if (xReturn != pdPASS) {
        printf("create controller task failed...\n");
    } else {
        printf("create controller task succeed...\n");
    }
    */
   
    printf("create controller task succeed...\n");
    //vTaskStartScheduler();
    TaskHandle_t monitorTaskHandle = NULL;
    //UBaseType_t maxStack = 8192;
    //xTaskCreate(monitorTask, "MonitorTask", maxStack, NULL, 2, &monitorTaskHandle); 
    double count_per_min = 80;
    signal_start(DAC_CHANNEL_1, 5, 0.0, 0.0, 0.8, count_per_min / 60.0, &ecgAmplitudeTable);
    struct ServerInfo serverInfo;
    searchServer(&serverInfo);
    /*
    while (true) {
        vTaskGetRunTimeStats((char *)&globalBuf);
        printf("task status : \n");
        printf("%s\n", globalBuf);
        vTaskList((char *)&globalBuf);
        printf("task list : \n");
        printf("%s\n", globalBuf);
        vTaskDelay(MS_TO_TICK(800));
    }
    */
    //while (true) {
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
       
       /*
       UBaseType_t highestWaterLevel = uxTaskGetStackHighWaterMark(controllerTaskHandle);
       UBaseType_t maxStackUsed = maxStack - highestWaterLevel;
       //printf("max stack used in controller task is : %d, original stack size : %d, highest water level : %d\n", maxStackUsed, maxStack, highestWaterLevel);
       //printf("system error!!!\n");
       sleep(1);
       */
    //}
}
