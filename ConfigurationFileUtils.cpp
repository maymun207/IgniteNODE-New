#include "ConfigurationFileUtils.h"
#include <FS.h>



/**
   @brief Read WiFi connection information from file system.
   @param ssid String pointer for storing SSID.
   @param pass String pointer for storing PSK.
   @return True or False.

   The config file have to containt the WiFi SSID in the first line
   and the WiFi PSK in the second line.
   Line seperator can be \r\n (CR LF) \r or \n.
*/
bool loadConfig(String *ssid, String *pass)
{
  // open file for reading.
  File configFile = SPIFFS.open("/cl_conf.txt", "r");
  if (!configFile)
  {
    Serial.println("Failed to open cl_conf.txt.");

    return false;
  }

  // Read content from config file.
  String content = configFile.readString();
  configFile.close();

  content.trim();

  // Check if ther is a second line available.
  int8_t pos = content.indexOf("\r\n");
  uint8_t le = 2;
  // check for linux and mac line ending.
  if (pos == -1)
  {
    le = 1;
    pos = content.indexOf("\n");
    if (pos == -1)
    {
      pos = content.indexOf("\r");
    }
  }

  // If there is no second line: Some information is missing.
  if (pos == -1)
  {
    Serial.println("Infvalid content.");
    Serial.println(content);

    return false;
  }

  // Store SSID and PSK into string vars.
  *ssid = content.substring(0, pos);
  *pass = content.substring(pos + le);

  ssid->trim();
  pass->trim();

  return true;
} // loadConfig

bool loadNodeID(String *nodeID) {
  // open file for reading.
  File configFile = SPIFFS.open("/node_conf.txt", "r");
  if (!configFile)
  {
    Serial.println("Failed to open node_conf.txt.");

    return false;
  }

  String content = configFile.readString();
  configFile.close();


  content.trim();

  *nodeID = content;
  nodeID->trim();

  Serial.println("Loading nodeID");
  Serial.println("nodeID: " + *nodeID);


  return true;
}
/**
   @brief Save WiFi SSID and PSK to configuration file.
   @param ssid SSID as string pointer.
   @param pass PSK as string pointer,
   @return True or False.
*/
bool saveConfig(String *ssid, String *pass)
{
  // Open config file for writing.
  File configFile = SPIFFS.open("/cl_conf.txt", "w");
  if (!configFile)
  {
    Serial.println("Failed to open cl_conf.txt for writing");

    return false;
  }

  // Save SSID and PSK.
  configFile.println(*ssid);
  configFile.println(*pass);

  configFile.close();

  return true;
} // saveConfig

bool saveNodeID(String *nodeID) {
  // Open config file for writing.
  File configFile = SPIFFS.open("/node_conf.txt", "w");
  if (!configFile)
  {
    Serial.println("Failed to open node_conf.txt for writing");

    return false;
  }

  // Save nodeID



  configFile.println(*nodeID);
  configFile.close();

  Serial.println("Saving nodeID");
  Serial.println("nodeID: " + *nodeID);

  return true;
}

void clearConfigurationFiles(){
  clearFile("/node_conf.txt");
  clearFile("/gateway_conf.txt");
  clearFile("/registration_conf.txt");
  clearFile("/cl_conf.txt");
  clearFile("/wifiTryCount_conf.txt");
}

void clearFile(String fileName){
  Serial.println("File deleting " + fileName);
  File file = SPIFFS.open(fileName, "w");
  file.close();
}

bool saveGatewayID(String *gatewayID) {
  File configFile = SPIFFS.open("/gateway_conf.txt", "w");
  if (!configFile){
    Serial.println("Failed to open gateway_conf.txt for writing");
    return false;
  }
  configFile.println(*gatewayID);
  configFile.close();
  Serial.println("Saving gatewayID " + *gatewayID);
  return true;
}

bool loadGatewayID(String *gatewayID) {
  File configFile = SPIFFS.open("/gateway_conf.txt", "r");
  if (!configFile){
    Serial.println("Failed to open gateway_conf.txt.");
    return false;
  }
  String content = configFile.readString();
  configFile.close();
  content.trim();
  *gatewayID = content;
  gatewayID->trim();
  Serial.println("Loading gatewayID " + *gatewayID);
  return true;
}

bool saveRegistrationStatus(String *registrationStatus) {
  File configFile = SPIFFS.open("/registration_conf.txt", "w");
  if (!configFile){
    Serial.println("Failed to open registration_conf.txt for writing");
    return false;
  }
  configFile.println(*registrationStatus);
  configFile.close();
  Serial.println("Saving registrationStatus " + *registrationStatus);
  return true;
}

bool loadRegistrationStatus(String *registrationStatus) {
  File configFile = SPIFFS.open("/registration_conf.txt", "r");
  if (!configFile){
    Serial.println("Failed to open registration_conf.txt.");
    return false;
  }
  String content = configFile.readString();
  configFile.close();
  content.trim();
  *registrationStatus = content;
  registrationStatus->trim();
  Serial.println("Loading registrationStatus " + *registrationStatus);
  return true;
}

bool saveWifiTryCount(String *wifiTryCount) {
  File configFile = SPIFFS.open("/wifiTryCount_conf.txt", "w");
  if (!configFile){
    Serial.println("Failed to open wifiTryCount_conf.txt for writing");
    return false;
  }
  configFile.println(*wifiTryCount);
  configFile.close();
  Serial.println("Saving wifiTryCount " + *wifiTryCount);
  return true;
}

bool loadWifiTryCount(String *wifiTryCount) {
  File configFile = SPIFFS.open("/wifiTryCount_conf.txt", "r");
  if (!configFile){
    Serial.println("Failed to open wifiTryCount_conf.txt.");
    return false;
  }
  String content = configFile.readString();
  configFile.close();
  content.trim();
  *wifiTryCount = content;
  wifiTryCount->trim();
  if(*wifiTryCount == ""){
    *wifiTryCount = "0";
  }
  Serial.println("Loading wifiTryCount " + *wifiTryCount);
  return true;
}


bool startFileSystem() {

  // Initialize file system.
  if (!SPIFFS.begin())
  {
    Serial.println("Failed to mount file system");
    return false;
  }

  return true;

}

/**
   @brief Save WiFi SSID and PSK to configuration file.
   @param ssid SSID as string pointer.
   @param pass PSK as string pointer,
   @return True or False.
*/
bool saveLog(int logType, String *logMsg)
{

  // check type

  if (ERROR_LOG == logType) {
    // Open error log file for writing.
    File errorLogFile = SPIFFS.open("/error_log.txt", "w");
    if (!errorLogFile) {
      Serial.println("Failed to open error_log.txt for writing");
      return false;
    }


    // Save error log.
    errorLogFile.println(*logMsg);
    errorLogFile.close();
    return true;

  } else if (INFO_LOG == logType) {
    //TODO umut
    // Open info log file for writing.
    /*File infoLogFile = SPIFFS.open("/info_log.txt", "w");
    if (!infoLogFile) {
      Serial.println("Failed to open info_log.txt for writing");
      return false;
    }


    // Save info log.
    //infoLogFile.println(*logMsg);
    infoLogFile.close();*/
    return true;
  }


  return false;
} // saveLog

bool loadLog(int logType, String *logMsg) {

  if (ERROR_LOG == logType) {

    // open file for reading.
    File errorLogFile = SPIFFS.open("/error_log.txt", "r");
    if (!errorLogFile) {
      Serial.println("Failed to open error_log.txt.");
      return false;
    }

    String errorLogString = errorLogFile.readString();
    errorLogFile.close();
    errorLogString.trim();

    *logMsg = errorLogString;
    logMsg->trim();

    Serial.println("Loading Error LOG....");
    Serial.println(*logMsg);

    return true;



  } else if (INFO_LOG == logType) {

    // open file for reading.
    File infoLogFile = SPIFFS.open("/info_log.txt", "r");
    if (!infoLogFile) {
      Serial.println("Failed to open info_log.txt.");
      return false;
    }

    String infoLogString = infoLogFile.readString();
    infoLogFile.close();
    infoLogString.trim();

    *logMsg = infoLogString;
    logMsg->trim();

    Serial.println("Loading Error LOG....");
    Serial.println(*logMsg);

    return true;

  }

  return false;

}

bool saveLastIP(String *lastIP) {
  // Open config file for writing.
  File lastIPFile = SPIFFS.open("/last_ip.txt", "w");
  if (!lastIPFile)
  {
    Serial.println("Failed to open node_conf.txt for writing");

    return false;
  }

  // Save nodeID



  lastIPFile.println(*lastIP);
  lastIPFile.close();

  Serial.println("Saving lastIP");
  Serial.println("lastIP: " + *lastIP);

  return true;
}


bool loadLastIP(String *lastIP) {
  // open file for reading.
  File lastIPFile = SPIFFS.open("/last_ip.txt", "r");
  if (!lastIPFile)
  {
    Serial.println("Failed to open last_ip.txt.");

    return false;
  }

  String content = lastIPFile.readString();
  lastIPFile.close();


  content.trim();

  *lastIP = content;
  lastIP->trim();

  Serial.println("Loading lastIP");
  Serial.println("lastIP: " + *lastIP);


  return true;
}

