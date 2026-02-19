#include "mbed.h"
#include <mbed_mktime.h>
#include "RPC.h"

// Dispenser settings: [interval hours, amount pills per dose, calibration weight, active]
int value[4][4] = {
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0}
};
unsigned long nextTime1 = 0UL;
unsigned long nextTime2 = 0UL;
unsigned long nextTime3 = 0UL;
unsigned long nextTime4 = 0UL;
unsigned long currentTime = 0UL;

// Load cell calibration
long zeroOffset = 0;
float calibrationFactor = 1.0;

void setup() {
  Serial.begin(9600);
  if (RPC.begin()) {
    RPC.println("M4: Reading the RTC.");
    RTCset(); //sets the RTC to start from a specific time & date
  }
}

void loop() {
  curentTime = getCurrenttime();
  for (int a = 0; a < 4; a++) {
    if (a == 0) {
      checkTime(nextTime1, value[0][0], a);
    } else if (a == 1) {
      checkTime(nextTime2, value[1][0], a);
    } else if (a == 2) {
      checkTime(nextTime3, value[2][0], a);
    } else if (a == 4) {
      checkTime(nextTime4, value[3][0], a);
    }
  }
}


unsigned long getCurrenttime() {
  unsigned long currentTime = (unsigned long)time(NULL);
  return currentTime;
}

void RTCset()  // Set cpu RTC
{
  tm t;
  t.tm_sec = (0);            // 0-59
  t.tm_min = (58);           // 0-59
  t.tm_hour = (11);          // 0-23
  t.tm_mday = (1);           // 1-31
  t.tm_mon = (9);            // 0-11  "0" = Jan, -1
  t.tm_year = ((24) + 100);  // year since 1900,  current year + 100 + 1900 = correct year
  set_time(mktime(&t));      // set RTC clock
}

unsigned long nextInterval(int interval) {
  unsigned long nextTime = currentTime + (interval * 3600);
  return nextTime;
}

unsigned long checkTime(unsigned long time, int interval, int slot) {
  if (time <= currentTime) {
    Serial.println("Time is up");
    return nextInterval(interval);
  } else {
    return time;
  }
}