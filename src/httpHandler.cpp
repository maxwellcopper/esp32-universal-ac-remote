#include <myHttpHandler.h>

///// POST STATUS HANDLER ///////
void httpPostStatusHandler_s::init(const char* url){
    urlAddr = url;
}

void httpPostStatusHandler_s::startSend(){
    String dataJson = "";
    buildJsonPayload(dataJson);

    int ret = sendJsonPost(urlAddr, dataJson);
    Serial.print("post status ret: "); Serial.println(ret); //get http response 
}

void httpPostStatusHandler_s::buildJsonPayload(String &payloadJson){
    JsonDocument doc; 

    doc["id"] = DEVICE_ID;
    doc["lastCmdId"] = last_cmd_id;
    doc["ip"] = WiFi.localIP().toString();
    doc["uptimeMs"] = millis();
    doc["fwVer"] = FW_VERSION;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["acPower"] = ac_power;
    doc["temp"] = temp;
    doc["mode"] = mode;
    doc["fan"] = fan;
    
    payloadJson = "";
    serializeJson(doc, payloadJson);    
}

///// POST CURRENT HANDLER ///////
void httpPostCurrentHandler_s::init(const char* url, sct013_val_s *currentInstance){
    while(currentInstance == NULL){}; //make sure it's valid object 
    p_val = currentInstance;
    urlAddr = url;
}

void httpPostCurrentHandler_s::startSend(){
    String dataJson = "";
    buildJsonPayload(dataJson);

    int ret = sendJsonPost(urlAddr, dataJson);
    Serial.print("post current ret: "); Serial.println(ret); //get http response 
}


void httpPostCurrentHandler_s::buildJsonPayload(String &payloadJson){
    JsonDocument doc;

    doc["id"] = DEVICE_ID;
    doc["acPower"] = ac_power;
    doc["current"] = p_val->currRms;
    doc["power"] = p_val->powerRms;
    doc["timestamp_ms"] = millis();

    payloadJson = "";
    serializeJson(doc, payloadJson);
}


//////// common function ///////////////

int sendJsonPost(const char*url, const String &payloadJson){
    if (WiFi.status() != WL_CONNECTED) { //check wifi connection
        return -1; 
    }
    
    HTTPClient http;
    http.setTimeout(3000); //make sure if http not reply in 3 seconds, skip it 

    http.begin(url); //specify url server address
    http.addHeader("Content-Type", "application/json"); //tell server, we will send json data     
    int httpResponseCode = http.POST(payloadJson); //send payload with POST method 

    http.end(); //end http comm 
    return httpResponseCode;
}