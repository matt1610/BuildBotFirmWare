/* Create a WiFi access point and provide a web server on it. */

/* The BuildBot will initially start up in a Config mode (mode 0), it will create and AP which must be connected to
 *  Once connected, POST to the writeConfig method giving the device a wifi network to connect to and change the config setting to 1,
 *  if the config setting is 1 then the device will boot into working buildbot mode, connect to the wifi network and accept update http
 *  calls from the TeamCity Wep API: https://github.com/matt1610/TeamCityWebHooksApi
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include "EEPROMAnything.h"

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti WiFiMulti;


/* Set these to your desired credentials. */
const char *ssid = "BuildBot";
const char *password = "buildbot";
bool pushed = false;
int status = WL_IDLE_STATUS;

struct config_t
{
    int mode;
    char rid[32];
    char ssid[32];
    char pass[64];
} configuration;

struct UpdateModel
{
    int mode;
    char ProjectName[32];
    char EventName[32];
    char BranchName[32];
    boolean BuildSucceeded;
} updateModel;

ESP8266WebServer server(80);

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */

void httpResponse(String message) {
  server.sendHeader("Connection", "close");
//  server.sendHeader("Access-Control-Allow-Origin", "0.0.0.0");
  server.send(200, "application/json", "{\"success\" : \"true\", \"message\" : \""+message+"\"}");
}

void handleRoot() {
	httpResponse("Buildbot is on");
}

void yellow() {
  digitalWrite(D1, LOW);    
  digitalWrite(D2, LOW);
  digitalWrite(D7, HIGH);
  httpResponse("yellow");
  delay(1000);
}

void red() {
  digitalWrite(D1, LOW);  
  digitalWrite(D2, HIGH);
  digitalWrite(D7, LOW);
  httpResponse("red");
  delay(1000);
}

void blue() {
  digitalWrite(D1, HIGH);    
  digitalWrite(D2, LOW);
  digitalWrite(D7, LOW);
  httpResponse("blue");
  delay(1000);
}

void getId() {
//  httpResponse(ESP.getChipId());
}

void writeToDisk(String rid, String wifiNet, String wifiPass, int mode) {

  EEPROM.begin(512);
  delay(100);

  strncpy(configuration.rid, rid.c_str(), 32);
  strncpy(configuration.ssid, wifiNet.c_str(), 32);
  strncpy(configuration.pass, wifiPass.c_str(), 64);
  configuration.mode = mode;

  EEPROM_writeAnything(0, configuration);

  Serial.println("Object Values");
  Serial.println(configuration.rid);
  Serial.println(configuration.ssid);
  Serial.println(configuration.pass);

  EEPROM.end();
  httpResponse("ID written to Device");
  Serial.println("Data written to disk");
}

void readFromDisk() {
  Serial.println("Reading");
  EEPROM_readAnything(0, configuration);

  Serial.print("Val: ");
  Serial.println(configuration.ssid);
  
  httpResponse(configuration.ssid);
}

void HandleUpdate(UpdateModel updateModel) {
  if (strcmp (updateModel.EventName,"buildStarted") == 0) {
      Serial.println("Building Project");
  }
  if (updateModel.EventName == "finished") {
      Serial.println("Idle");
  }
  if (updateModel.EventName == "buildFinished") {
      Serial.println("Finished Building");
  }
}

void setup() {

  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D3, INPUT);
  
	delay(1000);
	Serial.begin(115200);

  EEPROM_readAnything(0, configuration);

  if (configuration.mode == NULL) {
    configuration.mode = 0; // Setup Mode
  }

  if (configuration.mode < 1) {
      Serial.println(configuration.ssid);
      Serial.print("Configuring access point...");
      WiFi.softAP(ssid, password);     
  } else {
      Serial.println("BuildBot Mode");
      WiFi.begin(configuration.ssid, configuration.pass);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/red", red);
  server.on("/yellow", yellow);
  server.on("/blue", blue);
  server.on("/getId", getId);
  
  server.on("/writeConfig", []() {
    String rId = server.arg("id");
    String wifiNet = server.arg("wifi");
    String wifiPass = server.arg("wipass");
    String mode = server.arg("mode");
    writeToDisk(rId, wifiNet, wifiPass, mode.toInt());
  });

  server.on("/Update", []() {
    StaticJsonBuffer<200> newBuffer;
    JsonObject& json = newBuffer.parseObject(server.arg("plain"));
    //json.printTo(Serial);

    strcpy(updateModel.ProjectName, json["ProjectName"]);
    strcpy(updateModel.EventName, json["EventName"]);
    //strcpy(updateModel.BuildSucceeded, json["BuildSucceeded"]);
    strcpy(updateModel.BranchName, json["BranchName"]);

    HandleUpdate(updateModel);
    httpResponse("It worked..");
  });
  
  server.on("/readId", readFromDisk);
    
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {

  int buttonState = digitalRead(D3);

  if (buttonState == 0 && !pushed) {
      pushed = true;
      Serial.println("Push");
      delay(1000);
      if (buttonState == 0) {
        Serial.println("Change config");
        configuration.mode = 0;
        EEPROM_writeAnything(0, configuration);
        delay(10);
      }
  }

  if (buttonState == 1) {
    pushed = false;  
  }

  server.handleClient();
}
