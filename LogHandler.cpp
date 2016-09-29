#include "LogHandler.h"
#include <ArduinoJson.h>



String getMessageString(const String id, const int type, const String code, const String msg){


	char json[] = "{\"id\":\"\",\"type\":,\"code\":\"\",\"message\":\"\",\"params\":[]}";
	
	String buffer;

	StaticJsonBuffer<2000> jsonBuffer;

	JsonObject& root = jsonBuffer.parseObject(json);

	root["id"] = id;
  root["type"] = type;
	root["code"] = code;
	root["message"] = msg;

	root.printTo(buffer);

	return buffer;
}
