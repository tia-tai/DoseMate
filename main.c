#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7789.h>  // Hardware-specific library for ST7789

#define TFT_CS 10
#define TFT_DC 8
#define TFT_RST 9

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Colors (16-bit RGB 565 format)
#define BLACK 0x0000
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

const int PIN_CLK = 2;
const int PIN_DT = 3;
const int PIN_SW = 4;
int encoderPos = 0;
int lastEncoderPos = 0;
int buttonVal = 0;
bool buttonClick = false;
String screen = "Home";

const char alphabet[52][1] = { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" };
int onLetter = 0;

const char* dispensers[6] = {"Slot 1", "Slot 2", "Slot 3", "Slot 4", "Exit"};
int dispenser = 0;

const char* settings[3] = {"Interval", "Amount", "Exit"};
int setting = 0;

int value[4][3] = {
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
};

void setup() {
  tft.init(240, 240);

  // Set rotation (0-3)
  tft.setRotation(2);

  // Fill screen with black
  tft.fillScreen(BLACK);

  // Display welcome message
  render_Startup();

  pinMode(PIN_CLK, INPUT_PULLUP);
  pinMode(PIN_DT, INPUT_PULLUP);
  pinMode(PIN_SW, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_CLK), checkEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_DT), checkEncoder, CHANGE);

  delay(2000);
  render_Home();
}

void loop() {}
}

void render_Startup() {
  tft.setCursor(30, 0);
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.println("Getting Things");
  tft.setCursor(50, 100);
  tft.println("Ready");
  tft.setCursor(90, 10);
  tft.setTextSize(2);
  tft.println("Please Hold");
}

void render_Home() {
  tft.setCursor(30, 40);
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.println("Welcome to");
  tft.setCursor(50, 70);
  tft.println("DoseMate");
  tft.setCursor(10, 150);
  tft.setTextSize(2);
  tft.println("Click to Enter Menu");
}

void render_Menu() {

}

void render_Setting() {
  
}

void render_NoPill() {
  tft.setCursor(50, 40);
  tft.setTextColor(RED);
  tft.setTextSize(6);
  tft.println("NEED");
  tft.setCursor(15, 120);
  tft.println("RELOAD");
}

void render_Alert() {
  tft.setCursor(30, 40);
  tft.setTextColor(GREEN);
  tft.setTextSize(6);
  tft.println("PILLS");
  tft.setCursor(30, 120);
  tft.println("READY");
}

void checkEncoder() {
  // Read the current state of pins A and B
  int a = digitalRead(PIN_CLK);
  int b = digitalRead(PIN_DT);
 
  // Determine rotation direction using state changes
  static int lastA = LOW;
  static int lastB = LOW;
 
  if (a != lastA || b != lastB) {
    if (lastA == LOW && a == HIGH) {
      // A rising edge
      if (b == LOW) {
        // Clockwise rotation
        encoderPos++;
      } else {
        // Counterclockwise rotation
        encoderPos--;
      }
    }
   
    // Update last states
    lastA = a;
    lastB = b;
  }
}
