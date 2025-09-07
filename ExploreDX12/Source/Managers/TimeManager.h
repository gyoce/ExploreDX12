#pragma once

class TimeManager
{
public:
    void Tick();

    static float GetDeltaTime();

private:
    TimeManager();

    double mSecondsPerCount;
    double mDeltaTime = 0.0f;
    __int64 mCurrentTime = 0;
    __int64 mPreviousTime = 0;

    static TimeManager* sInstance;

    friend class Application;
};

