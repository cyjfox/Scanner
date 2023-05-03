#include "esp_system.h"

#define SERVER_LISTEN_UDP_PORT 1028
#define LOCAL_UDP_PORT 1027
#define SERVER_LISTEN_TCP_PORT 1026
#define LOCAL_TCP_PORT 1025
#define MAX_UDP_CONNECT_RETRY 100

#define opSucceed 0
#define opFailed -1

struct ServerInfo {
    in_addr_t ip;
    uint16_t port;
};

typedef int32_t OpCode;

OpCode searchServer(const struct ServerInfo *pServerInfo);

OpCode startWifiControlCenter();

OpCode getServerInfo(char *data, int len, struct ServerInfo *pServerInfo);

void wifiControlCenterTask(void *parameter);