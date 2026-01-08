/*
 * PROJECT: Urban Pulse - IoT Traffic Monitoring System
 * AUTHOR: Vaibhav Purbia
 * DATE: Jan 2026
 * STATUS: PATENT PENDING
 * * DESCRIPTION:
 * Firmware for ESP32-based piezoelectric traffic analysis node.
 * Handles sensor data acquisition, edge computing for vehicle classification,
 * and secure transmission to Google Firebase.
 * * NOTE: 
 * Due to ongoing intellectual property protection, specific hardware schematics 
 * and sensor calibration constants have been obfuscated or omitted.
 * Commercial use or reverse engineering is strictly prohibited.
 */

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "esp_sleep.h"
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// ---------------------------------------------------
// 1. CONFIGURATION (CREDENTIALS REDACTED)
// ---------------------------------------------------
#define WIFI_SSID "YOUR_WIFI_SSID"          // <--- User to configure
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"  // <--- User to configure

// --- Firebase Credentials ---
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "https://your-project-id.firebasedatabase.app/"
#define DATABASE_SECRET "YOUR_DATABASE_SECRET"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ---------------------------------------------------
// 2. HARDWARE PIN DEFINITIONS
// ---------------------------------------------------
#define PIEZO_PIN 34        // ADC1 Channel for Piezo Input
#define VIBRATION_PIN 27    // Digital Input for Wake-up Interrupt

// ---------------------------------------------------
// 3. ANALYSIS THRESHOLDS (CALIBRATED)
// ---------------------------------------------------
#define PIEZO_THRESHOLD 200     // ADC trigger level
#define OVERSPEED_RISE_MS 30    // Impact rise time threshold for high speed

void setup() {
  Serial.begin(115200);

  pinMode(VIBRATION_PIN, INPUT);
  analogReadResolution(12);

  // A. Check Wake-up Reason
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("[SYSTEM] Woke up due to vibration trigger.");
  } else {
    Serial.println("[SYSTEM] Normal Boot.");
  }

  // B. Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[NETWORK] Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[NETWORK] Connected.");

  // C. Configure Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // ------------------------------------------------
  // D. SENSOR DATA ACQUISITION
  // ------------------------------------------------
  Serial.println("[SENSOR] Waiting for Tile Press...");

  unsigned long riseStart = 0;
  unsigned long riseEnd = 0;
  int peakValue = 0;

  unsigned long timeout = millis();

  while (true) {
    int val = analogRead(PIEZO_PIN);

    // 1. Detect start of pressure event
    if (val > PIEZO_THRESHOLD && riseStart == 0) {
      riseStart = millis();
    }

    // 2. Track peak amplitude & rise time end
    if (riseStart != 0 && val > peakValue) {
      peakValue = val;
      riseEnd = millis();
    }

    // 3. Stop capturing when signal stabilizes (Debounce/Decay)
    if (riseStart != 0 && millis() - riseEnd > 150) {
      break;
    }

    // 4. Safety Timeout
    if (millis() - timeout > 3000) {
      Serial.println("[ERROR] Timeout: No valid press detected.");
      break;
    }
  }

  // ------------------------------------------------
  // E. EDGE COMPUTING (SPEED LOGIC)
  // ------------------------------------------------
  int riseTime = 0;
  if (riseEnd > riseStart) {
    riseTime = riseEnd - riseStart;
  }

  // ------------------------------------------------
  // F. EDGE COMPUTING (CLASSIFICATION LOGIC)
  // ------------------------------------------------
  String type = "Unknown";
  if (peakValue > 0) {
    if (peakValue < 800) type = "Human";
    else if (peakValue < 1500) type = "Cycle";
    else if (peakValue < 2600) type = "Car";
    else type = "Truck";
  }

  String speedStatus = "Normal";
  if (riseTime > 0 && riseTime < OVERSPEED_RISE_MS) {
    speedStatus = "Overspeed";
  }

  bool danger = (speedStatus == "Overspeed" && (type == "Car" || type == "Truck"));

  // ------------------------------------------------
  // G. CLOUD UPLOAD
  // ------------------------------------------------
  if (Firebase.ready() && peakValue > 0) {
    FirebaseJson json;

    json.set("peakADC", peakValue);
    json.set("riseTime_ms", riseTime);
    json.set("type", type);
    json.set("speedStatus", speedStatus);
    json.set("danger", danger);
    json.set("timestamp", millis());

    Serial.println("[CLOUD] Uploading Data...");

    if (Firebase.RTDB.setJSON(&fbdo, "/urbanPulse/LiveFeed", &json)) {
      Serial.println("[CLOUD] Upload Success");
    } else {
      Serial.print("[ERROR] Firebase Error: ");
      Serial.println(fbdo.errorReason());
    }
  }

  delay(1000); // Allow network buffer to clear

  // ------------------------------------------------
  // H. POWER MANAGEMENT (DEEP SLEEP)
  // ------------------------------------------------
  Serial.println("[POWER] Entering Deep Sleep...");
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 1); // Wake on High
  esp_deep_sleep_start();
}

void loop() {
  // Loop is unused in Deep Sleep architecture
}