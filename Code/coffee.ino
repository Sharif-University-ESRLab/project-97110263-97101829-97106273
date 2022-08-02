#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>


// constants are here:
#define RTCMEMORYSTART 65
#define RTC_MAGIC 12345678

char* ssid;
char* pass;
bool has_ssid_pass = false;

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


void setupMem(){
  if (system_rtc_mem_read(RTCMEMORYSTART, &rtcMem, sizeof(rtcMem)) && rtcMem.magic == RTC_MAGIC){
    ssid = rtcMem.ssid;
    pass = rtcMem.pass;
    has_ssid_pass = true;
    //// TODO: delete password for security reasons
    Serial.printf("Read from mem %s %s %d %d\n", ssid, pass, strlen(ssid), strlen(pass));
  }
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
  log("setup wifi");
}

void setupServer(){
  server.on("/ping", HTTP_GET, []() {
    server.send(200, "text/plain", "PONG");
  });
  
  // TODO: other things about nodeMCU being server
  
  server.begin();
  Serial.println("Setup server done");
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
  // TODO: client must get updates from server

  // TODO: codes about coffee maker parameters and actions must be done
}