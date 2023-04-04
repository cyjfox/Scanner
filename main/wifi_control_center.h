#include "esp_system.h"


#define opSucceed 0
#define opFailed -1

struct ServerInfo {
    in_addr_t ip;
    uint16_t port;
} 

typedef int32_t opCode;

opCode searchServer(const struct ServerInfo *pServerInfo);

opCode startWifiControlCenter();

opCode getServerInfo(char *data, int len, struct ServerInfo *pServerInfo);

void wifiControlCenterTask(void *parameter);