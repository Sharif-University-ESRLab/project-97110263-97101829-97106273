#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
// extern "C" {
//   #include "user_interface.h"
// }


char* ssid;
char* pass;
bool has_ssid_pass = false;
bool connected_to_wifi = false;
bool connected_to_mqtt = false;

char* device_id;
char* access_token;

const char* ACCESS_POINT_SSID = "coffeetest";
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
ESP8266WebServer server(8080);

WiFiClient espClient;
PubSubClient client(espClient);
char* SUB_TOPIC = "cmd/";
char* sub_topic = (char*) malloc(128);
char* PUB_TOPIC = "signal/";
char* pub_topic = (char*) malloc(128);
char* MQTT_HOST = "94.101.176.204";
int MQTT_PORT = 1883;

// this function is a simple util:
//// TODO: move function to another file for clean code
char* getCharArrayFromString(String str){
  int str_len = str.length() + 1; 
  char* chr = (char*) malloc(str_len);
  str.toCharArray(chr, str_len);
  return chr;
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

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.printf("Call back %s %s \n", topic, payload);
  DynamicJsonDocument data(length + 10);
  DeserializationError err = deserializeJson(data, payload);
  if (err) {
    Serial.print(("deserializeJson() failed: "));
    Serial.println(err.c_str());

    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (int i = 0; i < length; i++) {
      Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
  }
  log("callback");

  //// TODO: works about coffee maker data must be written here
  //// also here we must rewrite data to pins and update them using rewrite function()
  return;
}

bool connectToMqtt() {
  Serial.printf("Connecting to mqtt %s:%d\n", MQTT_HOST, MQTT_PORT);
  client.setServer(MQTT_HOST, MQTT_PORT) ;
  client.setCallback(callback);
  if (!client.connect(device_id, device_id, access_token)) {
    Serial.println("Failed to connect");
    delay(1000);
    return false;
  }
  sub_topic[0] = '\0';
  strcat(sub_topic, SUB_TOPIC);
  strcat(sub_topic, device_id);
  pub_topic[0] = '\0';
  strcat(pub_topic, PUB_TOPIC);
  strcat(pub_topic, device_id);
  Serial.printf("Subscribing on %s\n", sub_topic);
  Serial.printf("Publishing on %s\n", pub_topic);
  client.subscribe(sub_topic);
  Serial.println("Connected to mqtt");
  log("connected to mqtt");
  return true;
}

void setupMetrics() {
  //// TODO: do things about coffee maker board
  // here we set input and outputs of the board
}

void rewrite(){
  Serial.println("Rewrite keys");
  //// TOOD: here we write data (that we get from MQTT server) to digital output pins
}

void setup() {
  ESP.eraseConfig();
  
  Serial.begin(9600);
  delay(10);
  
  setupMetrics();
  
  setupAP();
  setupServer();
  Serial.println("Setup done");
}

void loop() {
  server.handleClient();
  client.loop();
  
  // connect nodeMCU to WiFi
  if (has_ssid_pass) {
    connected_to_wifi = connectToWifi();
    has_ssid_pass = false;
  }

  if (connected_to_wifi && !connected_to_mqtt) {
    connected_to_mqtt = connectToMqtt();
  }
  if (!client.connected() && connected_to_mqtt ) {
    Serial.println("mqtt disconnected");
    connected_to_mqtt = false;
  }

  //// TODO: codes about coffee maker parameters and actions must be done
  //// TODO: send data (metrics) about coffe maker to server
}