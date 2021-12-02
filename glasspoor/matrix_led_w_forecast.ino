#include <WiFi.h>
#include <time.h>
#include <MD_Parola.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

#include "matrix_font.h"

// 4 matrizes de LED, do tipo MAX7289
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
// Pins Utilizados do ESP32
#define CLK_PIN   18 // or SCK
#define DATA_PIN  19 // or MOSI
#define CS_PIN    5 // or SS
#define SPEED_TIME  75
#define PAUSE_TIME  0
#define MAX_MESG  20

// Biblioteca para o display
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// login e senha do Wi-Fi
char* ssid = "AndroidAP";
char* password = "batataas";
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
String city = "São Paulo";
String countryCode = "BR";

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
   if(WiFi.status()== WL_CONNECTED){
      // Chama a API, faz a conversão do get em Json, separa o Json nas variaveís adequadas: temperatura atual, maxima, minima e humidade do ar
      // talvez pegar mais parametros depois, apenas seguir o modelo
      String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey + "&units=metric";
      String jsonBuffer = httpGETRequest(serverPath.c_str());
      Serial.println(jsonBuffer);
      JSONVar myObject = JSON.parse(jsonBuffer);
      Serial.print("JSON object = ");
      Serial.println(myObject);
      JSONVar main = myObject["main"];
      JSONVar temp = main["temp"];
      JSONVar humidity = main["humidity"];
      JSONVar max_temp = main["temp_max"];
      JSONVar min_temp = main["temp_min"];
      // transforma as leituras do Json em int, depois String e depois Char, adiciona C em temperatura
      // adicionar % em humidade não funcionou atm, tem que alterar o Font_data prob
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
      // Transforma por fim as leituras em char para serem exibidas no display, printa no terminal
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
}

/**********  MAIN   ******************************/
void setup(void)
{
  String inicio = "";
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
	
  delay(3000);  
  // não sei
  WiFi.mode(WIFI_STA);

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
  static uint8_t  display = 0;  // current display mode
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

  // display das Animações
  P.displayAnimate();
  // sinceramente não sei pra que serve esse if, mas imagino que seja para checar a zona escolhida para animação
  if (P.getZoneStatus(0))
  {
    Serial.println(display);
    switch (display)
    {
      case 0: // timer
      
        getTime(displayAll, flasher);
        flasher = !flasher;
        // aqui é a animação: zona, animação de entrada, animação de saída
        // dá para mudar e brincar um pouco mas eu fiquei com preguiça.
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        display++;      
        
        break;

     case 1:
        getForecast();
        tmp4 += atemp;
        // variaveis temporarias para apresentar melhor no display
        tmp4.toCharArray(displayAll, 16);
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        display++;
        
        break;
        
     case 2:
        getForecast();
        tmp += maxtemp;
         // variaveis temporarias para apresentar melhor no display
        tmp.toCharArray(displayAll, 16);
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        display++;
        
        break;     

     case 3:
        getForecast();
        tmp2 += mintemp;
         // variaveis temporarias para apresentar melhor no display
        tmp2.toCharArray(displayAll, 16);
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        display++;
        
        break;

     case 4:
        getForecast();
        tmp5 += hum;
        tmp5 += tmp3;
         // variaveis temporarias para apresentar melhor no display
        tmp5.toCharArray(displayAll, 16);
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        display++;
        
        break;

     default: 
        display = 0;
        // tentei alterar o default mas não deu certo, deixei assim.
        
        break;
    }

    P.displayReset(0); 
  }

}
