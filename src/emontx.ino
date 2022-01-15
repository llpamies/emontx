#include <Arduino.h>
#include <avr/wdt.h>

#include "MyWire.h"
#include "emonLibCM.h"

// This EmonTX ID.
constexpr byte emontx_id = 2;

// Status LED pin.
constexpr byte kArduinoLED = 13;
constexpr byte kEmonTxLED = 9;

// Buss address (for a 3-device configuration).
constexpr byte kWireAddress[] = {8, 9, 10};

// EmonTx Shield calibration values.
constexpr float v_cal = 148.44;
constexpr byte v_input = 7;
constexpr byte ct_count = 4;
constexpr float ct_cal[] = {61, 61, 61, 61};
constexpr float ct_lead[] = {1.0, 1.0, 1.0, 1.0};
constexpr byte ct_inputs[] = {6, 5, 4, 1};

// Voltage to use for calculating assumed apparent power if a.c input is absent.
constexpr float assumed_vrms = 124.0;

// Reading period: Every that many seconds a new reading is produced.
constexpr float datalog_period_secs = 5;

// The last reading to be sent over the wire to EmonESP.
char last_reading[256];

// Flag to indicate a new reading is available.
bool reading_available;

void onRequestEvent() {
  if (reading_available) {
    MyWire.write(last_reading, strlen(last_reading));
    reading_available = false;
  }
}

void setup() {
  // Watch dog to eight seconds.
  wdt_enable(WDTO_8S);

  // Initialize LED.
  pinMode(kArduinoLED, OUTPUT);
  pinMode(kEmonTxLED, OUTPUT);

  // Init flag.
  reading_available = false;

  EmonLibCM_ADCCal(5.0);  // Reference voltage (for Arduino).
  EmonLibCM_setAssumedVrms(assumed_vrms);
  EmonLibCM_SetADC_VChannel(v_input, v_cal);
  for (byte i = 0; i < ct_count; i++){
    EmonLibCM_SetADC_IChannel(ct_inputs[i], ct_cal[i], ct_lead[i]);
  }

  // Mains frequency 60Hz.
  EmonLibCM_cycles_per_second(60);
  EmonLibCM_datalog_period(datalog_period_secs);
  EmonLibCM_min_startup_cycles(10);
  EmonLibCM_Init();

  // Serial.
  Serial.begin(115200);

  // Wire.
  MyWire.begin(kWireAddress[emontx_id]);
  MyWire.onRequest(onRequestEvent);
}

void loop() {
  // Wait and reset watch dog.
  delay(1000);
  wdt_reset();

  // Handle the pair of LED that blink.
  static bool high = false;
  digitalWrite(kArduinoLED, high ? HIGH : LOW);
  digitalWrite(kEmonTxLED, high ? HIGH : LOW);
  high ^= true;

  // Load the latest reading from EmonLibCM.
  if (EmonLibCM_Ready()) {
    char *buf = last_reading;
    byte id = emontx_id * ct_count;
    for (byte i = 0; i < ct_count; i++) {
      buf += sprintf(buf, "C%d:%.2f,", id, EmonLibCM_getIrms(i));
      buf += sprintf(buf, "P%d:%d,", id, EmonLibCM_getRealPower(i));
      buf += sprintf(buf, "E%d:%ld,", id, EmonLibCM_getWattHour(i));
      id++;
    }
    buf += sprintf(buf, "V%d:%.2f", emontx_id, EmonLibCM_getVrms());
    Serial.println(last_reading);
    reading_available = true;
  }
}
