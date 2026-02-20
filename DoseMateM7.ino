#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <RPC.h>
#include <SPI.h> 

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
#define HX711_DT A2
#define HX711_SCK A3

// Rotary encoder pins
const int PIN_CLK = 2;
const int PIN_DT = 3;
const int PIN_SW = 4;

// Encoder variables
volatile int encoderPos = 0;
int lastEncoderPos = 0;
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
  
  tft.init(240, 240);
  tft.setRotation(2);
  tft.fillScreen(BLACK);
  
  render_Startup();
  
  // Setup encoder
  pinMode(PIN_CLK, INPUT_PULLUP);
  pinMode(PIN_DT, INPUT_PULLUP);
  pinMode(PIN_SW, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(PIN_CLK), checkEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_DT), checkEncoder, CHANGE);
  
  // Setup load cell
  pinMode(HX711_SCK, OUTPUT);

  long reading = readHX711();
  zeroOffset = reading;
  
  delay(2000);
  render_Home();
}

void loop() {
  handleButton();
  
  if (encoderPos != lastEncoderPos) {
    handleEncoder();
    lastEncoderPos = encoderPos;
    
    refreshDisplay();
  }
}

void handleButton() {
  int reading = digitalRead(PIN_SW);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && lastButtonState == HIGH) {
      processButtonClick();
    }
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
  int change = encoderPos - lastEncoderPos;
  
  if (process == "Menu") {
    dispenser = (dispenser + change + 6) % 6;
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
  tft.setCursor(30, 60);
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.println("Welcome to");
  tft.setCursor(30, 100);
  tft.println("DoseMate");
  tft.setCursor(10, 180);
  tft.setTextSize(2);
  tft.println("Click to Enter");
}

void render_Menu() {
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
    tft.print(value[dispenser][2]);
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

void render_NoPill() {
  tft.fillScreen(BLACK);
  tft.setCursor(50, 80);
  tft.setTextColor(RED);
  tft.setTextSize(4);
  tft.println("NEED");
  tft.setCursor(15, 130);
  tft.println("RELOAD");
}

void calibrateLoadCell() {
  tft.fillScreen(BLACK);
  tft.setCursor(10, 40);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.println("Place single pill");
  tft.setCursor(10, 80);
  tft.println("Weighing...");
  
  delay(2000);
  
  long reading = readHX711();
  
  value[dispenser][2] = (int) ((reading - zeroOffset) / 100) * 10;
  
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
  static int lastCLK = HIGH;
  int currentCLK = digitalRead(PIN_CLK);
  
  if (currentCLK != lastCLK && currentCLK == LOW) {
    if (digitalRead(PIN_DT) == HIGH) {
      encoderPos++;
    } else {
      encoderPos--;
    }
  }
  
  lastCLK = currentCLK;
}

unsigned long readHX711() {
  unsigned long count = 0;
  
  pinMode(HX711_DT, OUTPUT);
  digitalWrite(HX711_DT, HIGH);
  digitalWrite(HX711_SCK, LOW);
  pinMode(HX711_DT, INPUT);
  
  while (digitalRead(HX711_DT));
  
  for (int i = 0; i < 24; i++) {
    digitalWrite(HX711_SCK, HIGH);
    count = count << 1;
    digitalWrite(HX711_SCK, LOW);
    if (digitalRead(HX711_DT)) {
      count++;
    }
  }
  
  digitalWrite(HX711_SCK, HIGH);
  count = count ^ 0x800000;
  digitalWrite(HX711_SCK, LOW);
  
  return count;
}
