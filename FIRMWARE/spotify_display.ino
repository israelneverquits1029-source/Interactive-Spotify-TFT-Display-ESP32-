#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define TFT_CS   10
#define TFT_DC   1
#define TFT_RST  3
#define TFT_SCLK 4
#define TFT_MOSI 6

#define BTN_NEXT     7
#define BTN_PREV     5
#define BTN_PLAY     2
#define BTN_VOL_UP   0
#define BTN_VOL_DOWN 8

const char* ssid = "ROYALTY";
const char* password = "DINOMASTER101$";
const char* serverBase = "http://10.200.56.58:5000";

Adafruit_ST7735 tft = Adafruit_ST7735(
  TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST
);

String title = "Loading...";
String artist = "";
bool playing = false;

int progress = 0;
int duration = 100;
int volumeLevel = 50;

unsigned long lastFetch = 0;
unsigned long lastProgressTick = 0;
unsigned long lastButtonPress = 0;
unsigned long volumePopupTime = 0;

bool showVolumePopup = false;

const unsigned long fetchInterval = 900;
const unsigned long debounceDelay = 120;
const unsigned long volumePopupDuration = 900;

void setup() {
  Serial.begin(115200);

  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_PLAY, INPUT_PULLUP);
  pinMode(BTN_VOL_UP, INPUT_PULLUP);
  pinMode(BTN_VOL_DOWN, INPUT_PULLUP);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  drawMessage("Connecting WiFi...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }

  drawMessage("WiFi Connected");
  delay(500);

  fetchData();
  drawUI();
}

void loop() {
  unsigned long now = millis();

  handleButtons();

  if (playing && now - lastProgressTick >= 1000) {
    lastProgressTick = now;
    if (progress < duration) {
      progress++;
      drawUI();
    }
  }

  if (now - lastFetch >= fetchInterval) {
    lastFetch = now;
    fetchData();
    drawUI();
  }

  if (showVolumePopup && now - volumePopupTime >= volumePopupDuration) {
    showVolumePopup = false;
    drawUI();
  }
}

void fetchData() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.setTimeout(600);

  String url = String(serverBase) + "/status";
  http.begin(url);

  int code = http.GET();

  if (code > 0) {
    String payload = http.getString();

    StaticJsonDocument<768> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      title = doc["title"].as<String>();
      artist = doc["artist"].as<String>();
      playing = doc["playing"];
      progress = doc["progress"];
      duration = doc["duration"];

      if (duration <= 0) duration = 100;
      if (progress < 0) progress = 0;
      if (progress > duration) progress = duration;
    }
  }

  http.end();
}

void sendCommand(String cmd) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.setTimeout(400);

  String url = String(serverBase) + "/control/" + cmd;
  http.begin(url);
  http.GET();
  http.end();
}

void handleButtons() {
  if (millis() - lastButtonPress < debounceDelay) return;

  if (!digitalRead(BTN_NEXT)) {
    sendCommand("next");
    title = "Changing...";
    artist = "";
    progress = 0;
    drawUI();
    lastButtonPress = millis();
  }

  if (!digitalRead(BTN_PREV)) {
    sendCommand("previous");
    title = "Changing...";
    artist = "";
    progress = 0;
    drawUI();
    lastButtonPress = millis();
  }

  if (!digitalRead(BTN_PLAY)) {
    playing = !playing;
    drawUI();
    sendCommand("playpause");
    lastButtonPress = millis();
  }

  if (!digitalRead(BTN_VOL_UP)) {
    volumeLevel += 5;
    if (volumeLevel > 100) volumeLevel = 100;

    showVolumePopup = true;
    volumePopupTime = millis();

    drawUI();
    sendCommand("volumeup");

    lastButtonPress = millis();
  }

  if (!digitalRead(BTN_VOL_DOWN)) {
    volumeLevel -= 5;
    if (volumeLevel < 0) volumeLevel = 0;

    showVolumePopup = true;
    volumePopupTime = millis();

    drawUI();
    sendCommand("volumedown");

    lastButtonPress = millis();
  }
}

void drawMessage(String msg) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 50);
  tft.print(msg);
}

void drawUI() {
  tft.fillScreen(ST77XX_BLACK);

  tft.fillRect(0, 0, 160, 18, ST77XX_GREEN);
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(7, 5);
  tft.print("MUSIC DISPLAY");

  tft.drawRect(8, 28, 42, 42, ST77XX_WHITE);
  tft.fillRect(12, 32, 34, 34, ST77XX_GREEN);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(58, 28);
  tft.print(trimText(title, 16));

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(58, 43);
  tft.print(trimText(artist, 16));

  tft.setTextColor(playing ? ST77XX_GREEN : ST77XX_YELLOW);
  tft.setCursor(58, 58);
  tft.print(playing ? "Now Playing" : "Paused");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 84);
  tft.print(formatTime(progress));

  tft.setCursor(128, 84);
  tft.print(formatTime(duration));

  tft.drawRect(8, 98, 144, 8, ST77XX_WHITE);

  int bar = map(progress, 0, duration, 0, 142);
  if (bar < 0) bar = 0;
  if (bar > 142) bar = 142;

  tft.fillRect(9, 99, bar, 6, ST77XX_GREEN);

  if (showVolumePopup) {
    drawVolumePopup();
  }
}

void drawVolumePopup() {
  tft.fillRect(20, 45, 120, 40, ST77XX_BLACK);
  tft.drawRect(20, 45, 120, 40, ST77XX_WHITE);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(30, 53);
  tft.print("Volume ");
  tft.print(volumeLevel);
  tft.print("%");

  tft.drawRect(30, 68, 100, 8, ST77XX_WHITE);

  int volBar = map(volumeLevel, 0, 100, 0, 98);
  tft.fillRect(31, 69, volBar, 6, ST77XX_GREEN);
}

String trimText(String txt, int maxLen) {
  if (txt.length() > maxLen) {
    return txt.substring(0, maxLen - 2) + "..";
  }
  return txt;
}

String formatTime(int seconds) {
  int mins = seconds / 60;
  int secs = seconds % 60;

  String result = String(mins) + ":";
  if (secs < 10) result += "0";
  result += String(secs);

  return result;
}
