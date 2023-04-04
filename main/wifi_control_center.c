#include "esp32_golbal.h"
#include "wifi_control_center.h"
#include "esp_wifi.h"
#include "lwip/sockets.h"



#define SERVER_LISTEN_UDP_PORT 1028
#define LOCAL_UDP_PORT 1027

#define ALWAYS_TRY_TO_CONNET_TO_SEVER 1


extern esp_netif_ip_info_t ipInfo;

//static const char *TAG = "wifi control center";
char deviceName[32] = "My first scanner";

opCode startWifiControlCenter() {
    uint32_t stackSize = 8192;
    BaseType_t xReturn;
    xReturn = xTaskCreatePinnedToCore(wifiControlCenterTask, "wifi_control_center_task", stackSize, NULL, 10, NULL, 1);
    if (xReturn == pdPASS) {
        printf("wifi control center successfully started!\n");
        return opSucceed;
    } else {
        printf("start wifi control center failed!\n");
        return opFailed;
    }
}

void wifiControlCenterTask(void *parameter) {
    /*
    if (parameter == NULL) {
        printf("wifi control center cannot get server info!gona close wifi cotrol center!\n");
        return;
    }
    */
    //ServerInfo *pServerInfo = (struct SererInfo *)parameter;
    struct ServerInfo serverInfo;
    while (true) {
          searchServer(&serverInfo);  
    }
}

opCode getServerInfo(char *data, int len, struct ServerInfo *pServerInfo) {
    opCode opReuslt = opSucceed;

    return opResult;
}

opCode searchServer(const struct ServerInfo *pServerInfo) {
    int result;
    int udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket < 0) {
        //ESP_LOGI(TAG, "create udp socket failed!\n");
        printf("create udp socket failed!\n");
        close(udpSocket);
        return opFailed;
    } else {
        int opt = 1;
        result = setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
        if (result != 0) {
            printf("enable socket broadcast option failed!!!\n");
            close(udpSocket);
            return opFailed;
        } else {
            struct sockaddr_in localAddr;
            bzero(&localAddr, sizeof(struct sockaddr_in));
            localAddr.sin_family = AF_INET;
            localAddr.sin_port = htons(LOCAL_UDP_PORT);
            localAddr.sin_addr.s_addr = *(unsigned int *)&ipInfo.ip;
            result = bind(udpSocket, &localAddr, sizeof(struct sockaddr_in));
            if (result < 0) {
                printf("bind to udp port failed!!!check if the port is used!!!\n");
                close(udpSocket);
                return opFailed;
            } else {
                struct timeval recvTimeout = {1, 0};
                result = setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout));
                if (result < 0) {
                    printf("socket option SO_RCVTIMEO not supported\n");
                    close(udpSocket);
	                return opFailed;
                }

                struct sockaddr_in remoteAddr;
                bzero(&remoteAddr, sizeof(struct sockaddr_in));
                remoteAddr.sin_family = AF_INET;
                remoteAddr.sin_port = htons(SERVER_LISTEN_UDP_PORT);
                //remoteAddr.sin_addr.s_addr = inet_addr("192.168.1.7");
                //remoteAddr.sin_addr.s_addr = inet_addr("192.168.1.2");
                remoteAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST); //255.255.255.255需要在同一个子网下才能收到广播
                //remoteAddr.sin_addr.s_addr = htonl(INADDR_ANY);
                int sendLen = 0;
                char sendBuf[128];
                sprintf(sendBuf, "I\'m here with esp32.Where is my friend?I\'m {%s}.", deviceName);
                int strLen = strlen(sendBuf);
                int errCount = 0;
                while (true) {
                    sendLen = 0;
                    sendLen = sendto(udpSocket, &sendBuf, strLen, 0, &remoteAddr, sizeof(struct sockaddr_in));
                    if (sendLen != strLen) {
                        printf("udp broadcast had error!!!\n");
                        errCount++;
                        if (ALWAYS_TRY_TO_CONNET_TO_SEVER != 0) {
                            errCount = 0;
                        }
                        if (errCount > 10) {
                            printf("udp broadcast reach maximun error count!!!gona end broadcast!!!\n");
                            close(udpSocket);
                            return opFailed;
                        } else {
                            vTaskDelay(MS_TO_TICK(500));
                        }
                    } else {
                        //printf("ready to recieve data from server!!!\n");
                        char recvBuf[1024];
                        int recvLen = 0;
                        bzero(&recvBuf, sizeof(recvBuf));
                        struct sockaddr_in serverAddr;
                        bzero(&serverAddr, sizeof(struct sockaddr_in));
                        //serverAddr.sin_family = AF_INET;
                        //serverAddr.sin_port = htons(SERVER_LISTEN_UDP_PORT);
                        //serverAddr.sin_addr.s_addr = inet_addr(INADDR_ANY);
                        //serverAddr.sin_addr.s_addr = inet_addr("192.168.1.7");
                        //printf("test point3!!!\n");
                        socklen_t sockLen = sizeof(struct sockaddr_in);
                        //len = recvfrom(udpSocket, &recvBuf, sizeof(recvBuf), 0, &serverAddr, &socklen);
                        recvLen = recvfrom(udpSocket, &recvBuf, sizeof(recvBuf), 0, &serverAddr, &sockLen);
                        if (recvLen < 0) {
                            if (errno == EWOULDBLOCK || errno == EAGAIN)
                                printf("recvfrom timeout\n");
                            else
                                printf("recvfrom err:%d\n", errno);
                        } else {
                                //printf("data received!!!\n");
                                //""IP2STR((ip4_addr_t *)&serverAddr.sin_addr.s_addr)""
                                //IP2STR((ip4_addr_t *)&serverAddr.sin_addr.s_addr)
                                //printf("receive data : %s, length : %d, server ip : %s\n", recvBuf, len, inet_ntoa(serverAddr.sin_addr.s_addr));
                            printf("receive data : %s, length : %d, server ip : %s\n", recvBuf, recvLen, inet_ntoa(serverAddr.sin_addr.s_addr));
                            getServerInfo(recvBuf, recvLen, pServerInfo);
                        }
                    }
                }
            }
        }
    }
}
 