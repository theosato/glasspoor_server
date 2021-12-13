#include "Arduino.h"
#include <WiFi.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

// Network credentials
const char* ssid = "FREZZATO_2G";
const char* password = "matiasfrezzato";

WebServer server(80);

Preferences preferences;

void getConfigurations() {
    DynamicJsonDocument doc(512);
    
    DynamicJsonDocument color(512);
    color["red"] = preferences.getUInt("red", 0);
    color["green"] = preferences.getUInt("green", 0);
    color["blue"] = preferences.getUInt("blue", 0);
    doc["color"] = color;

    DynamicJsonDocument location(512);
    location["country"] = preferences.getString("country", "");
    location["state"] = preferences.getString("state", "");
    location["city"] = preferences.getString("city", "");
    doc["location"] = location;
    
    String buf;
    serializeJson(doc, buf);
    server.send(200, "application/json", buf);
}

void setConfigurations() {
    String postBody = server.arg("plain"); 
    Serial.println(postBody);
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, postBody);
    if (error) {
        server.send(400, F("text/html"), "Error in parsin json body!");
    } else {
        JsonObject postObj = doc.as<JsonObject>();
        if (postObj.containsKey("color") && postObj.containsKey("location")) {
          
            if (postObj["color"].containsKey("red") && postObj["color"].containsKey("green") && postObj["color"].containsKey("blue")){
                preferences.putUInt("red", postObj["color"]["red"]);
                preferences.putUInt("green", postObj["color"]["green"]);
                preferences.putUInt("blue", postObj["color"]["blue"]);
            }

            if (postObj["location"].containsKey("country") && postObj["location"].containsKey("state") && postObj["location"].containsKey("city")){
                const char* country = postObj["location"]["country"];
                preferences.putString("country", country);
                const char* state = postObj["location"]["state"];
                preferences.putString("state", state);
                const char* city = postObj["location"]["city"];
                preferences.putString("city", city);
            }

            DynamicJsonDocument doc(512);
            doc["status"] = "OK";
            String buf;
            serializeJson(doc, buf);
            server.send(201, F("application/json"), buf);
        } else {
            DynamicJsonDocument doc(512);
            doc["status"] = "KO";
            doc["message"] = F("Data not found or incorrect");
            String buf;
            serializeJson(doc, buf);
            server.send(400, F("application/json"), buf);
        }
    }
}
 
// Define routing
void restServerRouting() {
    server.on("/", HTTP_GET, []() {
        server.send(200, F("text/html"),
            F("Welcome to the REST Web Server"));
    });
    server.on(F("/config"), HTTP_GET, getConfigurations);
    server.on(F("/config"), HTTP_POST, setConfigurations);
}
 
// Manage not found URL
void handleNotFound() {
    DynamicJsonDocument doc(512);
    doc["message"] = "Not found";
    String buf;
    serializeJson(doc, buf);
    server.send(404, "application/json", buf);
}

void setup() {
  Serial.begin(115200);
  preferences.begin("config", false);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // permite chamar o server em glasspoor.com
  if (MDNS.begin("glasspoor")) {
    Serial.println("MDNS started");
  }

  restServerRouting();
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(){
  server.handleClient();
}
