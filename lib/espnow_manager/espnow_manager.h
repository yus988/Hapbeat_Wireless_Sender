#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

namespace espnowManager {
  void initEspNow(void);
  void sendSerialViaESPNOW(void);
  void SendEspNOW(const char* cmd);
}

#endif
