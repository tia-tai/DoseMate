#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789

#define TFT_CS  10
#define TFT_DC   8
#define TFT_RST  9

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Colors (16-bit RGB 565 format)
#define BLACK    0x0000
#define RED      0xF800
#define GREEN    0x07E0
#define BLUE     0x001F
#define YELLOW   0xFFE0
#define WHITE    0xFFFF

const int PIN_CLK = 2;
const int PIN_DT = 3;
const int PIN_SW = 4;
int encoderPos = 0;    
int lastEncoderPos = 0;
int buttonVal = 0;
bool buttonClick = false;

const char alphabet[52][1] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "A", "B", "C", "D","E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};
int onLetter = 0;

void setup() {
  tft.init(240, 240);
  
  // Set rotation (0-3)
  tft.setRotation(2);
  
  // Fill screen with black
  tft.fillScreen(BLACK);
  
  // Display welcome message
  tft.setCursor(0, 40);
  tft.setTextColor(WHITE);
  tft.setTextSize(5);
  tft.println("STARTING");
  tft.setCursor(90, 120);
  tft.setTextSize(5);
  tft.println("UP");

  pinMode(PIN_CLK, INPUT_PULLUP);
  pinMode(PIN_DT, INPUT_PULLUP);
  pinMode(PIN_SW, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_CLK), checkEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_DT), checkEncoder, CHANGE);
  
  delay(2000);
  tft.fillScreen(BLACK);
}

void loop() {}
}
