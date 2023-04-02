#include "esp32_golbal.h"
#include "esp_wifi.h"
#include "lwip/sockets.h"

#define opSucceed 0
#define opFailed -1

#define SERVER_LISTEN_UDP_PORT 1028
#define LOCAL_UDP_PORT 1027

#define ALWAYS_TRY_TO_CONNET_TO_SEVER 1


extern esp_netif_ip_info_t ipInfo;

//static const char *TAG = "wifi control center";
char deviceName[32] = "My first scanner";
typedef int32_t opCode;

opCode searchServer() {
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
                bzero(&remoteAddr, sizeof(struct sockaddr_in));
                remoteAddr.sin_family = AF_INET;
                remoteAddr.sin_port = htons(SERVER_LISTEN_UDP_PORT);
                //remoteAddr.sin_addr.s_addr = inet_addr("192.168.1.7");
                //remoteAddr.sin_addr.s_addr = inet_addr("192.168.1.2");
                remoteAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
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
                            erroCount = 0;
                        }
                        if (errCount > 10) {
                            printf("udp broadcast reach maximun error count!!!gona end broadcast!!!\n");
                            close(udpSocket);
                            return opFailed;
                        } else {
                            vTaskDelay(MS_TO_TICK(500));
                        }
                    }
                }
                

            }
        }
    }
}
 controllerTask(void * parameter) {
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
