#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <WiFi.h>
#include <RPC.h>

#define TFT_CS 10
#define TFT_DC 8
#define TFT_RST 9

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

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

// Wifi credentials
String ssid = "";
String pass = "";
const char* wifiOptions[3] = {"Connect", "New Wifi", "Exit"};
int wifiOption = 0;
int status = WL_IDLE_STATUS;
int keyIndex = 0;            // your network key index number (needed only for WEP)
unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServer(162, 159, 200, 123); // pool.ntp.org NTP server
const int NTP_PACKET_SIZE = 48; // NTP timestamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP Udp; // A UDP instance to let us send and receive packets over UDP

// Menu options
const char* dispensers[6] = {"Slot 1", "Slot 2", "Slot 3", "Slot 4", "Wifi", "Exit"};
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

// Alphabet for Wifi input
String alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*_-+=/?<>.{}";
int letter = 0;
int inputMode = 0; // 0 = SSID, 1 = Password, 2 = Exit

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

  if (status == WL_CONNECTED) {
    currentTime = readTime();
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
    } else if (dispenser == 4) {
      process = "Wifi";
      wifiOption = 0;
      render_Wifi();
    } else {
      process = "Setting";
      setting = 0;
      render_Setting();
    }
  } 
  else if (process == "Setting") {
    handleSettingClick();
  } 
  else if (process == "Wifi") {
    handleWifiClick();
  }
  else if (process == "New Wifi") {
    handleNewWifiClick();
  }
  else if (process == "noWifi") {
    process = "Menu";
    render_Menu();
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

void handleWifiClick() {
  if (wifiOption == 0) {
    connectToWifi();
  } else if (wifiOption == 1) {
    process = "New Wifi";
    inputMode = 0;
    letter = 0;
    render_NewWifi();
  } else {
    process = "Menu";
    wifiOption = 0;
    render_Menu();
  }
}

void handleNewWifiClick() {
  if (alphabet[letter] == '{') {
    if (inputMode == 1) {
      inputMode = 0;
      letter = 0;
      render_NewWifi();
    } else if (inputMode == 0) {
      process = "Wifi";
      letter = 0;
      inputMode = 0;
      render_Wifi();
    }
  }
  else if (alphabet[letter] == '}') {
    if (inputMode == 0) {
      inputMode = 1;
      letter = 0;
      render_NewWifi();
    } else if (inputMode == 1) {
      process = "Wifi";
      letter = 0;
      inputMode = 0;
      render_Wifi();
    }
  }
  else {
    if (inputMode == 0) {
      ssid += alphabet[letter];
      render_NewWifi();
    } else if (inputMode == 1) {
      pass += alphabet[letter];
      render_NewWifi();
    }
  }
}

void handleEncoder() {
  int change = encoderPos - lastEncoderPos;
  
  if (process == "Menu") {
    dispenser = (dispenser + change + 6) % 6;
  } 
  else if (process == "Wifi") {
    wifiOption = (wifiOption + change + 3) % 3;
  }
  else if (process == "New Wifi") {
    letter = (letter + change + alphabet.length()) % alphabet.length();
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
  } else if (process == "Wifi") {
    render_Wifi();
  } else if (process == "New Wifi") {
    render_NewWifi();
  } else if (process == "noWifi") {
    render_WifiNotFound();
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

void render_Wifi() {
  tft.setCursor(80, 40);
  tft.setTextColor(BLUE);
  tft.setTextSize(3);
  tft.println("Wifi");
  
  tft.setCursor(40, 100);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.println(wifiOptions[wifiOption]);
  
  tft.setCursor(20, 180);
  tft.setTextSize(2);
  tft.println("Click to Select");
}

void render_NewWifi() {
  tft.setCursor(20, 20);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  
  if (inputMode == 0) {
    tft.println("Enter SSID:");
    tft.setCursor(20, 60);
    tft.print("SSID: ");
    tft.println(ssid);
  } else {
    tft.println("Enter Password:");
    tft.setCursor(20, 60);
    tft.print("Pass: ");
    for (int i = 0; i < pass.length(); i++) {
      tft.print("*");
    }
  }
  
  tft.setCursor(80, 120);
  tft.setTextSize(4);
  tft.setTextColor(YELLOW);
  tft.println(alphabet[letter]);
  
  tft.setCursor(10, 170);
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  if (alphabet[letter] == '[') {
    tft.println("[ = Go Back");
  } else if (alphabet[letter] == ']') {
    if (inputMode == 0) {
      tft.println("] = Next (Password)");
    } else {
      tft.println("] = Done");
    }
  } else {
    tft.println("Rotate to select letter");
  }
  
  tft.setCursor(20, 200);
  tft.setTextSize(2);
  tft.println("Click to enter");
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

void connectToWifi() {
  if (ssid.length() == 0) {
    tft.fillScreen(BLACK);
    tft.setCursor(20, 100);
    tft.setTextColor(RED);
    tft.setTextSize(2);
    tft.println("No Wifi Found!");
    delay(2000);
    render_Wifi();
    return;
  }

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    tft.setTextColor(RED);
    tft.println("Failed!");
    while (true);
  }
  else {
    tft.fillScreen(BLACK);
    tft.setCursor(20, 100);
    tft.setTextColor(YELLOW);
    tft.setTextSize(2);
    tft.println("Connecting...");

    // attempt to connect to WiFi network:
    int attempts = 0;
    while (status != WL_CONNECTED && attempts < 20) {
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(ssid);
      tft.setTextColor(WHITE);
      // Connect to WPA/WPA2 network. Change this line if using open network:
      status = WiFi.begin(ssid.c_str(), pass.c_str());

      // wait 3 seconds for connection:
      delay(3000);
      attempts++;
    }
    
    tft.fillScreen(BLACK);
    tft.setCursor(20, 100);
    
    if (WiFi.status() == WL_CONNECTED) {
      tft.setTextColor(GREEN);
      tft.println("Connected!");
      Serial.println(WiFi.localIP());
      Serial.println("\nStarting connection to server...");
      Udp.begin(localPort);
    } else {
      tft.setTextColor(RED);
      tft.println("Failed!");
    }
  }
  
  delay(2000);
  tft.fillScreen(BLACK);
  render_Wifi();
}

void render_WifiNotFound() {
  tft.fillScreen(BLACK);
  tft.setCursor(10, 80);
  tft.setTextColor(RED);
  tft.setTextSize(4);
  tft.println("WiFi Not Found");
  tft.setCursor(10, 130);
  tft.println("Click to Connect");
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

unsigned long sendNTPpacket(IPAddress& address) {
  if (status != WL_CONNECTED) {
    process = "noWifi";
  } else {
    Serial.println("1");
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    Serial.println("2");
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;

    Serial.println("3");

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    Udp.beginPacket(address, 123); //NTP requests are to port 123
    Serial.println("4");
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Serial.println("5");
    Udp.endPacket();
    Serial.println("6");
  }
}

unsigned long readTime() {
  sendNTPpacket(timeServer); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  if (Udp.parsePacket()) {
    Serial.println("packet received");
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = ");
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);

    return epoch;
  }
}

unsigned long nextInterval(int interval) {
  unsigned long nextTime = currentTime + (interval * 3600);
  return nextTime;
}

unsigned long checkTime(unsigned long time, int interval) {
  if (time <= currentTime) {
    Serial.println("Time is up");
    return nextInterval(interval);
  } else {
    return time;
  }
}