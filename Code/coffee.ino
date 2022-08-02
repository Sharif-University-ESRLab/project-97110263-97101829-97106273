#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
extern "C" {
  #include "user_interface.h"
}


// constants are here:
#define RTCMEMORYSTART 65
#define RTC_MAGIC 12345678

char* ssid;
char* pass;
bool has_ssid_pass = false;
bool connected_to_wifi = false;

char* device_id;
char* access_token;

typedef struct {
  uint32 magic ;
  char* ssid;
  char* pass;
} rtcStore;
rtcStore rtcMem;

const char* ACCESS_POINT_SSID = "coffeetest";
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
ESP8266WebServer server(8080);

// this function is a simple util:
//// TODO: move function to another file for clean code
char* getCharArrayFromString(String str){
  int str_len = str.length() + 1; 
  char* chr = (char*) malloc(str_len);
  str.toCharArray(chr, str_len);
  return chr;
}

void setupMem(){
  if (system_rtc_mem_read(RTCMEMORYSTART, &rtcMem, sizeof(rtcMem)) && rtcMem.magic == RTC_MAGIC){
    ssid = rtcMem.ssid;
    pass = rtcMem.pass;
    has_ssid_pass = true;
    //// TODO: delete password for security reasons
    Serial.printf("Read from mem %s %s %d %d\n", ssid, pass, strlen(ssid), strlen(pass));
  }
}

void storeMem(){
    rtcMem.magic = RTC_MAGIC;
    rtcMem.ssid = ssid;
    rtcMem.pass = pass;
    system_rtc_mem_write(RTCMEMORYSTART, &rtcMem, sizeof(rtcMem));
}

void setupAP() {
  Serial.printf("AP name %s\n", ACCESS_POINT_SSID);
  Serial.print("Set config ");
  
  // WiFi.mode(WIFI_STA);
  Serial.println(WiFi.softAPConfig(local_ip, gateway, subnet) ? "Successful" : "Failed!");
  Serial.print("Setup AP ");
  Serial.println(WiFi.softAP(ACCESS_POINT_SSID) ? "Successful" : "Failed!");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address");
  Serial.println(IP);
  Serial.println("Setup wifi done");
  access_token = device_id = getCharArrayFromString(WiFi.macAddress());
  Serial.println("setup wifi");
}

void setupServer(){
//   server.on("/ping", HTTP_GET, []() {
//     server.send(200, "text/plain", "PONG");
//   });
  
  server.on("/connect", HTTP_POST, []() {
    if (server.hasArg("ssid") && server.hasArg("pass") && server.arg("ssid") != NULL && server.arg("pass") != NULL){
      ssid = getCharArrayFromString(server.arg("ssid"));
      pass = getCharArrayFromString(server.arg("pass"));
      Serial.println(ssid);
      char* data = (char*) malloc(1024);
      data[0] = '\0';
      strcat(data, "{\"device_id\":\"");
      strcat(data, device_id);
      strcat(data, "\",\"access_token\":\"");
      strcat(data, access_token);
      strcat(data, "\"}");
      Serial.println(data);
      storeMem();
      server.send(200, "text/plain", data);
      has_ssid_pass = true;
      free(data);
    } else {
      server.send(400, "text/plain", "Bad args");
    }
  });

  server.onNotFound([]() {
    server.send(404, "text/plain", "404: Not found");
  });

  server.begin();
  Serial.println("Setup server done");
}

bool connectToWifi() {
  Serial.printf("connecting to %s %s\n", ssid, pass);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(500);
    Serial.print(++i); Serial.print(' ');
    if (i == 30){
      Serial.println("Failed to connect");
      return false;
    }
    log("retry");
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP()); 
  // device_id = getCharArrayFromString(WiFi.macAddress());
  // access_token = device_id;
  Serial.printf("MacAddress: %s\n", device_id);
  Serial.println("connected to wifi");
  return true;
}

void setupMetrics() {
  // TODO: do things about coffee maker board
  // here we set input and outputs of the board
}


void setup() {
  ESP.eraseConfig();
  
  Serial.begin(9600);
  delay(10);
  
  setupMetrics();
  setupMem();
  
  setupAP();
  setupServer();
  Serial.println("Setup done");
}

void loop() {
  server.handleClient();
  client.loop();
  
  if (has_ssid_pass) {
    connected_to_wifi = connectToWifi();
    has_ssid_pass = false;
  }

  // TODO: client must get updates from server

  // TODO: codes about coffee maker parameters and actions must be done
}