#include "time_utils.h"


int64_t getLocalTimeMillisl(void)
{
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	return computeTimeMillis(sysTime);
}

int64_t computeTimeMillis(WORD hour, WORD minute, WORD second, WORD milliseconds)
{
	return milliseconds + 1000LL * (second + 60LL * (minute + 60LL * hour));
}

int64_t computeTimeMillis(const SYSTEMTIME& time)
{
	return computeTimeMillis(time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
}
