#ifndef __TIME_UTILS__
#define __TIME_UTILS__

#include <cstdint>
#include "Windows.h"

int64_t getLocalTimeMillisl(void);

int64_t computeTimeMillis(WORD hour, WORD minute, WORD second, WORD milliseconds);

int64_t computeTimeMillis(const SYSTEMTIME& time);

#endif
