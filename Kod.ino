#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

// --- Konfiguracja URL ---
const char* urlQueue  = "https://boxlej.vercel.app/api/queue";
const char* urlReset  = "https://boxlej.vercel.app/api/queue/reset";
const char* urlResult = "https://boxlej.vercel.app/api/results";

LiquidCrystal_I2C lcd(0x27, 20, 4);
Servo s;

// --- Piny ---
const int serwoPin       = 27;
const int posOtwarta     = 90;
const int posZamknieta   = 60;
const int buttonPin      = 4;
const int skipPin        = 26;
const int buttonResetPin = 14;
const int hallPin        = 13;
const int serwoTogglePin = 25;

// --- Timingi i Stany ---
const unsigned long flowTimeout  = 500;
const unsigned long pollInterval = 2000;

enum State { WAITING_FOR_PLAYER, PLAYER_READY_PROMPT, MEASURING, SHOWING_RESULT };
State appState = WAITING_FOR_PLAYER;

String currentPlayerId   = "";
String currentPlayerName = "";
String nextPlayerName    = "";
String lastPlayerName    = "";
String lastSkippedName   = "";
float  lastPlayerTime    = 0;

unsigned long lastPollTime    = 0;
unsigned long lastImpulseTime = 0;
unsigned long actualStartTime = 0;
bool measuring                = false;
bool serwoOtwarty             = false;
int lastHallState             = HIGH;
unsigned long lastLcdUpdate   = 0;

WiFiClientSecure secureClient;

// --- Funkcje API ---

void resetQueueAPI() {
  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.print("Resetowanie bazy...");
    secureClient.setInsecure();
    HTTPClient http;
    http.begin(secureClient, urlReset);
    int httpResponseCode = http.POST("");
    if (httpResponseCode > 0) {
      Serial.print("Kolejka zresetowana. Kod: ");
      Serial.println(httpResponseCode);
      lcd.setCursor(0, 1);
      lcd.print("Gotowe!");
    } else {
      Serial.print("Blad resetu. Kod: ");
      Serial.println(httpResponseCode);
      lcd.setCursor(0, 1);
      lcd.print("Blad: "); lcd.print(httpResponseCode);
    }
    http.end();
    currentPlayerId = ""; currentPlayerName = "";
    nextPlayerName = ""; lastSkippedName = "";
    appState = WAITING_FOR_PLAYER;
    delay(1500);
    lcd.clear();
    lcd.print("Czekam na gracza");
  }
}

bool fetchNextPlayer() {
  if (WiFi.status() != WL_CONNECTED) return false;
  secureClient.setInsecure();
  HTTPClient http;
  String urlWithCache = String(urlQueue) + "?t=" + String(millis());
  http.begin(secureClient, urlWithCache);
  int code = http.GET();
  if (code != 200) { http.end(); return false; }
  String body = http.getString();
  http.end();
  StaticJsonDocument<2048> doc;
  if (deserializeJson(doc, body)) return false;
  JsonArray queue = doc["queue"].as<JsonArray>();
  if (queue.size() == 0) { lastSkippedName = ""; return false; }
  String tempName = queue[0]["nickname"].as<String>();
  if (tempName == lastSkippedName) return false;
  currentPlayerId   = queue[0]["id"].as<String>();
  currentPlayerName = tempName;
  nextPlayerName    = queue.size() >= 2 ? queue[1]["nickname"].as<String>() : "";
  return true;
}

void skipPlayer(String nickname) {
  if (WiFi.status() != WL_CONNECTED || nickname == "") return;
  secureClient.setInsecure();
  HTTPClient http;
  http.begin(secureClient, urlQueue);
  http.addHeader("Content-Type", "application/json");
  String body = "{\"nickname\":\"" + nickname + "\"}";
  http.sendRequest("DELETE", body);
  http.end();
}

void sendResult(float seconds) {
  if (WiFi.status() != WL_CONNECTED) return;
  secureClient.setInsecure();
  HTTPClient http;
  http.begin(secureClient, urlResult);
  http.addHeader("Content-Type", "application/json");
  String body = "{\"playerId\":\"" + currentPlayerId
              + "\",\"nickname\":\"" + currentPlayerName
              + "\",\"time\":" + String(seconds, 2) + "}";
  http.POST(body);
  http.end();
}

// --- Ekrany LCD ---

void showReadyScreen() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(currentPlayerName.substring(0, 20));
  lcd.setCursor(0, 1); lcd.print("Gotowy?");
  lcd.setCursor(0, 2); lcd.print("[START]     [SKIP]");
  lcd.setCursor(0, 3);
  String next = nextPlayerName != "" ? "Nast: " + nextPlayerName : "Nast: brak";
  lcd.print(next.substring(0, 20));
}

void showResultScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  String res = lastPlayerName + ": " + String(lastPlayerTime, 2) + "s";
  lcd.print(res.substring(0, 20));
  lcd.setCursor(0, 1); lcd.print("--------------------");
  lcd.setCursor(0, 2); lcd.print("[START]     [SKIP]");
  lcd.setCursor(0, 3);
  if (currentPlayerName != "") {
    lcd.print(("Nast: " + currentPlayerName).substring(0, 20));
  } else {
    lcd.print("Kolejka pusta!");
  }
}

// --- Toggle zaworu ---

void toggleSerwo() {
  if (serwoOtwarty) {
    s.write(posZamknieta);
    serwoOtwarty = false;
    // Jeśli trwał pomiar - przerwij bez zapisywania wyniku
    if (appState == MEASURING) {
      measuring = false;
      lastImpulseTime = 0;
      actualStartTime = 0;
      appState = PLAYER_READY_PROMPT;
      showReadyScreen();
    }
  } else {
    s.write(posOtwarta);
    serwoOtwarty = true;
  }
}

// --- Setup ---

void setup() {
  Serial.begin(115200);

  pinMode(serwoPin, OUTPUT);
  digitalWrite(serwoPin, LOW);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  pinMode(buttonPin,      INPUT_PULLUP);
  pinMode(skipPin,        INPUT_PULLUP);
  pinMode(buttonResetPin, INPUT_PULLUP);
  pinMode(hallPin,        INPUT_PULLUP);
  pinMode(serwoTogglePin, INPUT_PULLUP);

  lastHallState = digitalRead(hallPin);

  lcd.clear();
  lcd.print("Uruchamiam WiFi...");

  WiFiManager wm;
  if (!wm.autoConnect("Lej_Konfiguracja", "pawelszef")) {
    lcd.clear();
    lcd.print("Blad WiFi. Reset.");
    delay(3000);
    ESP.restart();
  }

  lcd.clear();
  lcd.print("WiFi OK!");
  delay(1000);

  s.attach(serwoPin, 600, 2400);
  delay(200);
  s.write(posZamknieta);
  serwoOtwarty = false;
  delay(1000);

  lcd.clear();
  lcd.print("Czekam na gracza");
}

// --- Loop ---

void loop() {
  unsigned long now = millis();
  int currentHallState = digitalRead(hallPin);

  if (WiFi.status() != WL_CONNECTED) {
    lcd.setCursor(19, 0);
    lcd.print("!");
  }

  // --- TOGGLE ZAWORU (działa zawsze) ---
  if (digitalRead(serwoTogglePin) == LOW) {
    delay(50);
    if (digitalRead(serwoTogglePin) == LOW) {
      toggleSerwo();
      while (digitalRead(serwoTogglePin) == LOW) { delay(10); }
    }
  }

  // --- RESET KOLEJKI ---
  if (digitalRead(buttonResetPin) == LOW) {
    delay(50);
    if (digitalRead(buttonResetPin) == LOW) {
      resetQueueAPI();
      while (digitalRead(buttonResetPin) == LOW) { delay(10); }
    }
  }

  // --- WAITING_FOR_PLAYER ---
  if (appState == WAITING_FOR_PLAYER) {
    if (now - lastPollTime >= pollInterval) {
      lastPollTime = now;
      if (fetchNextPlayer()) {
        appState = PLAYER_READY_PROMPT;
        showReadyScreen();
      }
    }
    lastHallState = currentHallState;
    return;
  }

  // --- PLAYER_READY_PROMPT ---
  if (appState == PLAYER_READY_PROMPT) {
    if (digitalRead(skipPin) == LOW) {
      delay(200);
      lcd.clear(); lcd.print("Pomijam gracza...");
      lastSkippedName = currentPlayerName;
      skipPlayer(currentPlayerName);
      delay(1200);
      currentPlayerId = ""; currentPlayerName = ""; nextPlayerName = "";
      appState = WAITING_FOR_PLAYER;
      lastPollTime = millis();
      lcd.clear(); lcd.print("Czekam na gracza");
      return;
    }
    if (digitalRead(buttonPin) == LOW) {
      delay(200);
      lastSkippedName = "";
      appState = MEASURING;
      measuring = false; lastImpulseTime = 0; actualStartTime = 0;
      lastHallState = currentHallState;
      s.write(posOtwarta);
      serwoOtwarty = true;
      lcd.clear(); lcd.print("ZAWOR OTWARTY");
      lcd.setCursor(0, 1); lcd.print("Czekam na plyn...");
    }
    return;
  }

  // --- MEASURING ---
  if (appState == MEASURING) {
    if (currentHallState != lastHallState) {
      lastHallState = currentHallState;
      lastImpulseTime = millis();

      if (!measuring) {
        measuring = true;
        actualStartTime = lastImpulseTime;
        lcd.clear();
        lcd.print("TRWA POMIAR...");
      }
    }

    if (measuring) {
      unsigned long t = millis();

      if (t - lastLcdUpdate > 100) {
        float liveTime = (t - actualStartTime) / 1000.0;
        lcd.setCursor(0, 1);
        lcd.print("Czas: "); lcd.print(liveTime, 1); lcd.print(" s    ");
        lastLcdUpdate = t;
      }

      if (lastImpulseTime > 0 && t - lastImpulseTime > flowTimeout) {
        float totalSeconds = (lastImpulseTime - actualStartTime) / 1000.0;
        if (totalSeconds < 0) totalSeconds = 0;
        s.write(posZamknieta);
        serwoOtwarty = false;
        lastPlayerName = currentPlayerName;
        lastPlayerTime = totalSeconds;
        sendResult(totalSeconds);
        lastSkippedName = currentPlayerName;
        skipPlayer(currentPlayerName);
        delay(1200);
        bool hasNext = fetchNextPlayer();
        appState = SHOWING_RESULT; lastPollTime = millis();
        if (!hasNext) { currentPlayerName = ""; currentPlayerId = ""; nextPlayerName = ""; }
        showResultScreen();
      }
    }
  }

  // --- SHOWING_RESULT ---
  if (appState == SHOWING_RESULT) {
    if (digitalRead(skipPin) == LOW) {
      delay(200);
      if (currentPlayerName != "") {
        lastSkippedName = currentPlayerName;
        skipPlayer(currentPlayerName);
        delay(1200);
      }
      currentPlayerId = ""; currentPlayerName = ""; nextPlayerName = "";
      appState = WAITING_FOR_PLAYER; lastPollTime = 0;
      lcd.clear(); lcd.print("Czekam na gracza");
      return;
    }
    if (digitalRead(buttonPin) == LOW) {
      delay(200);
      if (currentPlayerName != "") {
        lastSkippedName = "";
        appState = MEASURING;
        measuring = false; lastImpulseTime = 0; actualStartTime = 0;
        lastHallState = currentHallState;
        s.write(posOtwarta);
        serwoOtwarty = true;
        lcd.clear(); lcd.print("ZAWOR OTWARTY");
      }
      return;
    }
    if (now - lastPollTime >= 6000) {
      if (currentPlayerName != "") { appState = PLAYER_READY_PROMPT; showReadyScreen(); }
      else { appState = WAITING_FOR_PLAYER; lastPollTime = 0; lcd.clear(); lcd.print("Czekam na gracza"); }
    }
  }
}