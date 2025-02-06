#include "AccelmImpactDetector.h"
#include <M5Unified.h>

//-----------------------------------------------------------------------------
// ↓ M5Unified公式サンプルで使用している静的 / 定数定義の一部を抜粋。
//    (キャリブレーション関連の定数やカウンタなど)
//
// キャリブレーション操作の強度を指定する定数 (0=無効,
// 1～255で大きいほど強く補正)
static constexpr const uint8_t calib_value = 64;

// キャリブレーションを行う残り時間をカウントダウンするための変数
static uint8_t calib_countdown = 0;

//-----------------------------------------------------------------------------
// キャリブレーション開始・終了処理用の関数
// (本来は公式サンプル同様に細かく分割していますが、簡易化しています)
static void updateCalibration(uint32_t c, bool clear = false) {
  calib_countdown = c;

  // カウントダウンが0になったらキャリブレーション終了
  if (c == 0) {
    clear = true;
  }

  if (clear) {
    // カウントダウンがある → キャリブレーション開始
    if (c) {
      M5.Imu.setCalibration(calib_value, calib_value, calib_value);
      // ※ 実際の補正処理は M5.Imu.update() 内で随時行われる
    } else {
      // カウントダウンが0 → キャリブレーション停止 (磁気は継続)
      M5.Imu.setCalibration(0, 0, calib_value);

      // 全センサーのキャリブレーションを完全停止したい場合
      // M5.Imu.setCalibration(0, 0, 0);

      // 現時点のオフセット値をNVSに保存
      M5.Imu.saveOffsetToNVS();
    }
  }

  // キャリブレーションの状態をシリアル出力で確認したい場合
  if (c) {
    Serial.printf("Calibration countdown: %d\n", c);
  } else {
    Serial.println("Calibration finished.");
  }
}

//-----------------------------------------------------------------------------
// 実際にキャリブレーションを開始する際に呼び出す関数
static void startCalibration(void) {
  // 10秒間キャリブレーションする例
  updateCalibration(10, true);
}

//-----------------------------------------------------------------------------

namespace AccelmImpactDetector {

//-----------------------------------------------------------------------------
// IMU の初期化
//-----------------------------------------------------------------------------
bool initAccelm() {
  // M5 の設定構造体を取得 (必要に応じて設定を変更)
  auto cfg = M5.config();
  // 例: cfg.external_imu = true; // 外付けIMUを使う場合
  //     cfg.serial_baudrate = 115200; // シリアルボーレート変更など
  //     ... etc ...

  // M5 製品を初期化
  M5.begin(cfg);

  // どのIMUが認識されたかログに出す (公式サンプルの一部)
  const char* name;
  auto imu_type = M5.Imu.getType();
  switch (imu_type) {
    case m5::imu_none:
      name = "not found";
      break;
    case m5::imu_sh200q:
      name = "sh200q";
      break;
    case m5::imu_mpu6050:
      name = "mpu6050";
      break;
    case m5::imu_mpu6886:
      name = "mpu6886";
      break;
    case m5::imu_mpu9250:
      name = "mpu9250";
      break;
    case m5::imu_bmi270:
      name = "bmi270";
      break;
    default:
      name = "unknown";
      break;
  };
  Serial.printf("IMU type: %s\n", name);

  // IMUが見つからない場合は false を返す
  if (imu_type == m5::imu_none) {
    Serial.println("IMU not found or not enabled on this device!");
    return false;
  }

  // NVS から既存のキャリブレーション値を読み込む
  bool loaded = M5.Imu.loadOffsetFromNVS();
  if (!loaded) {
    Serial.println(
        "No saved calibration found. Start calibration automatically.");
    // キャリブレーションを自動開始
    startCalibration();
  } else {
    Serial.println("Loaded calibration values from NVS.");
  }

  Serial.println("IMU initialized successfully!");
  return true;
}

//-----------------------------------------------------------------------------
// メインループ等で呼び出し、IMUの値を取得＋出力する関数
//-----------------------------------------------------------------------------
void showAccelGraph() {
  // M5.update() でボタン操作やIMU内部の更新等を行う
  // (M5.Imu.update()もここで内部的に呼ばれるが、明示的に呼んでもOK)
  M5.update();

  // カウントダウン中なら1秒に1ずつ減らす (公式サンプルはもっと複雑に管理)
  static uint32_t prev_msec = 0;
  uint32_t now_msec = millis();
  if (now_msec - prev_msec >= 1000) {
    prev_msec = now_msec;
    if (calib_countdown > 0) {
      updateCalibration(calib_countdown - 1);
    }
  }

  // ボタンを押したらキャリブレーション再開 (例: BtnA)
  if (M5.BtnA.wasClicked() || M5.BtnPWR.wasClicked()) {
    Serial.println("Start calibration by button click.");
    startCalibration();
  }

  // IMUデータが更新されたかをチェック (M5.Imu.update()後の最新値を取得)
  //   戻り値 ≠ 0 : 新しいIMU値がある
  //   戻り値 = 0  : 変化がないか取得がまだ
  if (M5.Imu.update()) {
    // 最新の IMUデータをまとめて取得
    auto data = M5.Imu.getImuData();

    // 加速度 (X, Y, Z) [単位: G]
    float ax = data.accel.x;
    float ay = data.accel.y;
    float az = data.accel.z;
    az -= 1.0f;

    // z軸のみ
    // Serial.printf("[Accel] X=%.3f, Y=%.3f, Z=%.3f (G)\n", ax, ay, az);

    Serial.printf("%.3f\r\n", az);

    // // シリアルに出力
    // Serial.printf("[Accel] X=%.3f, Y=%.3f, Z=%.3f (G)\n", ax, ay, az);

    // ヘッダ行は最初の1回だけ表示する
    // static bool headerPrinted = false;
    // if (!headerPrinted) {
    //   Serial.println("x,y,z");  // ←ヘッダ
    //   headerPrinted = true;
    // }

    // Serial.printf("%.3f,%.3f,%.3f\n", ax, ay, az);
    // Serial.printf("[Accel] X=%.3f, Y=%.3f, Z=%.3f (G)\n", ax, ay, az);

    // ★★★ ここをVSCodeシリアルプロッター向けにスペース区切りで出力 ★★★
    // 例: "0.12 0.34 0.98"
    // (各軸を1行にまとめて出力することで3軸を同時にグラフ化可能)
    // Serial.printf("{\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f}\n", ax, ay, az);

    // ▼ 必要に応じてジャイロ・地磁気も出力可能
    //    float gx = data.gyro.x;
    //    float gy = data.gyro.y;
    //    float gz = data.gyro.z;
    //    Serial.printf("[Gyro ] X=%.3f, Y=%.3f, Z=%.3f (dps)\n", gx, gy, gz);
    //
    //    float mx = data.mag.x;
    //    float my = data.mag.y;
    //    float mz = data.mag.z;
    //    Serial.printf("[Mag  ] X=%.3f, Y=%.3f, Z=%.3f (uT)\n", mx, my, mz);
  }

  // 適宜処理負荷を軽減するためのウェイト
}

}  // namespace AccelmImpactDetector
