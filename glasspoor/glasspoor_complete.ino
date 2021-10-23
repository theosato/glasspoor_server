// Include libs
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <time.h>
#include <MD_Parola.h>
#include <SPI.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Adafruit_NeoPixel.h>
#include <ESPAsyncWebServer.h>
#include "matrix_font.h"
#include "Arduino.h"

// Define PIN
#define CLK_PIN   18 // or SCK
#define DATA_PIN  19 // or MOSI
#define CS_PIN    5 // or SS
#define RING_PIN 26

// Define constants
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define SPEED_TIME  75
#define PAUSE_TIME  0
#define MAX_MESG  20

// Init special classes 

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(60, RING_PIN, NEO_GRB + NEO_KHZ800);

// ******************* SETUP ***********************

WebServer server(80);

String mode = "clock";

Preferences preferences;

// ************* User Setup 
const char* ssid = "Sato 2g";
const char* password = "81527765";

// ************* Forecast API setup
String openWeatherMapApiKey = "56e9e9d9c9760da98287036625c00cca";

unsigned long timerDelay = 10000; // 10 minutes
unsigned long lastTime = 0;
unsigned long first = true;
String jsonBuffer;

String city;
String countryCode;

char temp_now[9]; 
char temp_max[9]; 
char temp_min[9]; 

// ************* LED Matrix setup 
int dst = 0;
uint16_t  h, m, s;
uint8_t dow;
int  day;
uint8_t month;
String  year;

char szTime[9];    // mm:ss\0
char szsecond[4];    // ss
char szMesg[MAX_MESG+1] = "";

// ******************* FUNCTIONS ***********************

// ************* Forecast functions
String httpGETRequest(const char* serverName) {
    WiFiClient client;
    HTTPClient http;
        
    // Your Domain name with URL path or IP address with path
    http.begin(client, serverName);
    
    // Send HTTP POST request
    int httpResponseCode = http.GET();
    
    String payload = "{}"; 
    
    if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        payload = http.getString();
    }
    else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();

    return payload;
}

void getForecast()
{
   if(WiFi.status()== WL_CONNECTED){
      String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey + "&units=metric";
      jsonBuffer = httpGETRequest(serverPath.c_str());
      Serial.println(jsonBuffer);
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }
    
        temp_now = char(myObject["main"]["temp"])
        temp_max = char(myObject["main"]["temp_max"])
        temp_min = char(myObject["main"]["temp_min"])
    }
    else {
      Serial.println("WiFi Disconnected");
    }
}

// ************* LED Matrix functions
void getsec(char *psz)
{
    sprintf(psz, "%02d", s);
}

void getTime(char *psz, bool f = true)
{
    time_t now = time(nullptr);
    struct tm* p_tm = localtime(&now);
        h = p_tm->tm_hour;
        m = p_tm->tm_min;
        s = p_tm->tm_sec;
    sprintf(psz, "%02d%c%02d", h, (f ? ':' : ' '), m);
    Serial.println(psz);
}

void getTimentp()
{
    configTime(timezoneinSeconds, dst, "pool.ntp.org","time.nist.gov");
    while(!time(nullptr)){
            delay(500);
            Serial.print(".");
    }
    Serial.print("Actual Time: ");
}

// ************* Server functions

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
            doc["status"] = "Error";
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

// ******************* CORE ***********************

void setup() {
    Serial.begin(115200);
    preferences.begin("config", false);

    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());

    WiFi.mode(WIFI_STA);
    
    // permite chamar o server em glasspoor.com
    if (MDNS.begin("glasspoor")) {
        Serial.println("MDNS started");
    }

    restServerRouting();
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP server started");

    getTimentp();

    P.begin(3);
    P.setInvert(false);

    P.setZone(0, 0, 0);
    P.setZone(1, 1, 3);
    P.setFont(0, numeric7Seg);
    P.setFont(1, numeric7Se);
    P.displayZoneText(0, szsecond, PA_LEFT, SPEED_TIME, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayZoneText(1, szTime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
    P.setIntensity (0); // intensity of led
    getTime(szTime);

    pixels.setBrightness(1);
    pixels.begin();

}

void loop() {
    static uint32_t lastTimeMatrix = 0; // millis() memory
    static uint32_t lastTimeForecast = 0; // millis() memory
    static uint8_t  display = 0;  // current display mode
    static bool flasher = false;  // seconds passing flasher
    for (int num = 0; num < 60; num ++){
        pixels.setPixelColor(num, pixels.Color(red, green, blue));
    }
    pixels.show();
    P.displayAnimate();

    if (millis() - lastTimeMatrix >= 1000)
    {
        lastTimeMatrix = millis();
        getsec(szsecond);
        getTime(szTime, flasher);
        flasher = !flasher;

        P.displayReset(0);
        P.displayReset(1);
    }

    // Send an HTTP GET request
    if (((millis() - lastTimeForecast) > timerDelay) || firstTime == true) {
        firstTime = false;
        getForecast();
        lastTimeForecast = millis();
    }
    
    server.handleClient();
}
