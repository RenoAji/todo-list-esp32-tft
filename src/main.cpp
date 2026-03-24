#include <Arduino.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// Button Pins
#define BUTTON_UP_PIN 26
#define BUTTON_DOWN_PIN 25
#define BUTTON_SELECT_PIN 32

struct Button{
  uint8_t pin;
  unsigned long lastDebounceTime;
};

void fetchTasks();
void display();
void processButton(Button &button, const char* name);
void completeTask(const char* taskId);
void processSelect();
void drawHighlight();
void updateRow(int index, bool isHighlighted);

const unsigned long debounceDelay = 200;
JsonDocument tasksData;
uint highlightedIndex = 0;
uint prevHighlightedIndex = 0;


Button b_up = {BUTTON_UP_PIN, 0};
Button b_down = {BUTTON_DOWN_PIN, 0};
Button b_select = {BUTTON_SELECT_PIN, 0};

TFT_eSPI tft = TFT_eSPI();
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  esp_log_level_set("*", ESP_LOG_NONE);
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);
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
  fetchTasks();
  drawHighlight();
}

void loop() {
  processButton(b_up, "UP");
  processButton(b_down, "DOWN");
  processButton(b_select, "SELECT");
}

void processButton(Button &button, const char* name) {
  if ((digitalRead(button.pin) )== LOW && (millis() - button.lastDebounceTime > debounceDelay) ){
    Serial.printf("%s button pressed\n", name);
    button.lastDebounceTime = millis();

    if (strcmp(name, "UP") == 0) {
      if (highlightedIndex > 0) {
        prevHighlightedIndex = highlightedIndex;
        highlightedIndex--;
        drawHighlight();
      }
    }else if (strcmp(name, "DOWN") == 0) {
      if (highlightedIndex < tasksData.size()) {
        prevHighlightedIndex = highlightedIndex;
        highlightedIndex++;
        drawHighlight();
      }
    }else if (strcmp(name, "SELECT") == 0) {
      processSelect();
    }
    Serial.println(highlightedIndex);
    Serial.println(tasksData.size());
  }
}

void completeTask(const char* taskId) {
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  String url = String(SCRIPT_URL) + "?action=completeTask&id=" + taskId;
  if (http.begin(client, url)) {
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET();
    String payload = http.getString();

    JsonDocument res;
    DeserializationError error = deserializeJson(res, payload);

    // error check
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      tft.fillScreen(TFT_RED);
      tft.println("JSON Error!");
      return;
    }

    if (!res["s"])
    {
      Serial.println("Failed to complete task");
      tft.fillScreen(TFT_RED);
      tft.println("Failed to complete task");
      Serial.println(res["e"].as<const char*>());
    } else {
      Serial.println("Task completed successfully, refreshing tasks...");
      fetchTasks();
    }
    http.end();
  }
}

void fetchTasks() {
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  String url = String(SCRIPT_URL) + "?action=fetchTask";
  if (http.begin(client, url)) {
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.printf("[HTTP] Response Code: %d\n", httpCode);
      
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Received Tasks:");
        Serial.println(payload); // Check your Serial Monitor for the JSON!

        DeserializationError error = deserializeJson(tasksData, payload);

        // error check
        if (error) {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          tft.fillScreen(TFT_RED);
          tft.println("JSON Error!");
          return;
        }

        prevHighlightedIndex = 0;
        highlightedIndex = 0;
        display();
    }else {
      Serial.printf("[HTTP] Failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
    }
  }
}



void display(){
  tft.fillScreen(TFT_BLACK);
  updateRow(0, true);
  for (int i = 0; i < tasksData.size(); i++)
  {
    updateRow(i + 1, false);
  }
  
}

void processSelect(){
  if(highlightedIndex == 0){
    fetchTasks();
  }
  if ((highlightedIndex - 1) < tasksData.size() )
  {
    String id = tasksData[highlightedIndex - 1]["i"];
    Serial.printf("Selected Task ID: %s\n", id.c_str());
    completeTask(id.c_str());

  }

}

void updateRow(int index, bool isHighlighted) {
  int y, h;
  if (index == 0) {
    y = 0; h = 16;
  } else {
    y = 16 + ((index - 1) * 24);
    h = 24;
  }

  // Set Colors based on highlight status
  uint16_t bgColor = isHighlighted ? TFT_WHITE : TFT_BLACK;
  uint16_t textColor = isHighlighted ? TFT_BLACK : TFT_WHITE;

  // Draw Background
  tft.fillRect(0, y, tft.width(), h, bgColor);

  // Draw Text
  tft.setTextColor(textColor);
  
  if (index == 0) {
    tft.setCursor(4, y + (h/4));
    tft.print("--- TODO LIST ---");
  } else {
      tft.setCursor(4, y + 2);
      const char* title = tasksData[index - 1]["t"];
      tft.printf("Task: %s", title);
      tft.setCursor(4, y + 12);
      if (tasksData[index - 1]["d"].is<const char*>())
      {
        const char* due = tasksData[index - 1]["d"];
        char shortDate[11];
        strncpy(shortDate, due, 10);
        shortDate[10] = '\0';
        tft.printf("due: %s", shortDate);
      }else{
        tft.print("No Deadline");
      }
  }
}

void drawHighlight() {
  if (prevHighlightedIndex != highlightedIndex) {
    updateRow(prevHighlightedIndex, false);
    updateRow(highlightedIndex, true);
    
    prevHighlightedIndex = highlightedIndex;
  }
}