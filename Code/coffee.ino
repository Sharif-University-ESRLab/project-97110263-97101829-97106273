#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>


// constants are here:
#define RTCMEMORYSTART 65
#define RTC_MAGIC 12345678


typedef struct {
  uint32 magic ;
  char* ssid;
  char* pass;
} rtcStore;
rtcStore rtcMem;

void setupMem(){
  if (system_rtc_mem_read(RTCMEMORYSTART, &rtcMem, sizeof(rtcMem)) && rtcMem.magic == RTC_MAGIC){
    ssid = rtcMem.ssid;
    pass = rtcMem.pass;
    has_ssid_pass = true;
    //// TODO delete password
    Serial.printf("Read from mem %s %s %d %d\n", ssid, pass, strlen(ssid), strlen(pass));
    log("Read from mem");
    log(ssid);
  }
}

void setupMetrics() {
  // TODO: do things about coffee maker board
  // here we set input and outputs of the board
}


void setup() {
  // TODO: connect to wifi
  
  ESP.eraseConfig();
  Serial.begin(9600);
  delay(10);
  
  setupMetrics();
  setupMem();
  // TODO: connect to server
  Serial.println("Setup done");
}

void loop() {
  // TODO: client must get updates from server

  // TODO: codes about coffee maker parameters and actions must be done
}