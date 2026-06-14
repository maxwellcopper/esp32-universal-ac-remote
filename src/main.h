#ifndef __MY_MAIN_H__
#define __MY_MAIN_H__

/*
***SCT013 Current Sensor GLobal Var
*/
#define SCT013_PIN     32
sct013_val_s sct013;

/*
***Http handler Global Var
*/
httpPostStatusHandler_s postStatus;
httpPostCurrentHandler_s postCurrent;
httpGetCommandHandler_s getCommand;

/*
***Utility Var n Proto Function
*/
#define POST_STATUS_INTERVAL    5000
#define POST_CURRENT_INTERVAL   2000
#define GET_COMMAND_INTERVAL    10000
uint32_t lastPostStatus = 0;
uint32_t lastPostCurrent = 0;
uint32_t lastBlinking = 0;
uint32_t lastGetCommand = 0;
uint32_t lastSampling = 0;
uint32_t loopDuration = 0;
int isOn = 0;
void wifiManager_task();
void remoteScan_task();
void blinkingLed_task(int blinkTime);
void serialCommand_task();
#endif