#include "AccelmImpactDetector.h"
#include "globals.h"

namespace AccelmImpactDetector {
BMI270 bmi270;
float accX = 0, accY = 0, accZ = 0;
bool isInitialized = false;

bool initAccelm() {
  // M5 Capsuleの正しいI2Cピン設定
  Wire.begin(1, 2);       // SDA=1, SCL=2
  Wire.setClock(400000);  // 400kHz

  // BMI270の初期化
  if (!bmi270.beginI2C()) {
    Serial.println("Failed to initialize BMI270!");
    return false;
  }

  // アクティブ化を待つ
  delay(100);

  // 加速度センサーの設定
  bmi2_sens_config config;
  config.type = BMI2_ACCEL;
  config.cfg.acc.odr = BMI2_ACC_ODR_100HZ;
  config.cfg.acc.range = BMI2_ACC_RANGE_2G;
  config.cfg.acc.bwp = BMI2_ACC_NORMAL_AVG4;
  config.cfg.acc.filter_perf = BMI2_PERF_OPT_MODE;

  if (bmi270.setConfig(config) != BMI2_OK) {
    Serial.println("Failed to configure BMI270!");
    return false;
  }

  // センサーが安定するまで待つ
  delay(50);

  isInitialized = true;
  return true;
}

void showAccelGraph() {
  if (!isInitialized) return;

  bmi270.getSensorData();

  accX = bmi270.data.accelX;
  accY = bmi270.data.accelY;
  accZ = bmi270.data.accelZ;
  Serial.printf("X: %.2f, Y: %.2f, Z: %.2f\n", accX, accY, accZ);
}
}  // namespace AccelmImpactDetector