#include "mbed.h"
#include <mbed_mktime.h>
#include "RPC.h"
#include <Stepper.h> 
#include "HX711.h"
#include <string>
#include <vector>

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
int stepsPerRevolution = 100;
Stepper stepper1(stepsPerRevolution, 25, 26, 27, 28);
Stepper stepper2(stepsPerRevolution, 29, 30, 31, 32);
Stepper stepper3(stepsPerRevolution, 33, 34, 35, 36);
Stepper stepper4(stepsPerRevolution, 37, 38, 39, 40);

// Load cell pins
const int LOADCELL_DOUT_PIN = 22;
const int LOADCELL_SCK_PIN = 23;

HX711 scale;

// Speaker pins
#define SpeakerPin 24

void setup() {

  // Setup load cell
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  stepper1.setSpeed(200);
  stepper2.setSpeed(200);
  stepper3.setSpeed(200);
  stepper4.setSpeed(200);
            
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
  RPC.println(nextTime1);
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
    int pillsOut = 0;
    float mass = value[slot][2] * 0.01;
    float totalMass = mass * dose;
    scale.tare();
    while (pillsOut < dose) {
      if (slot == 0) {
        stepper1.step(2000);
      } else if (slot == 1) {
        stepper2.step(2000);
      } else if (slot == 2) {
        stepper3.step(2000);
      } else {
        stepper4.step(2000);
      }
      float reading = scale.get_units(20);
      pillsOut = int(reading/mass);
    }

    float reading = scale.get_units(20);
    alarm();
    
    if (reading < totalMass + 0.2 && reading > totalMass - 0.2) {
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
    if (value[slot][3] == 1) {
      return time;
    } else {
      return 0UL;
    }
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

void receivePackage(std::string package) {
  RPC.println(package.c_str());
  std::vector<int> values;
  size_t start = 0;

  while (true) {
    size_t comma = package.find(',', start);
    if (comma == std::string::npos) {
      values.push_back(std::stoi(package.substr(start)));
      break;
    }
    values.push_back(std::stoi(package.substr(start, comma - start)));
    start = comma + 1;
  }

  if (values.size() != 16) return;

  for (int i = 0; i < 4; i++) {
    for (int x = 0; x < 4; x++) {
      value[i][x] = values[i * 4 + x];
    }
  }

  updateChecks();
}

void updateChecks() {
  for (int i = 0; i < 4; i++) {
    for (int x = 0; x < 4; x++) {
      if (value[i][x] != oldValue[i][x]) {
        if (x == 3 && value[i][x] == 1) {
          if (i == 0) {
            nextTime1 = nextInterval(value[i][0]);
          } else if (i == 1) {
            nextTime2 = nextInterval(value[i][0]);
          } else if (i == 2) {
            nextTime3 = nextInterval(value[i][0]);
          } else if (i == 3) {
            nextTime4 = nextInterval(value[i][0]);
          }
        } else if (x == 3 && value[i][x] == 0) {
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
