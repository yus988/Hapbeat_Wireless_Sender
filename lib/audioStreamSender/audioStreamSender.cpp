#include "audioStreamSender.h"
#include <esp_now.h>
#include <esp_timer.h>

#if defined(ENABLE_DISPLAY) && __has_include(<M5Unified.h>)
  #include <M5Unified.h>
#endif

namespace audioStreamSender {

static const uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static uint8_t seqNum = 0;
static StereoFrame sampleBuf[FRAMES_PER_PACKET];
static volatile int bufIdx = 0;

// ---- Level meter (for M5Stack Basic) ----
#if defined(ENABLE_DISPLAY) && __has_include(<M5Unified.h>)
static volatile int16_t peakL = 0;
static volatile int16_t peakR = 0;
static volatile uint32_t pktCountESPNOW = 0;

static inline void updatePeaks(const StereoFrame* frames, uint16_t count) {
  int16_t mL = peakL, mR = peakR;
  for (uint16_t i = 0; i < count; i++) {
    int16_t aL = frames[i].left < 0 ? -frames[i].left : frames[i].left;
    int16_t aR = frames[i].right < 0 ? -frames[i].right : frames[i].right;
    if (aL > mL) mL = aL;
    if (aR > mR) mR = aR;
  }
  peakL = mL;
  peakR = mR;
}
#endif

// Max ESP-NOW payload = 250 bytes; header = 4 bytes
static_assert(4 + FRAMES_PER_PACKET * sizeof(StereoFrame) <= 250,
              "FRAMES_PER_PACKET too large for ESP-NOW payload");

// ---- Packet sending ----

static void sendStreamPacket(const StereoFrame *frames, uint16_t count) {
#if defined(ENABLE_DISPLAY) && __has_include(<M5Unified.h>)
  updatePeaks(frames, count);
  pktCountESPNOW++;
#endif
  uint8_t packet[250];
  packet[0] = STREAM_PKT_TYPE;
  packet[1] = seqNum++;
  packet[2] = (uint8_t)(count & 0xFF);
  packet[3] = (uint8_t)((count >> 8) & 0xFF);
  memcpy(&packet[4], frames, count * sizeof(StereoFrame));

  uint16_t totalLen = 4 + count * sizeof(StereoFrame);
  esp_now_send(broadcastAddr, packet, totalLen);
}

// ---- ADC mode ----

#ifdef STREAM_SOURCE_ADC

#ifndef STREAM_ADC_PIN_L
#define STREAM_ADC_PIN_L 1
#endif
#ifndef STREAM_ADC_PIN_R
#define STREAM_ADC_PIN_R 2
#endif

static esp_timer_handle_t adcTimer = nullptr;

static void IRAM_ATTR onAdcTimerCb(void *arg) {
  int rawL = analogRead(STREAM_ADC_PIN_L);
  int rawR = analogRead(STREAM_ADC_PIN_R);
  // 12-bit unsigned (0-4095) → signed 16-bit centered at 0
  sampleBuf[bufIdx].left = (int16_t)((rawL - 2048) << 4);
  sampleBuf[bufIdx].right = (int16_t)((rawR - 2048) << 4);
  bufIdx++;

  if (bufIdx >= FRAMES_PER_PACKET) {
    sendStreamPacket(sampleBuf, bufIdx);
    bufIdx = 0;
  }
}

static void initADC() {
  pinMode(STREAM_ADC_PIN_L, INPUT);
  pinMode(STREAM_ADC_PIN_R, INPUT);
  // analogRead resolution defaults to 12-bit on ESP32

  uint64_t periodUs = 1000000ULL / STREAM_SAMPLE_RATE;

  esp_timer_create_args_t timerArgs = {};
  timerArgs.callback = onAdcTimerCb;
  timerArgs.arg = NULL;
  timerArgs.dispatch_method = ESP_TIMER_TASK;
  timerArgs.name = "adc_stream";

  esp_timer_create(&timerArgs, &adcTimer);
  esp_timer_start_periodic(adcTimer, periodUs);

  Serial.printf("[StreamTx] ADC mode: L=GPIO%d R=GPIO%d @ %d Hz, %d fr/pkt\n",
                STREAM_ADC_PIN_L, STREAM_ADC_PIN_R, STREAM_SAMPLE_RATE,
                FRAMES_PER_PACKET);
}

#endif  // STREAM_SOURCE_ADC

// ---- Serial binary mode ----

#ifdef STREAM_SOURCE_SERIAL

// Binary protocol from PC:
//   [0xBB] [numFrames_lo] [numFrames_hi] [L0_lo L0_hi R0_lo R0_hi] ...
// PC sends at STREAM_SAMPLE_RATE; ESP32 forwards via ESP-NOW

static constexpr uint8_t SERIAL_SYNC_BYTE = 0xBB;

enum class SerialState : uint8_t { WAIT_SYNC, READ_HEADER, READ_DATA };
static SerialState serialState = SerialState::WAIT_SYNC;
static uint16_t serialFrameCount = 0;
static uint16_t serialBytesRead = 0;
static uint8_t serialDataBuf[FRAMES_PER_PACKET * sizeof(StereoFrame)];

static void resetSerialState() {
  serialState = SerialState::WAIT_SYNC;
  serialFrameCount = 0;
  serialBytesRead = 0;
}

void processSerialStream() {
  while (Serial.available() > 0) {
    uint8_t b = Serial.read();

    switch (serialState) {
      case SerialState::WAIT_SYNC:
        if (b == SERIAL_SYNC_BYTE) {
          serialState = SerialState::READ_HEADER;
          serialBytesRead = 0;
        }
        break;

      case SerialState::READ_HEADER: {
        static uint8_t headerBuf[2];
        headerBuf[serialBytesRead++] = b;
        if (serialBytesRead >= 2) {
          serialFrameCount =
              (uint16_t)headerBuf[0] | ((uint16_t)headerBuf[1] << 8);
          if (serialFrameCount == 0 ||
              serialFrameCount > FRAMES_PER_PACKET) {
            resetSerialState();
            break;
          }
          serialState = SerialState::READ_DATA;
          serialBytesRead = 0;
        }
      } break;

      case SerialState::READ_DATA: {
        serialDataBuf[serialBytesRead++] = b;
        uint16_t totalBytes = serialFrameCount * sizeof(StereoFrame);
        if (serialBytesRead >= totalBytes) {
          sendStreamPacket(
              reinterpret_cast<const StereoFrame *>(serialDataBuf),
              serialFrameCount);
          resetSerialState();
        }
      } break;
    }
  }
}

#endif  // STREAM_SOURCE_SERIAL

// ---- Public init ----

void init() {
#ifdef STREAM_SOURCE_ADC
  initADC();
#endif
#ifdef STREAM_SOURCE_SERIAL
  Serial.printf("[StreamTx] Serial binary mode @ %d Hz, max %d fr/pkt\n",
                STREAM_SAMPLE_RATE, FRAMES_PER_PACKET);
#endif
}

void drawScopeIfEnabled() {
#if defined(ENABLE_DISPLAY) && __has_include(<M5Unified.h>)
  static uint32_t lastDrawMs = 0;
  uint32_t now = millis();
  if (now - lastDrawMs < 100) return;  // 10fps max
  lastDrawMs = now;

  static bool inited = false;
  if (!inited) {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    inited = true;
  }

  const int w = M5.Display.width();
  const int barMaxW = w - 30;
  const int barH = 24;

  int16_t pL = peakL;
  int16_t pR = peakR;
  uint32_t pktCnt = pktCountESPNOW;
  peakL = pL > 200 ? pL - 200 : 0;
  peakR = pR > 200 ? pR - 200 : 0;

  constexpr float DB_MIN = -60.0f;
  auto dbToBar = [&](int16_t peak) -> int {
    if (peak <= 0) return 0;
    float db = 20.0f * log10f((float)peak / 32768.0f);
    float ratio = (db - DB_MIN) / (0.0f - DB_MIN);
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    return (int)(ratio * barMaxW);
  };
  auto dbVal = [](int16_t peak) -> float {
    if (peak <= 0) return -999.0f;
    return 20.0f * log10f((float)peak / 32768.0f);
  };

  int barL = dbToBar(pL);
  int barR = dbToBar(pR);
  float dbL = dbVal(pL);
  float dbR = dbVal(pR);

  M5.Display.setCursor(0, 0);
  M5.Display.printf("Stream %dHz    ", STREAM_SAMPLE_RATE);

  int y = 30;
  M5.Display.setCursor(0, y + 4);
  M5.Display.print("L");
  M5.Display.fillRect(20, y, barL, barH, TFT_GREEN);
  M5.Display.fillRect(20 + barL, y, barMaxW - barL, barH, TFT_DARKGREEN);

  y = 60;
  M5.Display.setCursor(0, y + 4);
  M5.Display.print("R");
  M5.Display.fillRect(20, y, barR, barH, TFT_CYAN);
  M5.Display.fillRect(20 + barR, y, barMaxW - barR, barH, 0x0208);

  y = 100;
  M5.Display.setCursor(0, y);
  M5.Display.printf("ESPpkt: %lu    ", pktCnt);

  y = 130;
  M5.Display.setCursor(0, y);
  if (dbL > -999.0f) {
    M5.Display.printf("L:%5.1fdB ", dbL);
  } else {
    M5.Display.print("L: -inf  ");
  }
  if (dbR > -999.0f) {
    M5.Display.printf("R:%5.1fdB ", dbR);
  } else {
    M5.Display.print("R: -inf  ");
  }
#endif
}

}  // namespace audioStreamSender
