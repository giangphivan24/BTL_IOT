#ifndef __TASK_HANDLER_H__
#define __TASK_HANDLER_H__

#include <ArduinoJson.h>
#include <task_check_info.h>

// Xử lý message WebSocket
extern void handleWebSocketMessage(String message);

#endif