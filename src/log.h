#pragma once
#include <types.h>
#include "vstring.h"

#define MSG_MAX_LEN 40

typedef enum LogLevel LogLevel;

enum LogLevel { Info, Warn };

typedef struct Log Log;

struct Log {
    LogLevel level;
    u16 msgLen;
    char msg[MSG_MAX_LEN];
};

void log_init(void);
void log_info(const char* fmt, ...);
void log_warn(const char* fmt, ...);
Log* log_dequeue(void);
