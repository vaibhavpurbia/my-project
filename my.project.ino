#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "esp_sleep.h"
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// ---------- WiFi ----------
#define WIFI_SSID "Vipul"
#define WIFI_PASSWORD "123123123"

// ---------- Firebase ----------
#define API_KEY "AIzaSyAari4Vnpyo1DjEgSFz5Z9UYT8yWSRfIAY"
#define DATABASE_URL "https://urban-pulse-pavements-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define DATABASE_SECRET "lGaxG6yuGN7gOdQkK6wMvnLdBvZEBKxR20iZYKBx"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ---------- Pins ----------
#define PIEZO_PIN 34
#define VIBRATION_PIN 27

// ---------- Thresholds ----------
#define PIEZO_THRESHOLD 200      // touch detection
#define OVERSPEED_RISE_MS 30   // FAST impact threshold (demo friendly)

void setup() {
  Serial.begin(115200);

  pinMode(VIBRATION_PIN, INPUT);
  analogReadResolution(12);

  // Wake-up reason
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Woke up due to vibration!");
  }

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  // Firebase Config
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // ---------- PIEZO MEASUREMENT ----------
  Serial.println("Waiting for Tile Press...");

  unsigned long riseStart = 0;
  unsigned long riseEnd = 0;
  int peakValue = 0;

  unsigned long timeout = millis();

  while (true) {
    int val = analogRead(PIEZO_PIN);

    // 1. Detect start of pressure
    if (val > PIEZO_THRESHOLD && riseStart == 0) {
      riseStart = millis();
    }

    // 2. Track peak + rise end
    if (riseStart != 0 && val > peakValue) {
      peakValue = val;
      riseEnd = millis();
    }

    // 3. Stop when signal stabilizes or timeout
    if (riseStart != 0 && millis() - riseEnd > 150) {
      break;
    }

    if (millis() - timeout > 3000) {
      Serial.println("Timeout: No valid press detected.");
      break;
    }
  }

  // ---------- RISE TIME (SPEED LOGIC) ----------
  int riseTime = 0;
  if (riseEnd > riseStart) {
    riseTime = riseEnd - riseStart;
  }

  // ---------- OBJECT CLASSIFICATION ----------
  String type = "Unknown";
  if (peakValue > 0) {
    if (peakValue < 800) type = "Human";
    else if (peakValue < 1500) type = "Cycle";
    else if (peakValue < 2600) type = "Car";
    else type = "Truck";
  }

  // ---------- SPEED CLASSIFICATION ----------
  String speedStatus = "Normal";
  if (riseTime > 0 && riseTime < OVERSPEED_RISE_MS) {
    speedStatus = "Overspeed";
  }

  // ---------- DANGER LOGIC ----------
  bool danger = (speedStatus == "Overspeed" &&
                (type == "Car" || type == "Truck"));

  // ---------- FIREBASE UPLOAD ----------
  if (Firebase.ready() && peakValue > 0) {
    FirebaseJson json;

    json.set("peakADC", peakValue);
    json.set("riseTime_ms", riseTime);
    json.set("type", type);
    json.set("speedStatus", speedStatus);
    json.set("danger", danger);
    json.set("timestamp", millis());

    Serial.println("Uploading Data...");

    if (Firebase.RTDB.setJSON(&fbdo, "/urbanPulse/LiveFeed", &json)) {
      Serial.println("Upload Success");
    } else {
      Serial.print("Firebase Error: ");
      Serial.println(fbdo.errorReason());
    }
  }

  delay(1000);

  // ---------- DEEP SLEEP ----------
  Serial.println("Going to Sleep");
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 1);
  esp_deep_sleep_start();
}

void loop() {} 