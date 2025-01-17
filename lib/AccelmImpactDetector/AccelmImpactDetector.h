// AccelmImpactDetector.h
#ifndef ACCELM_IMPACT_DETECTOR_H
#define ACCELM_IMPACT_DETECTOR_H

#include <SparkFun_BMI270_Arduino_Library.h>

namespace AccelmImpactDetector {
    extern BMI270 bmi270;
    extern float accX, accY, accZ;
    extern bool isInitialized;

    bool initAccelm();
    void showAccelGraph();
}

#endif