#include <httpHandler.h>

#define API_AC_POWER_PATH   "/api/v1/ac/power"
#define API_AC_STATUS_PATH  "/api/v1/ac/status"
#define API_AC_CURRENT_PATH "/api/v1/ac/current"
#define API_AC_COMMAND_PATH  "/api/v1/ac/command"

/////rapihin nanti 
String buildApiUrl(const String& baseUrl, const char* path) {
  String url = baseUrl;
  if (url.endsWith("/")) {
    url.remove(url.length() - 1);
  }

  if (path[0] != '/') {
    url += "/";
  }

  url += path;
  return url;
}

///// POST STATUS HANDLER ///////
void httpPostStatusHandler_s::init(const String& url, const stdAc::state_t* acState, 
const String* protoName){
    if (acState == nullptr || protoName == nullptr) { Serial.println("err : nullptr on statusHandler"); while(1);};

    p_protoName = protoName;
    p_acState = acState;
    urlAddr = buildApiUrl(url, API_AC_STATUS_PATH);
}

void httpPostStatusHandler_s::startSend(){
    String dataJson = "";
    buildJsonPayload(dataJson);

    int ret = sendJsonPost(urlAddr, dataJson);
    // Serial.print("post status ret: "); Serial.println(ret); //get http response 
}

void httpPostStatusHandler_s::buildJsonPayload(String &payloadJson){
    JsonDocument doc; 

    doc["id"] = DEVICE_ID;
    doc["lastCmdId"] = last_cmd_id; //kekny gaperlu
    doc["ip"] = WiFi.localIP().toString();
    doc["uptimeMs"] = millis();
    doc["fwVer"] = FW_VERSION;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["protocol"] = *p_protoName;
    doc["P"] = p_acState->power;
    doc["temp"] = p_acState->degrees;
    // doc["mode"] = IRac::opmodeToString(p_acState->mode);
    // doc["fan"] = IRac::fanspeedToString(p_acState->fanspeed);
    // doc["swing"] = IRac::swingvToString(p_acState->swingv);
    doc["mode"] = (int)p_acState->mode;
    doc["fan"] = (int)p_acState->fanspeed;
    doc["swing"] = (int)p_acState->swingv;
    

    payloadJson = "";
    serializeJson(doc, payloadJson);    
}



///// POST CURRENT HANDLER ///////
void httpPostCurrentHandler_s::init(const String&baseUrl, sct013_val_s *currentInstance){
    while(currentInstance == NULL){}; //make sure it's valid object 
    p_val = currentInstance;
    urlAddr = buildApiUrl(baseUrl, API_AC_CURRENT_PATH);
}

void httpPostCurrentHandler_s::startSend(){
    String dataJson = "";
    buildJsonPayload(dataJson);

    int ret = sendJsonPost(urlAddr, dataJson);
    // Serial.print("post current ret: "); Serial.println(ret); //get http response 
}


void httpPostCurrentHandler_s::buildJsonPayload(String &payloadJson){
    JsonDocument doc;

    doc["id"] = DEVICE_ID;
    doc["current"] = p_val->currRms;
    doc["power"] = p_val->powerRms;
    doc["timestamp_ms"] = millis();

    payloadJson = "";
    serializeJson(doc, payloadJson);
}

//////// GET COMMAND HANDLER ///////
///// GET COMMAND HANDLER ///////

void httpGetCommandHandler_s::init(const String& baseUrl,
                                   stdAc::state_t* acState,
                                   const String* protoName) {
    if (acState == nullptr || protoName == nullptr) {
        Serial.println("err : nullptr on commandHandler");
        while (1);
    }

    p_acState = acState;
    p_protoName = protoName;
    last_cmd_id = 0;
    urlAddr = buildApiUrl(baseUrl, API_AC_COMMAND_PATH);
}


bool httpGetCommandHandler_s::checkCommand() {
    String responseJson = "";

    int ret = sendJsonGet(urlAddr, responseJson);

    Serial.print("get command ret: ");
    Serial.println(ret);

    if (ret != 200) {
        return false;
    }

    if (responseJson.length() == 0) {
        return false;
    }

    return parseCommandJson(responseJson);
}


bool httpGetCommandHandler_s::parseCommandJson(const String& payloadJson) {
    JsonDocument doc;

    DeserializationError err = deserializeJson(doc, payloadJson);
    if (err) {
        Serial.print("parse command json failed: ");
        Serial.println(err.c_str());
        return false;
    }

    uint32_t cmdId = doc["cmdId"] | 0;

    if (cmdId == 0) {
        Serial.println("invalid cmdId");
        return false;
    }

    if (cmdId == last_cmd_id) {
        Serial.println("same command id, skip");
        return false;
    }

    bool power = doc["power"] | p_acState->power;
    uint8_t temp = doc["temp"] | p_acState->degrees;
    uint8_t mode = doc["mode"] | (uint8_t)p_acState->mode;
    uint8_t fan = doc["fan"] | (uint8_t)p_acState->fanspeed;
    uint8_t swingv = doc["swingv"] | (uint8_t)p_acState->swingv;

    p_acState->power = power;
    p_acState->degrees = temp;
    p_acState->mode = (stdAc::opmode_t)mode;
    p_acState->fanspeed = (stdAc::fanspeed_t)fan;
    p_acState->swingv = (stdAc::swingv_t)swingv;

    last_cmd_id = cmdId;

    Serial.print("New command [");
    Serial.print(*p_protoName);
    Serial.print("] cmdId=");
    Serial.print(cmdId);
    Serial.print(" P=");
    Serial.print(p_acState->power ? "ON" : "OFF");
    Serial.print(" T=");
    Serial.print(p_acState->degrees);
    Serial.print(" M=");
    // Serial.print(IRac::opmodeToString(p_acState->mode));
    Serial.print((int)p_acState->mode);
    Serial.print(" F=");
    // Serial.print(IRac::fanspeedToString(p_acState->fanspeed));
    Serial.print((int)p_acState->fanspeed);
    Serial.print(" SV=");
    // Serial.println(IRac::swingvToString(p_acState->swingv));
    Serial.println((int)p_acState->swingv);

    return true;
}

//////// common function ///////////////

int sendJsonPost(const String&url, const String &payloadJson){
    if (WiFi.status() != WL_CONNECTED) { //check wifi connection
        return -1;  
    }
    
    HTTPClient http;
    http.setTimeout(1500); //make sure if http not reply in 1.5 seconds, skip it 

    http.begin(url); //specify url server address
    http.addHeader("Content-Type", "application/json"); //tell server, we will send json data     
    int httpResponseCode = http.POST(payloadJson); //send payload with POST method 

    http.end(); //end http comm 
    return httpResponseCode;
}

int sendJsonGet(const String& url, String& responseJson) {
    responseJson = "";

    if (WiFi.status() != WL_CONNECTED) {
        return -1;
    }

    HTTPClient http;
    http.setTimeout(3000);

    http.begin(url);
    http.addHeader("Accept", "application/json");

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
        responseJson = http.getString();
    }

    http.end();
    return httpResponseCode;
}