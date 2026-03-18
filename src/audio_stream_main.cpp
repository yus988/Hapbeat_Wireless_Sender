#include <Arduino.h>
#include <espnow_manager.h>

#include <audioStreamSender.h>

void setup() {
  Serial.setRxBufferSize(4096);
  Serial.begin(921600);

  espnowManager::initEspNow();

  audioStreamSender::init();
  Serial.println("=== Audio Stream Mode (minimal) ===");
}

void loop() {
  // Serial source only (PC→USB CDC→ESP32C3)
#ifdef STREAM_SOURCE_SERIAL
  audioStreamSender::processSerialStream();
#endif
  delay(1);
}

