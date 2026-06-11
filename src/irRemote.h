#ifndef __IR_REMOTE_H__
#define __IR_REMOTE_H__

#include <Arduino.h>
#include <IRac.h>

//public api
void irRemoteInit();
void irRemoteLoop();
void irRemoteProcessCommand(String cmd);
void irRemoteScan();
bool isScanMode();
void irRemoteSendSignal();

stdAc::state_t* irRemotegetAcState();
const String* irRemotegetProtoName();

#endif