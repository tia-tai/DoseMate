#include "mbed.h"
#include <mbed_mktime.h>
#include "RPC.h"
#include <Stepper.h> 
#include "HX711.h"

// Dispenser settings: [interval hours, amount pills per dose, calibration weight, active]
int oldValue[4][4] = {
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0}
};
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

// Setting up Stepper Motors
int stepsPerRevolution = 64;
Stepper stepper1(stepsPerRevolution, 8, 9, 10, 11);
Stepper stepper2(stepsPerRevolution, 8, 9, 10, 11);
Stepper stepper3(stepsPerRevolution, 8, 9, 10, 11);
Stepper stepper4(stepsPerRevolution, 8, 9, 10, 11);

// Load cell pins
const int LOADCELL_DOUT_PIN = 22;
const int LOADCELL_SCK_PIN = 23;

HX711 scale;

// Speaker pins
#define SpeakerPin 24

void setup() {

  // Setup load cell
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
            
  scale.set_scale(810);
  scale.tare();

  RPC.begin();
  RPC.bind("receivePackage", receivePackage);
  RTCset();
}

void loop() {
  currentTime = getCurrenttime();
  RPC.print("M4 System Clock: ");
  RPC.println(currentTime);
  for (int a = 0; a < 4; a++) {
    if (a == 0) {
      nextTime1 = checkTime(nextTime1, value[0][0], a);
    } else if (a == 1) {
      nextTime2 = checkTime(nextTime2, value[1][0], a);
    } else if (a == 2) {
      nextTime3 = checkTime(nextTime3, value[2][0], a);
    } else if (a == 3) {
      nextTime4 = checkTime(nextTime4, value[3][0], a);
    }
  }
  delay(1000);
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
  if (time <= currentTime && time != 0UL) {
    // do the dispensing
    int dose = value[slot][0];
    float mass = (value[slot][2] * 0.01) * dose;
    scale.tare();
    for (int i = 0; i < dose; i++){
      if (i == 0) {
        stepper1.setSpeed(10);
        stepper1.step(64);
        stepper1.setSpeed(0);
      } else if (i == 1) {
        stepper2.setSpeed(10);
        stepper2.step(64);
        stepper2.setSpeed(0);
      } else if (i == 2) {
        stepper3.setSpeed(10);
        stepper3.step(64);
        stepper3.setSpeed(0);
      } else {
        stepper4.setSpeed(10);
        stepper4.step(64);
        stepper4.setSpeed(0);
      }
    }
    float reading = scale.get_units(20);
    alarm();
    
    if (reading < mass + 0.2 && reading > mass - 0.2) {
      RPC.call("render_Alert");
    } else {
      RPC.call("render_Warning");
    }
    if (value[slot][3] == 1) {
      return nextInterval(interval);
    } else {
      return 0UL;
    }
  } else {
    return time;
  }
}

void playTone(int pin, int frequencyHz, int durationMs) {
    long halfPeriod = 1000000L / (frequencyHz * 2);
    long cycles = (long)frequencyHz * durationMs / 1000;
    for (long i = 0; i < cycles; i++) {
        digitalWrite(pin, HIGH);
        delayMicroseconds(halfPeriod);
        digitalWrite(pin, LOW);
        delayMicroseconds(halfPeriod);
    }
}

void alarm() {
    int freqs[] = {392, 131, 392, 131, 392, 131};
    for (int f : freqs) {
        playTone(SpeakerPin, f, 2000);
    }
}

void receivePackage(String package) {
  String packageItems[16];
  int itemCount = 0;

  while (package.length() > 0) {
    int commaIndex = package.indexOf(',');
    if (commaIndex == -1) {
      packageItems[itemCount++] = package;
      break;
    }
    packageItems[itemCount++] = package.substring(0, commaIndex);
    package = package.substring(commaIndex + 1);
  }

  for (int i = 0; i < 4; i++) {
    for (int x = 0; x < 4; x++) {
      int itemIndex = i*4+x; // 0*4 = 0 + 1 = 1 (2nd index) test for 2nd slot: 1*4 = 4 + 1 = 5 (2nd slot, 2nd item)
      value[i][x] = packageItems[itemIndex].toInt();
    }
  }
  updateChecks();
}

void updateChecks() {
  for (int i = 0; i < 4; i++) {
    for (int x = 0; x < 4; x++) {
      if (value[i][x] != oldValue[i][x]) {
        if (x == 2) {
          if (i == 0) {
            nextTime1 = nextInterval(value[i][x]);
          } else if (i == 1) {
            nextTime2 = nextInterval(value[i][x]);
          } else if (i == 2) {
            nextTime3 = nextInterval(value[i][x]);
          } else if (i == 3) {
            nextTime4 = nextInterval(value[i][x]);
          }
        }
        if (x == 3) {
          if (i == 0) {
            nextTime1 = 0UL;
          } else if (i == 1) {
            nextTime2 = 0UL;
          } else if (i == 2) {
            nextTime3 = 0UL;
          } else if (i == 3) {
            nextTime4 = 0UL;
          }
        }
      }
      oldValue[i][x] = value[i][x];
    }
  }
}
