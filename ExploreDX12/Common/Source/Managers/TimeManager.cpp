#include "Managers/TimeManager.h"

#include <windows.h>

TimeManager* TimeManager::sInstance = nullptr;

TimeManager::TimeManager()
{
    sInstance = this;
    __int64 countsPerSec;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
    mSecondsPerCount = 1.0 / static_cast<double>(countsPerSec);
}

void TimeManager::Tick()
{
    QueryPerformanceCounter((LARGE_INTEGER*)&mCurrentTime);
    mDeltaTime = (mCurrentTime - mPreviousTime) * mSecondsPerCount;
    mPreviousTime = mCurrentTime;

    if (mDeltaTime < 0.0f)
        mDeltaTime = 0.0f;
}

float TimeManager::GetDeltaTime()
{
    return static_cast<float>(sInstance->mDeltaTime);
}
