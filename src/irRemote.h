#ifndef __IR_REMOTE_H__
#define __IR_REMOTE_H__

#include <Arduino.h>

//public api
void irRemoteInit();
void irRemoteLoop();
void irRemoteProcessCommand(String cmd);
void irRemoteScan();
bool isScanMode();
const stdAc::state_t* irRemotegetAcState();
const String* irRemotegetProtoName();

#endif