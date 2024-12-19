#include <M5Unified.h>

int screen_width, screen_height, button_width, button_height;
int buttonA_x, buttonA_y, buttonB_x, buttonB_y, buttonC_x, buttonC_y;

bool buttonA_pressed = false, buttonB_pressed = false, buttonC_pressed = false;

// 現時点では M5Stack basic 用
void drawButtonState(int buttonIndex, int state, int color) {
  // M5Stack Basic ボタンの幅・高さ
  constexpr int buttonWidth = 50;    // ボタンの横幅
  constexpr int buttonHeight = 30;   // バーの高さ
  constexpr int screenWidth = 320;   // M5Stack Basic の画面幅
  constexpr int screenHeight = 240;  // M5Stack Basic の画面高さ
  constexpr int buttonSpacing = 68;  // ボタン間の間隔

  // ボタンBを画面中央に配置
  int centerX = screenWidth / 2;
  int yPos = screenHeight - buttonHeight;  // バーを画面下部に配置

  // 各ボタンの X 座標を計算
  int xPos;
  if (buttonIndex == 0) {
    // ボタンA (左側)
    xPos = centerX - buttonWidth - buttonSpacing;
  } else if (buttonIndex == 1) {
    // ボタンB (中央)
    xPos = centerX - buttonWidth / 2;
  } else if (buttonIndex == 2) {
    // ボタンC (右側)
    xPos = centerX + buttonSpacing;
  } else {
    return;  // 無効なボタンインデックス
  }

  // 描画
  M5.Display.fillRect(xPos, yPos, buttonWidth, buttonHeight, color);
}

#ifdef M5STACK_BASIC
const char *M5ButtonNotify(const char *stat) {
  M5.update();

  // ボタン配列とラベル
  auto buttons = {&M5.BtnA, &M5.BtnB, &M5.BtnC};
  const char *labels[] = {"BtnA", "BtnB", "BtnC"};

  // 各ボタンの前回の状態を保持する配列
  static bool previousStates[3] = {false, false, false};

  int index = 0;  // ボタンのインデックス

  for (auto button : buttons) {
    // ボタンの現在の状態を確認
    bool isPressed = button->isPressed();

    // 状態が変化した場合のみ画面を更新
    if (isPressed != previousStates[index]) {
      // 色の設定: 押下中は黄色、それ以外は緑
      int color = isPressed ? TFT_YELLOW : TFT_GREEN;

      // 状態に応じて stat を設定
      if (isPressed) {
        stat = labels[index];
      }

      // ボタン状態の描画
      drawButtonState(index, isPressed ? 3 : 0, color);

      // 前回の状態を更新
      previousStates[index] = isPressed;
    }

    index++;
  }

  return stat;
}

#endif

#ifdef M5STACK_CORE_S3

void drawButtons() {
  M5.Display.fillRect(buttonA_x, buttonA_y, button_width, button_height,
                      TFT_WHITE);
  M5.Display.fillRect(buttonB_x, buttonB_y, button_width, button_height,
                      TFT_WHITE);
  M5.Display.fillRect(buttonC_x, buttonC_y, button_width, button_height,
                      TFT_WHITE);

  // 境目に縦棒を描画
  M5.Display.fillRect(buttonB_x - 1, buttonB_y, 2, button_height, TFT_BLACK);
  M5.Display.fillRect(buttonC_x - 1, buttonC_y, 2, button_height, TFT_BLACK);

  M5.Display.setTextColor(TFT_BLACK);
  M5.Display.setTextDatum(CC_DATUM);

  M5.Display.drawString("Button A", buttonA_x + button_width / 2,
                        buttonA_y + button_height / 2);
  M5.Display.drawString("Button B", buttonB_x + button_width / 2,
                        buttonB_y + button_height / 2);
  M5.Display.drawString("Button C", buttonC_x + button_width / 2,
                        buttonC_y + button_height / 2);
}

const char *M5ButtonNotify(const char *stat) {
  M5.update();
  bool screenUpdated = false;

  if (M5.Touch.getCount() > 0) {
    auto touch = M5.Touch.getDetail();
    int touch_x = touch.x;
    int touch_y = touch.y;

    // ボタンAのタッチエリア判定
    if (touch_x > buttonA_x && touch_x < buttonA_x + button_width &&
        touch_y > buttonA_y && touch_y < buttonA_y + button_height) {
      if (!buttonA_pressed) {
        stat = "BtnA";
        screenUpdated = true;
      }
      buttonA_pressed = true;
      M5.Display.fillRect(buttonA_x, buttonA_y, button_width, button_height,
                          TFT_RED);
    } else {
      if (buttonA_pressed) {
        buttonA_pressed = false;
        screenUpdated = true;
      }
    }

    // ボタンBのタッチエリア判定
    if (touch_x > buttonB_x && touch_x < buttonB_x + button_width &&
        touch_y > buttonB_y && touch_y < buttonB_y + button_height) {
      if (!buttonB_pressed) {
        stat = "BtnB";
        screenUpdated = true;
      }
      buttonB_pressed = true;
      M5.Display.fillRect(buttonB_x, buttonB_y, button_width, button_height,
                          TFT_GREEN);
    } else {
      if (buttonB_pressed) {
        buttonB_pressed = false;
        screenUpdated = true;
      }
    }

    // ボタンCのタッチエリア判定
    if (touch_x > buttonC_x && touch_x < buttonC_x + button_width &&
        touch_y > buttonC_y && touch_y < buttonC_y + button_height) {
      if (!buttonC_pressed) {
        stat = "BtnC";
        screenUpdated = true;
      }
      buttonC_pressed = true;
      M5.Display.fillRect(buttonC_x, buttonC_y, button_width, button_height,
                          TFT_BLUE);
    } else {
      if (buttonC_pressed) {
        buttonC_pressed = false;
        screenUpdated = true;
      }
    }
  } else {
    // タッチされていないときに表示を元に戻す
    if (buttonA_pressed || buttonB_pressed || buttonC_pressed) {
      buttonA_pressed = false;
      buttonB_pressed = false;
      buttonC_pressed = false;
      screenUpdated = true;
    }
  }

  if (screenUpdated) {
    drawButtons();
  }

  return stat;
}

#endif

void initM5UImanager(void) {
  auto cfg = M5.config();
  cfg.external_spk = false;  // 内蔵スピーカーを無効にする例（必要に応じて設定）
  M5.begin(cfg);
  M5.Display.setTextSize(4);

#ifdef M5STACK_BASIC
  drawButtonState(0, 0, TFT_GREEN);  // ボタン A
  drawButtonState(1, 0, TFT_GREEN);  // ボタン B
  drawButtonState(2, 0, TFT_GREEN);  // ボタン C
#endif

#ifdef M5STACK_CORE_S3
  M5.Display.setTextSize(1);

  screen_width = M5.Display.width();
  screen_height = M5.Display.height();

  button_width = screen_width / 3;
  button_height = screen_height;

  buttonA_x = 0;
  buttonA_y = 0;

  buttonB_x = button_width;
  buttonB_y = 0;

  buttonC_x = 2 * button_width;
  buttonC_y = 0;

  drawButtons();
#endif
}
