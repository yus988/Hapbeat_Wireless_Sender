// AccelTrigger.h
// M5StickC PLUS2用 加速度トリガー機能
#ifndef ACCEL_TRIGGER_H
#define ACCEL_TRIGGER_H

#include <stdint.h>

#ifdef ENABLE_ACCEL_TRIGGER

namespace AccelTrigger {

// 初期化（M5.begin()後に呼び出すこと）
void init();

// メインループ処理（毎ループ呼び出し）
void loop();

// トリガーカウントを取得
uint32_t getTriggerCount();

// トリガーカウントをリセット
void resetTriggerCount();

}  // namespace AccelTrigger

#endif  // ENABLE_ACCEL_TRIGGER

#endif  // ACCEL_TRIGGER_H

