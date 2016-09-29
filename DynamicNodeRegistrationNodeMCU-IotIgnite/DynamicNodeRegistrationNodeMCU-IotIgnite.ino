/**
   Dynamic Node Registration Example Arduino Sketch
*/

// includes
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <Timer.h>

#include "DHT.h"
#include "LogHandler.h"
#include "ConfigurationFileUtils.h"


/**
   @brief mDNS and OTA Constants
   @{
*/
#define HOSTNAME "IotIgnite-ESP8266-" ///< Hostename. The setup function adds the Chip ID at the end.
#define PORT 9999
#define DHTPIN D6 // D1 Pin of NodeMCU a.k.a GPIO 5
#define DHTTYPE DHT11   // DHT 11
#define HEARTBEAT_INTERVAL 5000
#define BUILTIN_LED D0  // NodeMCU BuiltinLED a.k.a GPIO 16

#define CONNECT_TIMEOUT 10

#define LOG_PORT 12734


#define GLOBAL_ESP_RESET_TIMEOUT 120000
#define CONNECT_TRYOUT_TIME 5000
// LED BLINKS
#define BLINK_RAPIDLY 5 // Everything is OK situation. Connected to configured wifi also client is connected too.
#define BLINK_IN_TRY_TO_CONNECT 250 // Trying to connect configured wifi.
#define BLINK_IN_AP_MODE_WAITING_CLIENTS 500 // AP mode enabled watinig for client to connect.
#define BLINK_IN_AP_MODE_CLIENT_CONNECTED 1000 // AP mode enabled,client connected and waiting for configurations.
#define BLINK_IN_STATION_MODE_CONNECTED_TO_WIFI 3000 // Connected to configured wifi, waiting for client.

#define LED_PIN D2

#define RESET_BUTTON D4 // RESET Configuration button for interrupt

long resetStateTime=0;

unsigned long readingFreq = 10000;


DHT dht(DHTPIN, DHTTYPE);
Timer myDHTTimer;
Timer heartbeatTimer;
Timer espGlobalResetTimer;
Timer espLogTimer;
int myHeartbeatTimerInt;
int myDHTTimerInt;
int espGlobalResetTimerInt;
int espLogTimerInt;
int connectCounter = 0;
unsigned int discovery_ttl = 120;
String configuredNodeID = ".";
String gatewayID = "";
String wifiTryCount = "";
String tmpRegistrationStatus = "";
String station_ssid = "";
String station_psk = "";
String last_station_ip = "";

bool isFirstConnectionAP = true;

bool isFirstClientConnectionSTA = true;

int infoLogCounter = 0;


WiFiServer server(PORT);
WiFiClient client;
WiFiClient log_client;

IPAddress last_connected_ip;

String getMacAddress();
void doAPModeOperations();
bool handleConfigurationResponse();
void handleDataResponse();
void heartbeatGenerator();
void readAllSensors();
void startZeroConfService();
String packetBuilder(String fahrenheit, String celcius, String humidity);
void updateTimers();
void sendNodeIDToGateway();
void resetConfigurations();
void restartESP();
//void resetGlobalEspTimer();
void parseLastConnectedIPString();
void writeLogToFileAndSerialBus(int type, String code, String msg);
void tryToConnectLastConnectedHost();
void sendLogClientMessageIfAvailable(String msg);


void connectWifi(){
  if (station_ssid != "" && station_psk != "") {
    // ... Try to connect to WiFi station.
    String msg = "Wifi config changed ! new SSID : " + WiFi.SSID() + " - new NodeID :" + configuredNodeID + " - new GatewayId " + gatewayID;
    writeLogToFileAndSerialBus(INFO_LOG, "101", msg);
    WiFi.begin(station_ssid.c_str(), station_psk.c_str());
  }
  else {
    String msg = "No change beginning with SDK config : " + WiFi.SSID() + " - NodeID :" + configuredNodeID + " - new GatewayId " + gatewayID;
    writeLogToFileAndSerialBus(INFO_LOG, "102", msg);
    WiFi.begin();
  }
}

void setup()
{
  int wifiStatus;
  
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(115200);

  delay(100);

  Serial.println("\r\n");
  Serial.print("Chip ID: 0x");
  Serial.println(ESP.getChipId(), HEX);


  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);

  // Reset interrupt config
  pinMode(RESET_BUTTON, INPUT);
  attachInterrupt(RESET_BUTTON, resetOn, RISING);


  // Set Hostname.
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);

  // Print hostname.
  Serial.println("Hostname: " + hostname);
  //Serial.println(WiFi.hostname());


  // Initialize file system.
  if (!startFileSystem())
  {
    Serial.println("Failed to mount file system");
    return;
  }

  //resetConfigurations();
  // Load wifi connection information.
  if (! loadConfig(&station_ssid, &station_psk))
  {
    station_ssid = "";
    station_psk = "";

    writeLogToFileAndSerialBus(INFO_LOG, "100", "No WiFi connection information available in config file");
  }




  espLogTimerInt = espLogTimer.every(CONNECT_TRYOUT_TIME, tryToConnectLastConnectedHost);
  // Check WiFi connection
  // ... check mode
  if (WiFi.getMode() != WIFI_STA) {
    WiFi.mode(WIFI_STA);
    delay(1000);
  }

//  resetGlobalEspTimer();
  loadNodeID(&configuredNodeID);
  loadGatewayID(&gatewayID);
  loadWifiTryCount(&wifiTryCount);
  Serial.println("WiFi.SSID() " + WiFi.SSID() + " station_ssid=" + station_ssid);
  Serial.println("WiFi.psk() " + WiFi.psk() + " station_psk=" + station_psk);
  //Serial.println("WiFi.localIP() " + WiFi.localIP());
  // ... Compare file config with sdk config.
  connectWifi();

  Serial.println("WiFi.localIP() " + WiFi.localIP());

  writeLogToFileAndSerialBus(INFO_LOG, "103", "Wait for WiFi connection...");

  // ... Give ESP 10 seconds to connect to station.
  unsigned long startTime = millis();
  Serial.println("Waiting wifi connection ");
  int tmpWifiTryCount = wifiTryCount.toInt();
  Serial.println(tmpWifiTryCount);
  if(station_ssid != "" && station_psk != ""){
    Serial.println("ssid is empty");
    do {
      wifiStatus = WiFi.status();
      Serial.println(wifiStatus);
      //Serial.write('.');        
      if(wifiStatus != WL_IDLE_STATUS && wifiStatus != WL_CONNECTED){
        Serial.println("Connection failed, we will try again.");
        connectWifi();
      }
      blinkBuiltinLed(BLINK_IN_TRY_TO_CONNECT);
      delay(500);
  
    } while ((wifiStatus != WL_CONNECTED) && ((millis() - startTime) < 10000));

    tmpWifiTryCount = tmpWifiTryCount + 1;
    String tmpValue = String(tmpWifiTryCount);
    saveWifiTryCount(&tmpValue);
    if(WiFi.status() != WL_CONNECTED && tmpWifiTryCount < 10){
      Serial.println("Restart esp");
      Serial.println(tmpWifiTryCount);
      ESP.restart();
    }else{
      Serial.println("not restart esp");
      Serial.println(tmpWifiTryCount);
      wifiTryCount = "0";
      saveWifiTryCount(&wifiTryCount);
    }
  }
  
  

  // Check connection
  if (WiFi.status() == WL_CONNECTED) {
    // ... print IP Address
    writeLogToFileAndSerialBus(INFO_LOG, "104", "Wifi Connected! IP Address : " + WiFi.localIP());
    startZeroConfService();
  } else {
    String registrationCompleted = "";
    loadRegistrationStatus(&registrationCompleted);
    if(registrationCompleted == "OK"){
      Serial.println("Can not connect to wifi however we have configured, nothing we could do in here");
    }else{
      writeLogToFileAndSerialBus(ERROR_LOG, "100", "Can not connect to WiFi station. Go into AP mode...");
      // Go into software AP mode.
      WiFi.mode(WIFI_AP);
  
      delay(1000);
      int bufflength = hostname.length() + 1;
      char charSSIDBuffer[bufflength];
      hostname.toCharArray(charSSIDBuffer, bufflength);
      WiFi.softAP(charSSIDBuffer);
  
      // Serial.print("IP address: ");
      // Serial.println(WiFi.softAPIP());
  
      writeLogToFileAndSerialBus(INFO_LOG, "105", "AP Mode Started. IP address :  " + WiFi.softAPIP());
    }
  }

  server.begin();
  server.setNoDelay(true);

  dht.begin();

}

/**
   @brief Arduino loop function.
*/

String response;

void loop()
{
  if (WiFi.getMode() == WIFI_AP) {
    Serial.println("AP mode is running");
    doAPModeOperations();
  } else {
    Serial.println("Node mode is running");
    doNodeOperations();
  } // end of WIFI_STATION_MODE
  delay(10);
}

String getMacAddress() {
  byte mac[6];
  WiFi.macAddress(mac);
  String cMac = "";
  for (int i = 0; i < 6; ++i) {
    cMac += String(mac[i], HEX);
    if (i < 5)
      cMac += ":";

  }
  cMac.toUpperCase();
  return cMac;
}

void doNodeOperations() {
  if (isFirstClientConnectionSTA) {
    Serial.println("Connected to wifi network.!!!!!!!");
    Serial.println("SSID name is " +  WiFi.SSID());
    sendLogClientMessageIfAvailable("Connected to wifi network.!!!!!!!");
  }
  espLogTimer.update();
  if (isFirstClientConnectionSTA) {
    //espGlobalResetTimer.update();
  }
  unsigned long startTime = millis();
  while (true) {
    if (tmpRegistrationStatus != "OK") {
      Serial.println("Waiting connection from client");
      client = server.available();
    }
    if (client.connected()) {
      if (isFirstClientConnectionSTA) {
        writeLogToFileAndSerialBus(INFO_LOG, "109", "Gateway application connected.");
      }
      isFirstClientConnectionSTA = false;
//      resetGlobalEspTimer();
      blinkBuiltinLed(BLINK_RAPIDLY);
      //Serial.println("*** waitGateway starting");
      waitGateway();
    } else {
      tmpRegistrationStatus = "";
      String registrationCompleted = "";
      loadRegistrationStatus(&registrationCompleted);
      
      if(registrationCompleted != "OK"){
        if (millis() - startTime > 1000000){
          writeLogToFileAndSerialBus(ERROR_LOG, "131", "Gateway application could not connected in expected time. ");
          Serial.println("Configuration removed");
          resetConfigurations();
        }
      }
      sendLogClientMessageIfAvailable("Gateway application not connected! (STA)");
      writeLogToFileAndSerialBus(INFO_LOG, "119", "Gateway application not connected! (STA)");
      blinkBuiltinLed(BLINK_IN_STATION_MODE_CONNECTED_TO_WIFI);
    }
  }
}

void waitGateway(){

  //Serial.println("*** reat start");
  while (client.available()) {
    char c = client.read();
    response += c;
  }
  client.flush();
  //Serial.println("*** read end");
  if (tmpRegistrationStatus == "OK") {
    if (!response.equals("")){
      Serial.println("*** Response is not empty");
      handleDataResponse();
      //Serial.println("Timer update start");
      
      //Serial.println("Update stop");
    }

      myDHTTimer.update();
      heartbeatTimer.update();
  } else {
    if (response.equals("")){
      Serial.println("Response is empty");
    } else if (response.charAt(0) == '~') {
      String connectedGatewayId = response.substring(1, response.length());
      Serial.println("connected gateway " + connectedGatewayId);
      String compare = gatewayID + "\n";
      if (compare != connectedGatewayId) {
        String message = "@Gateway rejected "  + compare;
        Serial.println(message);
        client.write((uint8_t*) &message[0], message.length());
        client.stop();
      } else {
        Serial.println("Gateway accepted " + gatewayID);
        updateTimers();
        writeLogToFileAndSerialBus(INFO_LOG, "115", "Gateway application connected! Sending Node ID");
        sendLogClientMessageIfAvailable("Gateway application connected! Sending Node ID");
        tmpRegistrationStatus = "OK";
        sendNodeIDToGateway();
        saveRegistrationStatus(&tmpRegistrationStatus);
        isFirstClientConnectionSTA = false;
//        resetGlobalEspTimer();
        digitalWrite(BUILTIN_LED, HIGH);
      }
    } else {
      String message = "@Unknown gateway message " + response;
      Serial.println(message);
      client.write((uint8_t*) &message[0], message.length());
    }
  }
  response = "";
}

void doAPModeOperations() {

  if (!client.connected()) {
    writeLogToFileAndSerialBus(INFO_LOG, "106", "Esp in AP mode... Waiting for configurations.....");
    while (true) {
      client = server.available();
      if (client.connected()) {
        writeLogToFileAndSerialBus(INFO_LOG, "107", "Configurator client connected!");
        String macAdress = getMacAddress();
        String statusString = getMessageString("1", INFO_LOG, "121", macAdress);
        client.write((uint8_t*)&statusString[0], statusString.length());
        blinkBuiltinLed(BLINK_IN_AP_MODE_CLIENT_CONNECTED);
        break;
      } else {
        writeLogToFileAndSerialBus(INFO_LOG, "108", "Waiting for configurator client...(AP Mode)");
        //blink 0.5 sec here.
        blinkBuiltinLed(BLINK_IN_AP_MODE_WAITING_CLIENTS);
      }
    }
  } else {
    //Serial.println("Client connected waiting for configurations....!!");
    writeLogToFileAndSerialBus(INFO_LOG, "107", "Configurator Client connected waiting for configurations....!!");
    blinkBuiltinLed(BLINK_IN_AP_MODE_CLIENT_CONNECTED);
    if (isFirstConnectionAP) {
      String error_log_msg;
      if (loadLog(ERROR_LOG, &error_log_msg)) {
        client.write((uint8_t*) &error_log_msg[0], error_log_msg.length());
        isFirstConnectionAP = false;
      }
    }
    
    while (client.available()) {
      char c = client.read();
      response += c;
    }

    if (!response.equals("")) {
      writeLogToFileAndSerialBus(INFO_LOG, "110", "Configuration Message Arrived : " + response);
      String statusString = "";
      if (handleConfigurationResponse()) {
        writeLogToFileAndSerialBus(INFO_LOG, "116", "Configuration message received by node");
        statusString = getMessageString("1", INFO_LOG, "111", "Configuration message received by node");
        client.flush();
        delay(2000);
        client.write((uint8_t*)&statusString[0], statusString.length());
      } else {
        writeLogToFileAndSerialBus(ERROR_LOG, "101", "Configuration FAILURE");
        statusString = getMessageString("1", ERROR_LOG, "101", "Configuration FAILURE");
        client.flush();
        delay(2000);
        client.write((uint8_t*)&statusString[0], statusString.length());
      }

      Serial.println("Configuration completed.");
      client.flush();
      delay(2000);
      Serial.println("Restarting...");
      ESP.restart();
      Serial.println("Restart command executed");
    }
  }

}

bool handleConfigurationResponse() {

  const int numberOfPieces = 4;
  int counter = 0;
  int lastIndex = 0;

  String parsedStrings[numberOfPieces];

  for (int i = 0; i < response.length(); i++) {
    // Loop through each character and check if it's a comma
    if (response.charAt(i) == '~') {
      // Grab the piece from the last index up to the current position and store it
      parsedStrings[counter] = response.substring(lastIndex, i);
      // Update the last position and add 1, so it starts from the next character
      lastIndex = i + 1;
      // Increase the position in the array that we store into
      counter++;
    }
  }

  // Grab the last part of the string from the lastIndex to the end
  parsedStrings[counter] = response.substring(lastIndex, response.length());

  Serial.println("SSID:  ");
  Serial.println(parsedStrings[0]);

  Serial.println("Password:  ");
  Serial.println(parsedStrings[1]);

  Serial.println("Gateway ID:  ");
  Serial.println(parsedStrings[2]);

  Serial.println("Node ID:  ");
  Serial.println(parsedStrings[3]);

  Serial.println("Last IP:  ");
  Serial.println(parsedStrings[4]);

  response = "";

  if (saveConfig(&parsedStrings[0], &parsedStrings[1]) && saveGatewayID(&parsedStrings[2]) && saveNodeID(&parsedStrings[3]) && saveLastIP(&parsedStrings[4])) {
    return true;
  }

  return false;

}

void readAllSensors() {
  Serial.println("Reading started!");
  String packet = "";
  float h = dht.readHumidity();
  // Read temperature as Celsius
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit
  float f = dht.readTemperature(true);

  Serial.print("Humidity:");
  Serial.println(h);
  Serial.print("Celcius:");
  Serial.println(t);
  Serial.print("Fahrenheit:");
  Serial.println(f);
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  String f_String = String(f);
  String c_String = String(t);
  String h_String = String(h);
  // Compute heat index
  // Must send in temp in Fahrenheit!
  float hi = dht.computeHeatIndex(f, h);
  Serial.println("Computing finished!");

  packet = packetBuilder(f_String, c_String, h_String);
  /////////////------------WIRELESS-------//////////////
  client.write((uint8_t*)&packet[0], packet.length());

  // Serial1.print("Humidity: "+ h);
  Serial.print('#');
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print("  Temperature Celsius: ");
  Serial.print(t);
  Serial.print(" *C");
  Serial.println();
  Serial.println("Packet : " + packet);

  packet = "";
  delay(100);

}

String packetBuilder(String fahrenheit, String celcius, String humidity) {
  String packet = "";

  packet += "#";
  packet += "|Checksum|+";

  packet += "DHT11:|" + fahrenheit + "|" + celcius + "|" + humidity + "|";

  packet += "~";

  return packet;
}

void startZeroConfService() {

  String name = "esp8266 " + getMacAddress();
  IPAddress localIP = WiFi.localIP();
  if (!MDNS.begin(&name[0], localIP, discovery_ttl)) {
    Serial.println("Error setting up MDNS responder!");
    // write to log here.
    while (1) {
      delay(1000);
    }
  }
  Serial.println("Service Name: " + name);
  Serial.println("mDNS responder started");

  MDNS.addService("esp8266", "tcp", PORT);
}

void updateTimers() {


  Serial.print("*** Heartbeat frequency:");
  Serial.println(HEARTBEAT_INTERVAL);
  heartbeatTimer.stop(myHeartbeatTimerInt);
  myHeartbeatTimerInt = heartbeatTimer.every(HEARTBEAT_INTERVAL, heartbeatGenerator);

  Serial.print("*** Reading frequency:");
  Serial.println(readingFreq);
  myDHTTimer.stop(myDHTTimerInt);
  myDHTTimerInt = myDHTTimer.every(readingFreq, readAllSensors);


}

void heartbeatGenerator() {

  if (WiFi.status() != WL_CONNECTED) {
    setup();
    Serial.println("Not connected...");
  } else {
    Serial.println("Heartbeat time...");
    String macAddress = getMacAddress();
    String heartbeat = "~" + macAddress + "~";
    client.write((uint8_t*) &heartbeat[0], heartbeat.length());
  }

}


void sendNodeIDToGateway() {

  if (WiFi.status() != WL_CONNECTED) {
    setup();
  } else if (configuredNodeID.length() > 1) {

    Serial.println("ID");
    Serial.println(configuredNodeID);

    String packet = "";
    packet += "&";
    packet += configuredNodeID;
    packet += "&";

    Serial.println("Sending NODE Packet");
    Serial.println(packet);
    client.write((uint8_t*) &packet[0], packet.length());

    packet = "";
  }

}

void handleDataResponse() {

  if (response.charAt(0) == 'R') {
    String config = response.substring(12, response.length());
    Serial.print("Config : ");
    Serial.print(config);
    Serial.println();
    readingFreq = (unsigned long) config.toFloat();
    Serial.println("Read Freq Changed!");
    updateTimers();
  } else if (response.charAt(0) == 'L') {
    if (response.equals("LED_ON\n")) {
      Serial.println("Opening led...");
      digitalWrite(LED_PIN, HIGH);
      delay(100);
    } else if (response.equals("LED_OFF\n")) {
      Serial.println("Closing led...");
      digitalWrite(LED_PIN, LOW);
      delay(100);
    } else {
      Serial.println("Unknown message2 " + response);
    }
  } else if (response.charAt(0) == '#') {
    resetConfigurations();
  } else {
    Serial.println("Unknown message3 " + response);
  }
  response = "";
}

void resetConfigurations() {

  //dummy data.
  String pp = "";
  String uu = "";
  String nodeID = "";
  saveConfig(&pp, &uu);
  saveGatewayID(&nodeID);
  saveRegistrationStatus(&nodeID);
  saveNodeID(&nodeID);

  WiFi.begin("wrong_ssid....", "wrong_ssid_pass...");

  ESP.restart();
}


void blinkBuiltinLed(long blinkTime) {
  digitalWrite(BUILTIN_LED, LOW);
  delay(blinkTime);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(blinkTime);
}


void restartESPinAPMode() {
  Serial.println("Timeout Reached. Starting ESP in AP MODE...");
  sendLogClientMessageIfAvailable("Timeout Reached. Starting ESP in AP MODE...");
  resetConfigurations();
}

void resetGlobalEspTimer() {

  espGlobalResetTimer.stop(espGlobalResetTimerInt);
  espGlobalResetTimerInt = espGlobalResetTimer.every(GLOBAL_ESP_RESET_TIMEOUT, restartESPinAPMode);

}

void writeLogToFileAndSerialBus(int type, String code, String msg) {
  String infoCounterStr = String(infoLogCounter);
  String log_str = getMessageString(infoCounterStr, type, code, msg);
  infoLogCounter++;
  Serial.println(log_str);
  saveLog(type, &log_str);
}

void parseLastConnectedIPString() {

  loadLastIP(&last_station_ip);
  if (last_station_ip.length() > 0) {

    const int numberOfPieces = 4;
    int counter = 0;
    int lastIndex = 0;

    String parsedStrings[numberOfPieces];

    for (int i = 0; i < last_station_ip.length(); i++) {
      // Loop through each character and check if it's a comma
      if (last_station_ip.charAt(i) == '.') {
        // Grab the piece from the last index up to the current position and store it
        parsedStrings[counter] = last_station_ip.substring(lastIndex, i);
        // Update the last position and add 1, so it starts from the next character
        lastIndex = i + 1;
        // Increase the position in the array that we store into
        counter++;
      }
    }

    // Grab the last part of the string from the lastIndex to the end
    parsedStrings[counter] = last_station_ip.substring(lastIndex, last_station_ip.length());

    /* Serial.println("IP 0:  ");
      Serial.println(parsedStrings[0]);

      Serial.println("IP 1:  ");
      Serial.println(parsedStrings[1]);

      Serial.println("IP 2:  ");
      Serial.println(parsedStrings[2]);


      Serial.println("IP 3:  ");
      Serial.println(parsedStrings[3]);*/

    int first_octet = parsedStrings[0].toInt();
    int second_octet = parsedStrings[1].toInt();
    int third_octet = parsedStrings[2].toInt();
    int fourth_octet = parsedStrings[3].toInt();

    last_connected_ip = IPAddress(first_octet, second_octet, third_octet, fourth_octet);


  }
}


// try 10 times connect last_connected_ip , and send logs.

void tryToConnectLastConnectedHost() {

  if (connectCounter < CONNECT_TIMEOUT) {
    parseLastConnectedIPString();
    if (log_client.connect(last_connected_ip, LOG_PORT)) {
      writeLogToFileAndSerialBus(INFO_LOG, "112", "Connected to log channel");

      // stop connection tryout.
      espLogTimer.stop(espLogTimerInt);
    } else {
      Serial.println("CANNOT connect to log channel" + connectCounter);
      writeLogToFileAndSerialBus(INFO_LOG, "113", "CANNOT connect to log channel");
      connectCounter++;
    }
  }


}


void sendLogClientMessageIfAvailable(String msg) {
  if (log_client.connected()) {
    writeLogToFileAndSerialBus(INFO_LOG, "117", "Log client is ready. Sending message..");
    msg = getMessageString("1", INFO_LOG, "120", msg);
    log_client.flush();
    log_client.write((uint8_t*) &msg[0], msg.length());
    log_client.flush();
  } else {
    writeLogToFileAndSerialBus(INFO_LOG, "118", "Log client is NOT ready");
  }
}



void resetOn() {
  Serial.println("\nHold Reset Button 4 Sec.\n");
  detachInterrupt(RESET_BUTTON);
  attachInterrupt(RESET_BUTTON, resetOnFinal, FALLING);
  resetStateTime = millis();
}

void resetOnFinal() {
  detachInterrupt(RESET_BUTTON);
  attachInterrupt(RESET_BUTTON, resetOn, RISING);
  if (resetStateTime + 4000 < millis()) {
    Serial.println("\nResetting...\n");
    resetConfigurations();
  } else {
    Serial.println("\nFail to reset. Push 4 Sec.\n");
  }
}

