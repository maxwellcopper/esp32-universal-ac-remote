#ifndef __HTTP_HANDLER_H__
#define __HTTP_HANDLER_H__

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <sct013.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRac.h>
#define FW_VERSION      "1.0.0"
#define DEVICE_ID       "IR_01"


struct httpPostStatusHandler_s{
    const stdAc::state_t *p_acState = nullptr;
    const String* p_protoName = nullptr;
    const char* urlAddr;
    uint32_t last_cmd_id;
    
    void init(const char* url, const stdAc::state_t* acState, const String* protoName);
    void startSend();
    void buildJsonPayload(String &payloadJson);
};

struct httpPostCurrentHandler_s{
    const char* urlAddr;
    bool ac_power;
    sct013_val_s* p_val;

    void init(const char* url, sct013_val_s *currInstance);
    void startSend();
    void buildJsonPayload(String &payloadJson);
};


//common function
int sendJsonPost(const char*url, const String &payloadJson);

#endif