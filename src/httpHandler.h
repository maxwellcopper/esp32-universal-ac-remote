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
    String urlAddr;
    uint32_t last_cmd_id;
    
    void init(const String& url, const stdAc::state_t* acState, const String* protoName);
    void startSend();
    void buildJsonPayload(String &payloadJson);
};

struct httpPostCurrentHandler_s{
    String urlAddr;
    bool ac_power;
    sct013_val_s* p_val;

    void init(const String&baseUrl, sct013_val_s *currentInstance);
    void startSend();
    void buildJsonPayload(String &payloadJson);
};

typedef struct {
    String urlAddr;
    stdAc::state_t* p_acState;
    const String* p_protoName;
    uint32_t last_cmd_id;

    void init(const String& baseUrl, stdAc::state_t* acState, const String* protoName);
    bool checkCommand();
    bool parseCommandJson(const String& payloadJson);
} httpGetCommandHandler_s;


//common function
int sendJsonPost(const String &url, const String &payloadJson);
int sendJsonGet(const String& url, String& responseJson);

#endif