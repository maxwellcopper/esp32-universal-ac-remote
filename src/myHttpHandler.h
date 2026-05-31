#ifndef __MY_HTTP_HANDLER_H__
#define __MY_HTTP_HANDLER_H__

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <sct013.h>
#define FW_VERSION      "1.0.0"
#define DEVICE_ID       "IR_01"


typedef struct{
    const char* urlAddr;
    uint32_t last_cmd_id;
    bool ac_power;
    float temp;
    uint8_t mode;
    uint8_t fan;

    void init(const char* url);
    void startSend();
    void buildJsonPayload(String &payloadJson);
}httpPostStatusHandler_s;

typedef struct{
    const char* urlAddr;
    bool ac_power;
    sct013_val_s* p_val;

    void init(const char* url, sct013_val_s *currInstance);
    void startSend();
    void buildJsonPayload(String &payloadJson);
}httpPostCurrentHandler_s;


//common function
int sendJsonPost(const char*url, const String &payloadJson);

#endif