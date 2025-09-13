#include "Managers/TimeManager.h"

#include <windows.h>

TimeManager* TimeManager::sInstance = nullptr;

TimeManager::TimeManager()
{
    sInstance = this;
    __int64 countsPerSec;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
    mSecondsPerCount = 1.0 / static_cast<double>(countsPerSec);
    QueryPerformanceCounter((LARGE_INTEGER*)&mBaseTime);
}

void TimeManager::Tick()
{
    QueryPerformanceCounter((LARGE_INTEGER*)&mCurrentTime);
    mDeltaTime = (mCurrentTime - mPreviousTime) * mSecondsPerCount;
    mPreviousTime = mCurrentTime;

    if (mDeltaTime < 0.0f)
        mDeltaTime = 0.0f;
}

void TimeManager::Resume()
{
    if (mPause)
    {
        __int64 resumeTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&resumeTime);
        mAccumulatedPausedTime += (resumeTime - mPauseTime);

        mPreviousTime = resumeTime;
        mPauseTime = 0;
        mPause = false;
    }
}

void TimeManager::Pause()
{
    if (!mPause)
    {
        __int64 currTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

        mPauseTime = currTime;
        mPause = true;
    }
}

float TimeManager::GetDeltaTime()
{
    return static_cast<float>(sInstance->mDeltaTime);
}

float TimeManager::GetTotalTime()
{
    if (sInstance->mPause)
        return static_cast<float>( ((sInstance->mPauseTime - sInstance->mAccumulatedPausedTime) - sInstance->mBaseTime) * sInstance->mSecondsPerCount );
    
    return static_cast<float>(((sInstance->mCurrentTime - sInstance->mAccumulatedPausedTime) - sInstance->mBaseTime) * sInstance->mSecondsPerCount );
}
