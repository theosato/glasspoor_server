// Include libs
#include "Arduino.h"
#include <time.h>
#include <WiFi.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <MD_Parola.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

#include "matrix_font.h"

// 4 matrizes de LED, do tipo MAX7289
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
// Pins Ring Light
#define PIN 26
// Pins Utilizados do ESP32
#define CLK_PIN   18 // or SCK
#define DATA_PIN  19 // or MOSI
#define CS_PIN    5 // or SS
#define SPEED_TIME  75
#define PAUSE_TIME  0
#define MAX_MESG  20

// Biblioteca para o display
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
// Biblioteca para o ring
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);

// Define server
WebServer server(80);

// Define preferences
Preferences preferences;

// login e senha do Wi-Fi
char* ssid = "Sato 2g";
char* password = "81527765";

// Timezone em segundos, 1 hour = 3600 seconds and -3 UTC (BRT) = -10800
const int timezoneinSeconds = -10800;

uint16_t  h, m, s;
// Global variables
// variavel para o display
char displayAll[16];
// variaveis auxiliares
int readings[4];
String atemp = "";
String hum = "";
String maxtemp = "";
String mintemp = "";

// variaveis do openWeather
String openWeatherMapApiKey = "56e9e9d9c9760da98287036625c00cca";

// variaveis para a temperatura
char displayActualTemp[9];
char displayMaxTemp[9];
char displayMinTemp[9];
// variavel para humidade
char displayHumidity[9];

// controla quantidade de request
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

// display
uint8_t  display = 0;  // current display mode

/**********  Functions  ******************************/
// Função para retornar o horário
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
// função para retornar os segundos
void getsec(char *psz)
{
  sprintf(psz, "%02d", s);
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
    if ((millis() - lastTime) > timerDelay) {
        if(WiFi.status()== WL_CONNECTED){
            // Chama a API, faz a conversão do get em Json, separa o Json nas variaveís adequadas: temperatura atual, maxima, minima e humidade do ar
            // talvez pegar mais parametros depois, apenas seguir o modelo
            String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + preferences.getString("city", "") + ",BR"  + "&APPID=" + openWeatherMapApiKey + "&units=metric";
            String jsonBuffer = httpGETRequest(serverPath.c_str());
            Serial.println(jsonBuffer);

            DynamicJsonDocument forecast(512);
            deserializeJson(forecast, jsonBuffer);
            int temp = forecast["main"]["temp"];
            int humidity = forecast["main"]["humidity"];
            int max_temp = forecast["main"]["temp_max"];
            int min_temp = forecast["main"]["temp_min"];
            readings[0] = int(temp);
            readings[1] = int(humidity);
            readings[2] = int(max_temp);
            readings[3] = int(min_temp);
            atemp = "";
            atemp += readings[0];
            atemp += "C";
            hum = "";
            hum += readings[1];
            maxtemp = "";
            maxtemp += readings[2];
            maxtemp += "C";
            mintemp = "";
            mintemp += readings[3];
            mintemp += "C";
        //    Transforma por fim as leituras em char para serem exibidas no display, printa no terminal
            atemp.toCharArray(displayActualTemp, 9);
            hum.toCharArray(displayHumidity, 9);
            maxtemp.toCharArray(displayMaxTemp, 9);
            mintemp.toCharArray(displayMinTemp, 9);
            Serial.print("displayActualTemp: ");
            Serial.println(displayActualTemp);
            Serial.print("displayHumidity: ");
            Serial.println(displayHumidity);
            Serial.print("displayMaxTemp: ");
            Serial.println(displayMaxTemp);
            Serial.print("displayMinTemp: ");
            Serial.println(displayMinTemp);
        }
        else {
            Serial.println("WiFi Disconnected");
        }
        lastTime = millis();
   }
}


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
    lastTime = millis();
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
                for (int num = 0; num < 60; num ++){
                  pixels.setPixelColor(num, pixels.Color(postObj["color"]["red"], postObj["color"]["green"], postObj["color"]["blue"]));
                }
                pixels.show();
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


void setVoiceCommand() {
    String postBody = server.arg("plain"); 
    Serial.println(postBody);
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, postBody);
    if (error) {
        server.send(400, F("text/html"), "Error in parsin json body!");
    } else {
        JsonObject postObj = doc.as<JsonObject>();
        if (postObj.containsKey("voice")) {
            display = postObj["voice"];
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
    server.on(F("/voice"), HTTP_POST, setVoiceCommand);
}
 
// Manage not found URL
void handleNotFound() {
    DynamicJsonDocument doc(512);
    doc["message"] = "Not found";
    String buf;
    serializeJson(doc, buf);
    server.send(404, "application/json", buf);
}

/**********  MAIN   ******************************/
void setup(void)
{
  String inicio = "";
  preferences.begin("config", false);

  // baud rate
  Serial.begin(115200);
  delay(10);
  
  // conexão wifi
  WiFi.begin(ssid, password);

  // garantir conexão
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
	
  delay(2000);  

  WiFi.mode(WIFI_STA);
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
  pixels.setBrightness(1);
  pixels.begin();
  server.begin();
  Serial.println("HTTP server started");
  
  // retornar Timezone correta
  configTime(timezoneinSeconds, 0, "pool.ntp.org","time.nist.gov");

  P.begin(3);
  // inverter os leds acessos
  P.setInvert(false);

  // zona 0: matriz inteira
  P.setZone(0, 0, 3);
  // fonte da matriz numeric7Se no arquivo matrix_font.h
  P.setFont(0, numeric7Se);
  inicio = "GlassPoor";
  inicio.toCharArray(displayAll, 16);
  P.displayZoneText(0, displayAll, PA_CENTER, SPEED_TIME, 5, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  // intensidade do led
  P.setIntensity (0);
}

void loop(void)
{
  server.handleClient();

  static bool flasher = false;  // seconds passing flasher
  String tmp = "MAX: ";
  String tmp2 = "MIN: ";
  String tmp3 = "%";
  String tmp4 = "A: ";
  String tmp5 = "HUM: ";
  
  // display: 0 = timer / 1 = temp atual / 2 = temp max / 3 = temp min / 4 = umidade
  // aqui a ideia para fazer a troca: usar o switch (display). 
  // Receber no método loop ou no setup, não sei, a variavel display
  // e retirar o display++ dos cases
  P.displayAnimate();
  if (P.getZoneStatus(0))
  {
    Serial.println(display);
    switch (display)
    {
      case 0: // timer
        getTime(displayAll, flasher);
        flasher = !flasher;
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        
        break;

     case 1:
        tmp4 += atemp;
        tmp4.toCharArray(displayAll, 16);
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        
        break;
        
     case 2:
        getForecast();
        tmp += maxtemp;
        tmp.toCharArray(displayAll, 16);
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        
        break;     

     case 3:
        getForecast();
        tmp2 += mintemp;
        tmp2.toCharArray(displayAll, 16);
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        
        break;

     case 4:
        getForecast();
        tmp5 += hum;
        tmp5 += tmp3;
        tmp5.toCharArray(displayAll, 16);
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        
        break;

     default: 
        display = 0;
        
        break;
    }

    P.displayReset(0); 
  }
}
