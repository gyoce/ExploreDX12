#pragma once

class TimeManager
{
public:
    void Tick();
    void Resume();
    void Pause();

    static float GetDeltaTime();
    static float GetTotalTime();

private:
    TimeManager();

    bool mPause = false;
    double mSecondsPerCount;
    double mDeltaTime = 0.0f;
    __int64 mBaseTime = 0;
    __int64 mCurrentTime = 0;
    __int64 mPreviousTime = 0;
    __int64 mPauseTime = 0;
    __int64 mAccumulatedPausedTime = 0;

    static TimeManager* sInstance;

    friend class Application;
};

