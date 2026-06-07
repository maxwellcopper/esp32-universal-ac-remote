#ifndef __IR_REMOTE_H__
#define __IR_REMOTE_H__

#include <Arduino.h>

//public api
void irRemoteInit();
void irRemoteLoop();
void irRemoteProcessCommand(String cmd);

#endif