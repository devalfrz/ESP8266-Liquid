/*
TODO:
servo
responsive
Hookup
*/

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>

/* Constants */
#define LEVEL_PIN 16

#define SERVO_MAX 100
#define SERVO_MIN 30
#define SERVO_STEP 1
#define SERVO_STEP_TIME 10
#define SERVO_START_TIME 100
#define SERVO_STOP_TIME 1000

#define TEMP_BUFFER_SIZE 144 //3 days every 30 min
#define TEMP_PERIOD 30*60    // 30 minutes

const char *ssid = "Eileen";
const char *password = "You-are-far-too-young-and-clever";
uint32_t servo_cycle_time = (SERVO_MAX-SERVO_MIN)*SERVO_STEP*SERVO_STEP_TIME;
const float temp_x_interval = (380.0/TEMP_BUFFER_SIZE);


/* EEPROM vars */
uint32_t pump_period;
uint32_t pump_on;
uint32_t night_pump_on;
uint32_t air_pump_period;
uint32_t air_pump_on;
uint32_t feeder_period;
uint8_t  feeds;
uint8_t  day_time;

/* Runtime vars */
float temperature = 30.2;
uint32_t temp_= 0;
uint32_t pump;
uint32_t air_pump;
uint32_t feeder;
uint32_t feeder_on;
uint8_t  feeding = 0;
float temperature_buffer[TEMP_BUFFER_SIZE] = {20};
ESP8266WebServer server(80);
Ticker blinker;


uint32_t get_feeder_on(){
  return ((servo_cycle_time*feeds) + SERVO_START_TIME + SERVO_STOP_TIME);
}

void updateTemperatureBuffer(){
  for(int i = 1;i<TEMP_BUFFER_SIZE;i++){
    temperature_buffer[i-1] = temperature_buffer[i];
  }
  temperature_buffer[TEMP_BUFFER_SIZE-1] = temperature;
}

void eepromRead(void){
  uint8_t tmp;

  pump_period = EEPROM.read(0);
  pump_period <<= 8;
  pump_period |= EEPROM.read(1);
  pump_period <<= 8;
  pump_period |= EEPROM.read(2);
  pump_period <<= 8;
  pump_period |= EEPROM.read(3);

  pump_on = EEPROM.read(4);
  pump_on <<= 8;
  pump_on |= EEPROM.read(5);
  pump_on <<= 8;
  pump_on |= EEPROM.read(6);
  pump_on <<= 8;
  pump_on |= EEPROM.read(7);

  night_pump_on = EEPROM.read(8);
  night_pump_on <<= 8;
  night_pump_on |= EEPROM.read(9);
  night_pump_on <<= 8;
  night_pump_on |= EEPROM.read(10);
  night_pump_on <<= 8;
  night_pump_on |= EEPROM.read(11);

  air_pump_period = EEPROM.read(12);
  air_pump_period <<= 8;
  air_pump_period |= EEPROM.read(13);
  air_pump_period <<= 8;
  air_pump_period |= EEPROM.read(14);
  air_pump_period <<= 8;
  air_pump_period |= EEPROM.read(15);

  air_pump_on = EEPROM.read(16);
  air_pump_on <<= 8;
  air_pump_on |= EEPROM.read(17);
  air_pump_on <<= 8;
  air_pump_on |= EEPROM.read(18);
  air_pump_on <<= 8;
  air_pump_on |= EEPROM.read(19);

  feeder_period = EEPROM.read(20);
  feeder_period  <<= 8;
  feeder_period  |= EEPROM.read(21);
  feeder_period  <<= 8;
  feeder_period  |= EEPROM.read(22);
  feeder_period  <<= 8;
  feeder_period  |= EEPROM.read(23);

  feeds = EEPROM.read(24);

  feeder_on = (get_feeder_on()/1000) + 1;
  
  day_time = EEPROM.read(25);

  pump = pump_period;
  air_pump = air_pump_period;
  feeder = feeder_period;
}

void eepromWrite(void){
  uint32_t tmp = 0;

  EEPROM.write(0,uint8_t(pump_period>>24));
  EEPROM.write(1,uint8_t(pump_period>>16));
  EEPROM.write(2,uint8_t(pump_period>>8));
  EEPROM.write(3,uint8_t(pump_period));

  EEPROM.write(4,uint8_t(pump_on>>24));
  EEPROM.write(5,uint8_t(pump_on>>16));
  EEPROM.write(6,uint8_t(pump_on>>8));
  EEPROM.write(7,uint8_t(pump_on));

  EEPROM.write(8,uint8_t(night_pump_on>>24));
  EEPROM.write(9,uint8_t(night_pump_on>>16));
  EEPROM.write(10,uint8_t(night_pump_on>>8));
  EEPROM.write(11,uint8_t(night_pump_on));

  EEPROM.write(12,uint8_t(air_pump_period>>24));
  EEPROM.write(13,uint8_t(air_pump_period>>16));
  EEPROM.write(14,uint8_t(air_pump_period>>8));
  EEPROM.write(15,uint8_t(air_pump_period));

  EEPROM.write(16,uint8_t(air_pump_on>>24));
  EEPROM.write(17,uint8_t(air_pump_on>>16));
  EEPROM.write(18,uint8_t(air_pump_on>>8));
  EEPROM.write(19,uint8_t(air_pump_on));

  EEPROM.write(20,uint8_t(feeder_period>>24));
  EEPROM.write(21,uint8_t(feeder_period>>16));
  EEPROM.write(22,uint8_t(feeder_period>>8));
  EEPROM.write(23,uint8_t(feeder_period));
  
  EEPROM.write(24,feeds);
  EEPROM.write(25,day_time);

  EEPROM.commit();
}


void handleRoot() {
  
  for (uint8_t i = 0; i < server.args(); i++) {
    if(server.argName(i) == "pump_period"){
      pump_period = server.arg(i).toInt();
      if(pump>pump_period) pump = pump_period;
    }else if(server.argName(i) == "pump_on"){
      pump_on = server.arg(i).toInt();
    }else if(server.argName(i) == "night_pump_on"){
      night_pump_on = server.arg(i).toInt();
    }else if(server.argName(i) == "air_pump_period"){
      air_pump_period = server.arg(i).toInt();
      if(air_pump>air_pump_period) air_pump = air_pump_period;
    }else if(server.argName(i) == "air_pump_on"){
      air_pump_on = server.arg(i).toInt();
    }else if(server.argName(i) == "feeder_period"){
      feeder_period = server.arg(i).toInt();
      if(feeder>feeder_period) feeder = feeder_period;
    }else if(server.argName(i) == "feeds"){
      feeds = server.arg(i).toInt();
      feeder_on = (get_feeder_on()/1000) + 1;
    }else if(server.argName(i) == "day_time"){
      day_time = !day_time;
    }else if(server.argName(i) == "pump"){
      if(day_time)
        pump = (pump>pump_on)?pump_on:pump_period;
      else
        pump = (pump>night_pump_on)?night_pump_on:pump_period;
      delay(1000);
    }else if(server.argName(i) == "air_pump"){
      air_pump = (air_pump>air_pump_on)?air_pump_on:air_pump_period;
      delay(1000);
    }else if(server.argName(i) == "feeder"){
      feeder = (feeding)?feeder_period:feeder_on;
      feeding = !feeding;
    }
  }
  eepromWrite();

  uint32_t uptime = millis() / 1000;
  char temp[310];
  pinMode(LEVEL_PIN,INPUT_PULLUP);
  snprintf(temp, 310,
"{\
\"level\":%d,\
\"temperature\":%02.1f,\
\"day_time\":%d,\
\"pump_period\":%d,\
\"pump_on\":%d,\
\"night_pump_on\":%d,\
\"air_pump_period\":%d,\
\"air_pump_on\":%d,\
\"feeder_period\":%d,\
\"feeder_on\":%d,\
\"feeds\":%d,\
\"uptime\":%d,\
\"pump\":%d,\
\"air_pump\":%d,\
\"feeder\":%d\
}"
    ,digitalRead(LEVEL_PIN),
    temperature,
    day_time,
    pump_period,
    pump_on,
    night_pump_on,
    air_pump_period,
    air_pump_on,
    feeder_period,
    feeder_on,
    feeds,
    uptime,
    pump,
    air_pump,
    feeder);

  pinMode(LEVEL_PIN,INPUT);

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", temp);

}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(404, "text/plain", message);
}

uint8_t process_pump_status(){
  uint32_t p_on = (day_time)?pump_on:night_pump_on;
  if(pump){
    pump--;
    return pump<p_on;
  }
  pump = pump_period;
  return LOW;
}

uint8_t process_air_pump_status(){
  if(air_pump){
    air_pump--;
    return air_pump<air_pump_on;
  }
  air_pump = air_pump_period;
  return 0;
}

void processTemperature(){
  if(temp_){
    temp_--;
  }else{
    temp_ = TEMP_PERIOD;
    updateTemperatureBuffer();
  }
}

void timerISR(){
  process_pump_status();
  process_air_pump_status();
  if(feeder) feeder--;
  processTemperature();
}


void setup(void) {
  // Load data
  EEPROM.begin(512);
  eepromRead();
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN,HIGH);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  //Initialize Ticker every 0.5s
  blinker.attach(1, timerISR);

  server.on("/", handleRoot);
  server.on("/temperature.svg", drawGraph);

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void feed(void){
}

void loop(void) {
  server.handleClient();
  if(!feeding && feeder<feeder_on){
    feeding = 1;
  }
  if(feeding){
    digitalWrite(LED_BUILTIN,LOW);
    delay(get_feeder_on());
    digitalWrite(LED_BUILTIN,HIGH);
    feeder = feeder_period;
    feeding = 0;
  }
  delay(10);
}

void drawGraph() {
  String out = "";
  float x = 10;
  char temp[200];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" viewBox=\"0 0 400 150\">\n";
  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(255, 255, 255, 0.8)\" stroke-width=\"3\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<text x=\"10\" y=\"20\" fill=\"black\">Temperature</text>\n";
  out += "<g stroke=\"black\"><line x1=\"10\" y1=\"140\" x2=\"380\" y2=\"140\" stroke-width=\"1\" /></g>\n";
  out += "<g stroke=\"black\"><line x1=\"10\" y1=\"107.5\" x2=\"380\" y2=\"107.5\" stroke-width=\"1\" /></g>\n";
  out += "<g stroke=\"black\"><line x1=\"10\" y1=\"75\" x2=\"380\" y2=\"75\" stroke-width=\"1\" /></g>\n";
  out += "<g stroke=\"black\"><line x1=\"10\" y1=\"42.5\" x2=\"380\" y2=\"42.5\" stroke-width=\"1\" /></g>\n";
  out += "<g stroke=\"black\"><line x1=\"130\" y1=\"10\" x2=\"380\" y2=\"10\" stroke-width=\"1\" /></g>\n";
  out += "<g stroke=\"blue\">\n";
  float y = temperature_buffer[0];
  for (int i = 1; i < TEMP_BUFFER_SIZE; i++) {
    float y2 = temperature_buffer[i];
    sprintf(temp, "<line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" stroke-width=\"1\" />\n", x, 140 - (y*3.25), x + temp_x_interval, 140 - (y2*3.25));
    out += temp;
    y = y2;
    x += temp_x_interval;
  }
  out += "</g>\n</svg>\n";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "image/svg+xml", out);
}
