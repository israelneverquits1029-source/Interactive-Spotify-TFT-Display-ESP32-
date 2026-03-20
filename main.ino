#include <WiFi.h>
// #include <TFT_eSPI.h>   // display library (to be configured later)

// Pin definitions
const int button1 = 2;
const int button2 = 3;
const int button3 = 4;

void setup() {
  Serial.begin(115200);

  // Initialize buttons
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button3, INPUT_PULLUP);

  // Initialize WiFi (placeholder)
  // WiFi.begin("SSID", "PASSWORD");

  // Initialize display (placeholder)
  // tft.init();

  Serial.println("System initialized");
}

void loop() {
  // Read button states
  if (digitalRead(button1) == LOW) {
    Serial.println("Button 1 pressed");
  }

  if (digitalRead(button2) == LOW) {
    Serial.println("Button 2 pressed");
  }

  if (digitalRead(button3) == LOW) {
    Serial.println("Button 3 pressed");
  }

  // Placeholder for:
  // - Spotify API fetch
  // - Display updates
}
