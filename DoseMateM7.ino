#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <RPC.h>
#include <SPI.h>
#include "HX711.h"
#include <string>

#define TFT_CS     7   // Chip select
#define TFT_DC      5   // Data/command
#define TFT_RST     6   // Reset (or set to -1 if tied to Arduino reset)

// Create ST7789 object (240x240 display)
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);

// Colors
#define BLACK 0x0000
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

// Load cell pins
const int LOADCELL_DOUT_PIN = 22;
const int LOADCELL_SCK_PIN = 23;

HX711 scale;

// Rotary encoder pins
const int PIN_CLK = 2;
const int PIN_DT = 3;
const int PIN_SW = 4;

// Encoder variables
int position = 0;
int lastPosition = 0;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // Prevents Button Spamming

// Screen state
String process = "Home";

// Menu options
const char* dispensers[5] = {"Slot 1", "Slot 2", "Slot 3", "Slot 4", "Exit"};
int dispenser = 0;

const char* settings[6] = {"Interval", "Amount", "Calibrate", "Activate", "Clear", "Exit"};
int setting = 0;
bool buttonClicked = false;

// Dispenser settings: [interval hours, amount pills per dose, calibration weight, active]
int value[4][4] = {
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0},
  {1, 1, 0, 0}
};

void setup() {
  Serial.begin(9600);
  RPC.begin();
  
  // Setup encoder
  pinMode(PIN_SW, INPUT_PULLUP);
  pinMode(PIN_DT, INPUT_PULLUP);
  pinMode(PIN_CLK, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_DT), checkEncoder, CHANGE);

  tft.init(240, 240);
  tft.setRotation(2);
  tft.fillScreen(BLACK);
  
  render_Startup();

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
            
  scale.set_scale(810);
  scale.tare();
  
  delay(2000);
  render_Home();
}

void loop() {
  handleButton();
  
  if (position != lastPosition) {
    handleEncoder();
    lastPosition = position;
    refreshDisplay();
  }
}

void handleButton() {
  int reading = digitalRead(PIN_SW);
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && lastButtonState == HIGH) {
      processButtonClick();
    }
  }

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  lastButtonState = reading;
}

void processButtonClick() {
  if (process == "Home") {
    process = "Menu";
    render_Menu();
  } 
  else if (process == "Menu") {
    if (dispenser == 5) {
      process = "Home";
      dispenser = 0;
      render_Home();
    } else {
      process = "Setting";
      setting = 0;
      render_Setting();
    }
  } 
  else if (process == "Setting") {
    handleSettingClick();
  } 
}

void handleSettingClick() {
  if (setting == 2) {
    calibrateLoadCell();
  } 
  else if (setting == 3) {
    value[dispenser][3] = !value[dispenser][3];
    render_Setting();
  } 
  else if (setting == 4) {
    value[dispenser][0] = 1;
    value[dispenser][1] = 1;
    value[dispenser][2] = 0;
    value[dispenser][3] = 0;
    render_Setting();
  }
  else if (setting == 5) {
    process = "Menu";
    setting = 0;
    render_Menu();
  } 
  else {
    buttonClicked = !buttonClicked;
    render_Setting();
  }
}

void handleEncoder() {
  int change = position - lastPosition;
  
  if (process == "Menu") {
    dispenser = (dispenser + change + 5) % 5;
  } 
  else if (process == "Setting") {
    if (buttonClicked) {
      if (setting == 0) {
        value[dispenser][0] = constrain(value[dispenser][0] + change, 1, 24);
      } else if (setting == 1) {
        value[dispenser][1] = constrain(value[dispenser][1] + change, 1, 10);
      }
    } else {
      setting = (setting + change + 6) % 6;
    }
  }
}

void refreshDisplay() {
  if (process == "Home") {
    render_Home();
  } else if (process == "Menu") {
    render_Menu();
  } else if (process == "Setting") {
    render_Setting();
  }
}

void render_Startup() {
  tft.setCursor(20, 40);
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.println("Getting");
  tft.setCursor(20, 70);
  tft.println("Things");
  tft.setCursor(20, 100);
  tft.println("Ready");
  tft.setCursor(30, 150);
  tft.setTextSize(2);
  tft.println("Please Hold...");
}

void render_Home() {
  tft.fillScreen(BLACK);
  tft.setCursor(30, 60);
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.println("Welcome to");
  tft.setCursor(30, 100);
  tft.println("DoseMate");
  tft.setCursor(30, 180);
  tft.setTextSize(2);
  tft.println("Click to Enter");
}

void render_Menu() {
  tft.fillScreen(BLACK);
  tft.setCursor(20, 40);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.println("Select Slot:");
  
  tft.setCursor(60, 100);
  tft.setTextSize(3);
  tft.println(dispensers[dispenser]);
  
  tft.setCursor(10, 200);
  tft.setTextSize(2);
  tft.println("Click to Select");
}

void render_Setting() {
  tft.fillScreen(BLACK);
  tft.setCursor(10, 20);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.print(dispensers[dispenser]);
  tft.println(" Settings");
  
  tft.setCursor(10, 70);
  tft.setTextSize(2);
  tft.print(settings[setting]);
  tft.println(":");
  
  tft.setCursor(10, 110);
  tft.setTextSize(3);
  
  if (buttonClicked && (setting == 0 || setting == 1)) {
    tft.setTextColor(YELLOW);
    tft.print("> ");
  }
  
  if (setting == 0) {
    tft.print(value[dispenser][0]);
    tft.println(" hr");
  } else if (setting == 1) {
    tft.print(value[dispenser][1]);
    tft.println(" pills");
  } else if (setting == 2) {
    float mass = (value[dispenser][2])*0.01;
    tft.print(mass);
    tft.println(" g");
  } else if (setting == 3) {
    tft.println(value[dispenser][3] ? "ON" : "OFF");
  } else if (setting == 4) {
    tft.setTextColor(RED);
    tft.setTextSize(2);
    tft.println("Clear Data?");
  } else if (setting == 5) {
    tft.println("Exit");
  }
}

void render_Alert() {
  tft.fillScreen(BLACK);
  tft.setCursor(30, 80);
  tft.setTextColor(GREEN);
  tft.setTextSize(4);
  tft.println("PILLS");
  tft.setCursor(30, 130);
  tft.println("READY");
}

void render_Warning() {
  tft.fillScreen(BLACK);
  tft.setCursor(30, 80);
  tft.setTextColor(RED);
  tft.setTextSize(4);
  tft.println("Dispensing");
  tft.setCursor(40, 130);
  tft.println("Error");
}

void calibrateLoadCell() {
  tft.fillScreen(BLACK);
  tft.setCursor(10, 40);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.println("Place single pill");
  tft.setCursor(10, 80);
  tft.println("Weighing...");
  
  delay(10000);

  float reading = scale.get_units(20);
  
  value[dispenser][2] = int(reading * 100);
  
  tft.fillScreen(BLACK);
  tft.setCursor(10, 80);
  tft.setTextColor(GREEN);
  tft.setTextSize(2);
  tft.println("Calibration");
  tft.setCursor(10, 110);
  tft.println("Complete!");
  
  delay(2000);
  tft.fillScreen(BLACK);
  render_Setting();
}

void checkEncoder() {
  int a = digitalRead(PIN_CLK);
  int b = digitalRead(PIN_DT);
 
  static int lastA = LOW;
  static int lastB = LOW;
 
  if (a != lastA || b != lastB) {
    if (lastA == LOW && a == HIGH) {
      if (b == LOW) {
        position++;
      } else {
        position--;
      }
    }
   
    lastA = a;
    lastB = b;
  }
}

void sendPackage() {
  std::string package = "";
  for (int i = 0; i < 4; i++) {
    for (int x = 0; x < 4; x++) {
      package += std::to_string(value[i][x]);
      if (x < 3) {
        package += ",";
      }
    }
    if (i < 3) {
      package += ",";
    }
  }
  RPC.call("receivePackage", package);
}
