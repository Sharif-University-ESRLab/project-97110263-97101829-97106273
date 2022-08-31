#include <Servo.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
// #include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <AsyncElegantOTA.h>;
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

WiFiClient espClient;
PubSubClient client(espClient);
char* device_id;
char* access_token;
char* SUB_TOPIC = "cmd/";
char* sub_topic = (char*) malloc(128);
char* PUB_TOPIC = "signal/";
char* pub_topic = (char*) malloc(128);
char* MQTT_HOST = "94.101.176.204";
int MQTT_PORT = 1883;

char* ssid;
char* pass;
bool has_ssid_pass = false;
bool has_ssid_pass_really = false;
bool connected_to_wifi = false;;
bool connected_to_mqtt = false;

const char* ACCESS_POINT_SSID = "coffe_maker";
const char* ACCESS_POINT_PASS = "testtest";
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
AsyncWebServer server(8080);


Servo myservo;  // create servo object to control a servo

// TODO: change pins for nodeMCU: consider that "servoDataPin" is PWM pulse
// optional TODO: we don't need startPin: we read from the application

int closeDegree = 140;          // variable to store the servo position for closing
int openDegree = 60;            // variable to store the servo position for opening
int armaturePin = D0;           // variable to set the pin of HIGH and LOW voltage for turn on and off armature
int waterPin = D1;              // variable to set the pin of HIGH and LOW voltage for open and close water tank
int servoDataPin = D2;          // variable to set the pin of data/degree for servo
int coffeePowerPin = D3;        // variable to set the pin of HIGH and LOW voltage for turn on and off coffee power
int startPin = D4;              // variable to read the pin for starting coffee process

// constants:
int coffeeLevel = 1;            // 1 to 3
int waterLevel = 1;             // 1 to 2
int coffeeLevelUpUnit = 3000;   // for each level up
int waterLevelUpUnit = 10000;   // for each level up
int coffeeInitTime = 5000;      // first level time for coffee
int waterInitTime = 18000;      // first level time for water
int coffeeTurnOnTime = 120000;   // time (ms) that coffee maker must be on and boiling


long startTime = 0;
long drainCoffeeStartTime = 0;
long drainWaterStartTime = 0;
long lastMetricSent = 0;

int stage = 0;
/*
   stage 0 -> off
   stage 1 -> should open coffee
   stage 2 -> coffee is being poured

   stage 3 -> water should open
   stage 4 -> water is being poured

   stage 5 -> brewing must start
   stage 6 -> brewing is being done
*/

void setCoffeePower(bool isOn) {
  // 1 for on and 0 for off
  if (isOn) {
    digitalWrite(coffeePowerPin, LOW);
  } else {
    digitalWrite(coffeePowerPin, HIGH);
  }
}

void setCoffeeTank(bool isOpen) {
  // 1 for open and 0 for close
  if (isOpen) {
    myservo.write(openDegree);
  } else {
    myservo.write(closeDegree);
  }
}

void setWaterTank(bool isOpen) {
  // 1 for open and 0 for close
  if (isOpen) {
    digitalWrite(waterPin, LOW);
  } else {
    digitalWrite(waterPin, HIGH);
  }
}

void setArmature(bool isOn) {
  // 1 for on and 0 for off
  if (isOn) {
    digitalWrite(armaturePin, LOW);
  } else {
    digitalWrite(armaturePin, HIGH);
  }
}

void drainCoffee() {
  ///////////////////##########//////////////////////
  int coffeeOpenTime = coffeeInitTime + (coffeeLevel - 1) * coffeeLevelUpUnit;

  if (stage == 1) {
    setCoffeeTank(true);      // Open the coffee tank
    delay(100);               // Small wait
    setArmature(true);        // Turn on armature
    drainCoffeeStartTime = millis();
    stage = 2;
  } else if (stage == 2) {
    if (millis() - drainCoffeeStartTime >= coffeeOpenTime) {
      stage = 3;
      setArmature(false);       // Turn off armature
      delay(100);               // Small wait
      setCoffeeTank(false);     // Close the coffee tank
    } else {
      delay(100);
    }
  }


  ///////////////////##########//////////////////////
}

void drainWater() {
  int waterOpenTime = waterInitTime + (waterLevel - 1) * waterLevelUpUnit;
  if (stage == 3) {
    setWaterTank(true);       // Open water tank
    drainWaterStartTime = millis();
    stage = 4;
  } else if (stage == 4) {
    if (millis() - drainWaterStartTime >= waterOpenTime) {
      setWaterTank(false);      // Close water tank
      stage = 5;
    }
  }
}


void setup() {
  ESP.eraseConfig();
  Serial.begin(9600);
  delay(10);
  Serial.println("V1.0");
  setupAP();
  setupServer();
  myservo.attach(servoDataPin, 500, 2400);  // attaches the servo on pin 9 to the servo object
  pinMode(armaturePin, OUTPUT);
  pinMode(waterPin, OUTPUT);
  pinMode(coffeePowerPin, OUTPUT);
  pinMode(startPin, INPUT_PULLUP); // INPUT_PULLUP is HIGH in default so it is active LOW
  setCoffeeTank(false);
  setArmature(false);
  setWaterTank(false);
  setCoffeePower(false);

  Serial.println("Setup done");
}


void setupAP() {
  Serial.printf("AP name %s\n", ACCESS_POINT_SSID);
  Serial.print("Set config ");
  Serial.println(WiFi.softAPConfig(local_ip, gateway, subnet) ? "Successful" : "Failed!");
  Serial.print("Setup AP ");
  Serial.println(WiFi.softAP(ACCESS_POINT_SSID, ACCESS_POINT_PASS) ? "Successful" : "Failed!");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address");
  Serial.println(IP);
  Serial.println("Setup wifi done");
  Serial.println(getCharArrayFromString(WiFi.macAddress()));
  Serial.println();
  //  access_token = device_id = "A4:CF:12:F0:00:B3";
  access_token = device_id = getCharArrayFromString(WiFi.macAddress());
}


bool connectToWifi() {
  Serial.printf("connecting to %s %s\n", ssid, pass);
  // if (!WiFi.config(local_ip, gateway, subnet)){Serial.println("STA Failed to configure");}
  WiFi.begin(ssid, pass);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(500);
    //    Serial.print(++i); Serial.print(' ');
    if (i == 20) {
      Serial.println("Failed to connect");
      WiFi.disconnect();
      has_ssid_pass = has_ssid_pass_really;
      return false;
    }
  }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  device_id = getCharArrayFromString(WiFi.macAddress());
  //  device_id = "A4:CF:12:F0:00:B3";
  access_token = device_id;
  Serial.printf("MacAddress: %s\n", device_id);
  return true;
}


void callback(char *topic, byte *payload, unsigned int length) {
  Serial.printf("Call back %s %s \n", topic, payload);
  DynamicJsonDocument data(length + 10);
  DeserializationError err = deserializeJson(data, payload);
  if (err) {
    //    Serial.print(("deserializeJson() failed: "));
    //    Serial.println(err.c_str());

    //    Serial.print("Message arrived in topic: ");
    //    Serial.println(topic);
    //    Serial.print("Message:");
    for (int i = 0; i < length; i++) {
      //      Serial.print((char) payload[i]);
    }
    //    Serial.println();
    //    Serial.println("-----------------------");
  }
  if (data["signals"].containsKey("power")) {
    int power = data["signals"]["power"]["value"];
    if (stage == 0 && power == 1) {
      stage = 1;
    } else if (power == 0) {
      stage = 0;
    }
    //      Serial.print("power level: ");
    //      Serial.println(power);
  } else if (data["signals"].containsKey("coffee")) {
    coffeeLevel = data["signals"]["coffee"]["value"];
    //      Serial.print("coffee level: ");
    //      Serial.println(coffeeLevel);

  } else if (data["signals"].containsKey("water")) {
    waterLevel = data["signals"]["water"]["value"];
    //      Serial.print("water level: ");
    //      Serial.println(waterLevel);
  }

  return;
}

bool connectToMqtt() {
  //  Serial.printf("Connecting to mqtt %s:%d\n", MQTT_HOST, MQTT_PORT);
  client.setServer(MQTT_HOST, MQTT_PORT) ;
  client.setCallback(callback);
  if (!client.connect(device_id, device_id, access_token)) {
    Serial.println("Failed to connect to mqtt!");
    delay(1000);
    return false;
  }
  sub_topic[0] = '\0';
  strcat(sub_topic, SUB_TOPIC);
  strcat(sub_topic, device_id);
  pub_topic[0] = '\0';
  strcat(pub_topic, PUB_TOPIC);
  strcat(pub_topic, device_id);
  client.subscribe(sub_topic);
  //  Serial.println("Connected to mqtt");
  return true;
}

DynamicJsonDocument getMetrics() {
  DynamicJsonDocument metrics(1024);
  metrics["power"] = stage == 0 ? 0 : 1;
  metrics["coffee"] = coffeeLevel;
  metrics["water"] = waterLevel;
  return metrics;
}


void sendMetrics() {
  DynamicJsonDocument doc(2048);
  doc["id"] = device_id;
  doc["metrics"] = getMetrics();
  char Buf[2048];
  serializeJson(doc, Buf);
  client.publish(PUB_TOPIC, Buf);
  Serial.println("Sent metrics: coffee: ");
  delay(100);
  //  Serial.print(coffeeLevel);
  //  Serial.print(",  water: ");
  //  Serial.print(waterLevel);
  //  Serial.print(" and power: ");
  //  Serial.println(stage > 0);
}

char* getCharArrayFromString(String str) {
  int str_len = str.length() + 1;
  char* chr = (char*) malloc(str_len);
  str.toCharArray(chr, str_len);
  return chr;
}


void setupServer() {
  server.on("/ping", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", "PONG");
  });

  server.on("/connect", HTTP_POST, [](AsyncWebServerRequest * request) {
    if (request->hasArg("ssid") && request->hasArg("pass") && request->arg("ssid") != NULL && request->arg("pass") != NULL) {
      ssid = getCharArrayFromString(request->arg("ssid"));
      pass = getCharArrayFromString(request->arg("pass"));
      char* data = (char*) malloc(1024);
      data[0] = '\0';
      strcat(data, "{\"device_id\":\"");
      strcat(data, device_id);
      strcat(data, "\",\"access_token\":\"");
      strcat(data, access_token);
      strcat(data, "\"}");
      request->send(200, "text/plain", data);
      has_ssid_pass = true;
      connected_to_wifi = false;
      has_ssid_pass_really = true;
      free(data);
    } else {
      request->send(400, "text/plain", "Bad args");
    }
  });
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest * request) {
    DynamicJsonDocument doc(2048);
    doc["last_ssid"] = ssid;
    doc["connected_to_wifi"] = connected_to_wifi;
    doc["connected_to_mqtt"] = connected_to_mqtt;
    doc["wifi_status"] = WiFi.status();
    char Buf[2048];
    serializeJson(doc, Buf);
    request->send(200, "text/plain", Buf);
  });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", "Hi! I am ESP8266.");
  });
  server.onNotFound([](AsyncWebServerRequest * request) {
    request->send(404, "text/plain", "404: Not found");
  });

  AsyncElegantOTA.begin(&server);

  server.begin();
  Serial.println("Setup server done");
}




void loop() {
  client.loop();

  if (has_ssid_pass) {
    connected_to_wifi = connectToWifi();
    has_ssid_pass = false;
  }
  if (connected_to_wifi && !connected_to_mqtt) {
    Serial.println("Connecting to mqtt");
    connected_to_mqtt = connectToMqtt();
  }
  if (!client.connected() && connected_to_mqtt ) {
    Serial.println("mqtt disconnected");
    connected_to_mqtt = false;
  }

  if (millis() - lastMetricSent > 3000) {

    if (connected_to_mqtt)
      sendMetrics();
    lastMetricSent = millis();
  }

  // state machine
  if (stage > 0) {

    switch (stage) {
      case 1:
      case 2: // draining cofee
        drainCoffee();
        break;

      case 3:
      case 4:  // draining water
        drainWater();
        break;

      case 5: // start brewing
        setCoffeePower(true);
        startTime = millis();
        stage = 6;
        break;

      case 6: // brewing
        if (millis() - startTime >= coffeeTurnOnTime) {
          setCoffeePower(false);
          stage = 0;
        }
        break;
    }
  } else {
    setCoffeeTank(false);
    setArmature(false);
    setWaterTank(false);
    setCoffeePower(false);
  }

}
