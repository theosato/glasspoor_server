#include <WiFi.h>
#include <time.h>
#include <MD_Parola.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

#include "matrix_font.h"

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define CLK_PIN   18 // or SCK
#define DATA_PIN  19 // or MOSI
#define CS_PIN    5 // or SS

// Arbitrary output pins
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

#define SPEED_TIME  75
#define PAUSE_TIME  0
#define MAX_MESG  20

/**********  User Config Setting   ******************************/
char* ssid = "AndroidAP";
char* password = "batataas";
//calculate your timezone in seconds, 1 hour = 3600 seconds and -3 UTC = -10800
const int timezoneinSeconds = -10800;
/***************************************************************/

/**********  Variables  ******************************/

int dst = 0;
uint16_t  h, m, s;
uint8_t dow; // qual a utilidade?
int  day; // qual a utilidade?
uint8_t month; // qual a utilidade?
String  year; // qual a utilidade?
// Global variables
char szTime[9];    // mm:ss\0
char szsecond[4];    // ss
char szMesg[MAX_MESG+1] = ""; // qual a utilidade?

String openWeatherMapApiKey = "56e9e9d9c9760da98287036625c00cca";

String city = "São Paulo";
String countryCode = "BR";

char temp_now[9]; 
char temp_max[9]; 
char temp_min[9]; 

/**********  Functions  ******************************/
// ************* Matrix functions

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

/**********  MAIN   ******************************/

void setup(void)
{
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

  Serial.println("");
  Serial.println("WiFi connected");
  //Serial.println("IP address: ");
  //Serial.println(WiFi.localIP());
  delay(3000);
  WiFi.mode(WIFI_STA);
  getForecast();
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
}

void loop(void)
{
  static uint32_t lastTimeMatrix = 0; // millis() memory matrix 
  static uint32_t lastTimeForecast = 0; // millis() memory forecast 
  static uint32_t lastTimeMode = 0; // millis() memory switch mode
  static uint8_t  display = 0;  // current display mode
  static bool flasher = false;  // seconds passing flasher

  P.displayAnimate();

  if (millis() - lastTimeForecast >= 300000) // 5 min 
  {
    getForecast();
    lastTimeForecast = millis();
  }

  if (millis() - lastTimeMode >= 20000) // 20 sec
  {
    if (mode == "timer") 
    {
        mode = "forecast";
    } 
    else 
    {
        mode = "timer";
    }
    lastTimeMode = millis();
  }

  if (millis() - lastTimeMatrix >= 1000) // 1 sec
  {
    if (mode == "timer") {
        lastTimeMatrix = millis();
        getsec(szsecond);
        getTime(szTime, flasher);
        flasher = !flasher;
    } else {
        lastTime = millis();
        getsec(szsecond);
        getTime(szTime, flasher);
        flasher = !flasher;
    }

    P.displayReset(0);
    P.displayReset(1);
  }
}
