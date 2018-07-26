/*
 *  This sketch demonstrates how to use ESP8266 to control Xiaomi Yi Camera.
 *
 * @author  Rok Rodic alias GreenEyedExplorer alias Hacker007, www.rodic.si
 * @version 0.1
 * @since   2017-07-01
 * 
 */
//Find SSID containing "YDXJ_". Password "1234567890". Connect to 192.168.42.1:7878.
// * Get token:           {"msg_id":257,"token":x}
// * Take photo:          {"msg_id":769,"token":x}
// * Start record video:  {"msg_id":513,"token":x}
// * Stop record video:   {"msg_id":514,"token":x}
// Get live stream:     {"msg_id":259,"token":x,"param":"none_force"}
// Get bat stat:        {"msg_id":13,"token":x} -> {"rval":0,"msg_id":13,"type":"adapter","param":"100"}
// Get all settings:    {"msg_id":3,"token":x}
// Get setting choices: {"msg_id":9,"param":"video_resolution","token":x}
// Get single setting:  {"msg_id":1,"type":"video_resolution","token":x}
// Get SDCARD space:    {"msg_id":5,"type":"free","token":x}
// Set single setting:  {"msg_id":2,"type":"video_resolution","param":"1920x1080 60P 16:9","token":x}
// Activate log:        {"msg_id":2,"type":"save_log","param":"on","token":x} -> READ telnet x.x.x.x + tail -f /tmp/fuse_a/firmware.avtive.log
// VLC stream: rtsp://192.168.42.1:7878/live

#include "ESP8266WiFi.h"
#include "Bounce2.h"
 
WiFiClient client;
String YI_SSID;
const int buttonPin1 = 13;          // input pin for pushbutton
const int buttonPin2 = 12;          // input pin for pushbutton
Bounce debouncer1 = Bounce();
Bounce debouncer2 = Bounce();
bool RecON = false;

void searchCamera() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(10);
  int cnt = WiFi.scanNetworks();
  Serial.print("Networks: ");
  if (cnt > 0) {
    for (int i = 0; i < cnt; ++i) {
      Serial.print(WiFi.SSID(i) + ",");
      if (WiFi.SSID(i).startsWith("YDXJ_")) {
        YI_SSID = WiFi.SSID(i);
        break;
      }
    }
  }
  Serial.println();
}

void connectToCamera() {
  bool result = true;
  short retry = 30;
  const int jsonPort = 7878;
  char password[11] = "1234567890";
  char ssid[30];
  Serial.print("Con: ");
  YI_SSID.toCharArray(ssid, YI_SSID.length() + 1);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    if (retry == 0) {
      result = false;
      break;
    }
    delay(500);
    retry--;
  }
  Serial.print(" -> wifi con:");
  if (result == true) Serial.print("OK "); else Serial.print("XX ");

  if (!client.connect("192.168.42.1", jsonPort)) result = false;
  Serial.print(" IP con:");
  if (result == true) Serial.print("OK."); else Serial.print("XX.");
  Serial.println();
}

void setup() {
  //Only For debug
  Serial.begin(115200);
  Serial.println("STARTUP");
  searchCamera();
  connectToCamera();
  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
  debouncer1.attach(buttonPin1); debouncer1.interval(50);
  debouncer2.attach(buttonPin2); debouncer2.interval(50);
}
 
String requestToken() {
  String token;
  // This will send the request token msg to the server
  client.print("{\"msg_id\":257,\"token\":0}\n\r");
  //delay(1000);
  yield(); delay(250); yield(); delay(250); yield(); delay(250); yield(); delay(250);
  // Read all the lines of the reply from server and print them to Serial
  String response;
  while (client.available()) {
    char character = client.read();
    response.concat(character);
  }
  // Search token in to the stream
  int offset = response.lastIndexOf(':');
  if (offset != -1) {
    for (int i = offset + 1; i < response.length(); ++i) {
      if ((response.charAt(i) != ' ') && (response.charAt(i) != '}')) {
        token.concat(response.charAt(i));
      }
    }
  }

  return token;
}
 
void TakePhoto(String token) {
  if (RecON) {
    RecordOFF(token);
    //RecON = false;
    String token = requestToken();
    if (token.length() == 0) exit;
  }
  client.print("{\"msg_id\":769,\"token\":");
  client.print(token);
  client.print("}\n\r");
  Serial.print("Photo - Response: ");
  yield(); delay(250); yield(); delay(250); yield(); delay(250); yield(); delay(250);
  String response;
  while (client.available()) {
    char character = client.read();
    response.concat(character);
  }
  Serial.println(response);
  if (RecON) {
    String token = requestToken();
    if (token.length() == 0) exit;
    RecordON(token);
  }
}

void RecordON(String token) {
  client.print("{\"msg_id\":513,\"token\":");
  client.print(token);
  client.print("}\n\r");
  Serial.print("RecON - Response: ");
  yield(); delay(250); yield(); delay(250); yield(); delay(250); yield(); delay(250);
  String response;
  while (client.available()) {
    char character = client.read();
    response.concat(character);
  }
  Serial.println(response);
}

void RecordOFF(String token) {
  client.print("{\"msg_id\":514,\"token\":");
  client.print(token);
  client.print("}\n\r");
  Serial.print("RecOFF - Response: ");
  yield(); delay(250); yield(); delay(250); yield(); delay(250); yield(); delay(250);
  String response;
  while (client.available()) {
    char character = client.read();
    response.concat(character);
  }
  Serial.println(response);
}

void loop() {
  debouncer1.update();
  debouncer2.update(); 
  if (debouncer1.fell()) {
    String token = requestToken();
    if (token.length() != 0) {
      TakePhoto(token);
    }
  }
  if (debouncer2.fell()) {
    String token = requestToken();
    if (token.length() != 0) {
      if (RecON) {
        RecordOFF(token);
        RecON = false;
      } else {
        RecordON(token);
        RecON = true;
      }
    }
  }
  yield();
}

