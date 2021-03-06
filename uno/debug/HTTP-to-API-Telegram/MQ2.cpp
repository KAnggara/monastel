// MQ2.cpp
#include "Arduino.h"
#include "MQ2.h"

MQ2::MQ2(int pin) {
  _pin = pin;
  Ro = -1.0;
}

void MQ2::begin() {
  Ro = MQCalibration();
  Serial.print(F("Calibration is done...\nRo="));
  Serial.print(Ro);
  Serial.println(F("kohm"));
}

void MQ2::close() {
  Ro = -1.0;
  values[0] = 0.0;
  values[1] = 0.0;
  values[2] = 0.0;
}

bool MQ2::checkCalibration() {
  if (Ro < 0.0) {
    Serial.println("Device not calibrated, call MQ2::begin before reading any value.");
    return false;
  }
  return true;
}

float* MQ2::read(bool print) {
  if (!checkCalibration()) return NULL;

  values[0] = MQGetPercentage(LPGCurve);
  values[1] = MQGetPercentage(COCurve);
  values[2] = MQGetPercentage(SmokeCurve);
  
  if (print) {
    Serial.print(F("LPG:"));
    Serial.print(values[0], 2);
    Serial.print(F("ppm | CO:"));
    Serial.print(values[1], 2);
    Serial.print(F("ppm | SMOKE:"));
    Serial.print(values[2], 2);
    Serial.print(F("ppm | RS : "));
    Serial.print(String(MQRead()));
    Serial.print(F(" | RO: "));
    Serial.print(String(Ro));
    Serial.print(F(" | RAW :"));
    Serial.println(String(analogRead(_pin)));
  }
  return values;
}

float MQ2::MQCalibration() {
  Serial.println(F("Calibrating MQ-2..."));
  float val = 0.0;
  for (int i = 0; i < CALIBARAION_SAMPLE_TIMES; i++) {      //take multiple samples
    val += MQResistanceCalculation(analogRead(_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val = val / CALIBARAION_SAMPLE_TIMES;                 //calculate the average value
  val = val / RO_CLEAN_AIR_FACTOR;                      //divided by RO_CLEAN_AIR_FACTOR yields the RO according to the chart in the datasheet
  return val;
}

float MQ2::MQResistanceCalculation(int raw_adc) {
  return (((float)RL_VALUE * (1023 - raw_adc) / raw_adc));
}

float MQ2::MQRead() {
  float rs = 0.0;
  for (int i = 0; i < READ_SAMPLE_TIMES; i++) {
    rs += MQResistanceCalculation(analogRead(_pin));
    delay(READ_SAMPLE_INTERVAL);
  }
  return rs / ((float) READ_SAMPLE_TIMES);  // return the average
}

float MQ2::readLPG() {
  if (!checkCalibration()) return 0.0;

  if (millis() < (lastReadTime + READ_DELAY) && values[0] > 0)
    return values[0];
  else
    return (values[0] = MQGetPercentage(LPGCurve));
}

float MQ2::readCO() {
  if (!checkCalibration()) return 0.0;
  if (millis() < (lastReadTime + READ_DELAY) && values[1] > 0)
    return values[1];
  else
    return (values[1] = MQGetPercentage(COCurve));
}

float MQ2::readSmoke() {
  if (!checkCalibration()) return 0.0;
  if (millis() < (lastReadTime + READ_DELAY) && values[2] > 0)
    return values[2];
  else
    return (values[2] = MQGetPercentage(SmokeCurve));
}

float MQ2::MQGetPercentage(float *pcurve) {
  float rs_ro_ratio = MQRead() / Ro;
  return (200 + pow(10.0, (((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0])));
}
