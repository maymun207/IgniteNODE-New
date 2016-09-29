/**
 * 
 * 
 * @author yavuz.erzurumlu (http://www.ardictech.com)
 * @date 2016-09-07
 * 
 **/

#ifndef _INCL_CONFIGURATIONFILEUTILS
#define _INCL_CONFIGURATIONFILEUTILS

/// Uncomment the next line for verbose output over UART.
//#define SERIAL_VERBOSE

#include <WString.h>

#define ERROR_LOG 2
#define INFO_LOG 0
#define WARNING_LOG 1
#define VERBOSE_LOG 3

/**
 * @brief Read WiFi connection information from file system.
 * @param ssid String pointer for storing SSID.
 * @param pass String pointer for storing PSK.
 * @return True or False.
 * 
 * The config file have to containt the WiFi SSID in the first line
 * and the WiFi PSK in the second line.
 * Line seperator can be \r\n (CR LF) \r or \n.
 */
bool loadConfig(String *ssid, String *pass);

/**
 * @brief Save WiFi SSID and PSK to configuration file.
 * @param ssid SSID as string pointer.
 * @param pass PSK as string pointer,
 * @return True or False.
 */
bool saveConfig(String *ssid, String *pass);

bool loadNodeID(String *nodeID);

bool saveNodeID(String *nodeID);

bool loadGatewayID(String *gatewayID);

void clearConfigurationFiles();

void clearFile(String fileName);

bool saveRegistrationStatus(String *gatewayID);

bool loadRegistrationStatus(String *gatewayID);

bool saveWifiTryCount(String *wifiTryCount);

bool loadWifiTryCount(String *wifiTryCount);

bool saveGatewayID(String *gatewayID);

bool startFileSystem();


bool saveLog(int logType, String *logMsg);

bool loadLog(int logType, String *logMsg);

bool saveLastIP(String *ip);

bool loadLastIP(String *ip);


#endif
