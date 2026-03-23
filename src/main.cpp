#include <Arduino.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

TFT_eSPI tft = TFT_eSPI();

// put function declarations here:
void fetchTasks();
void display(JsonDocument doc, uint highlight);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  tft.init();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  // tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println("Fetching tasks...");
}

void loop() {
  // put your main code here, to run repeatedly:
  fetchTasks(); // Replace with your actual list name
  delay(60000); // Fetch tasks every 60 seconds
}

// put function definitions here:
void fetchTasks() {
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (http.begin(client, SCRIPT_URL)) {
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.printf("[HTTP] Response Code: %d\n", httpCode);
      
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Received Tasks:");
        Serial.println(payload); // Check your Serial Monitor for the JSON!

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          tft.fillScreen(TFT_RED);
          tft.println("JSON Error!");
          return;
        }

        display(doc, 0);
    }else {
      Serial.printf("[HTTP] Failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
    }
  }
}
void display(JsonDocument doc, uint highlight){
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.println("Todo List:");
  tft.println("----------");
  for (int i = 0; i < doc.size(); i++) {
    const char* title = doc[i]["t"];
    tft.printf("Task: %s\n", title);
    if (!doc[i]["d"].is<const char*>())
    {
      tft.println("No Deadline\n\n");
      continue;
    }
    
    const char* due = doc[i]["d"]; // 1 = done, 0 = todo
    char shortDate[11];
    strncpy(shortDate, due, 10);
    shortDate[10] = '\0'; // Add null terminator

    tft.printf("due: %s\n\n", shortDate);
  }
}