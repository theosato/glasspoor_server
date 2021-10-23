#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <ESPAsyncWebServer.h>
#define PIN 26

const char* ssid     = "...";
const char* password = "tibico123";

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);


AsyncWebServer server(80);

void setup()
{
    Serial.begin(115200);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print("#");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    pixels.setBrightness(1);
    pixels.begin();
    server.on("/changeColor", HTTP_GET, [](AsyncWebServerRequest *request){
      int paramsNr = request->params();
      int red, green, blue;
      red = 0;
      green = 0;
      blue = 0;
      for(int i=0;i<paramsNr;i++){
        AsyncWebParameter* p = request->getParam(i);
        if (p->name() == "red"){
          red = atoi(p->value().c_str());
        }else if(p->name() == "green"){
          green = atoi(p->value().c_str());
        }else if(p->name() == "blue"){
          blue = atoi(p->value().c_str());
        }
      }

      for (int num = 0; num < 60; num ++){
        pixels.setPixelColor(num, pixels.Color(red, green, blue));
      }
      pixels.show();
      request->send(200, "text/plain", "message received");
    });
    server.begin();
}

void loop(){
}
