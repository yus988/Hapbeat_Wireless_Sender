#ifndef AUDIO_STREAM_SENDER_H
#define AUDIO_STREAM_SENDER_H

#include <Arduino.h>

#ifndef STREAM_SAMPLE_RATE
#define STREAM_SAMPLE_RATE 8000
#endif

#ifndef FRAMES_PER_PACKET
#define FRAMES_PER_PACKET 40
#endif

#define STREAM_PKT_TYPE 0xAA

namespace audioStreamSender {

struct StereoFrame {
  int16_t left;
  int16_t right;
};

// Call after espnowManager::initEspNow()
void init();

// Serial binary input: call from loop()
void processSerialStream();

// M5Stack Basic向けの簡易波形表示（ENABLE_DISPLAY時のみ有効）
void drawScopeIfEnabled();

}  // namespace audioStreamSender

#endif
