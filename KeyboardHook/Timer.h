//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

struct Timer
{
	Timer()
	{
		Reset();
	}

	void Reset()
	{
		QueryPerformanceCounter((LARGE_INTEGER *)&mStartTime);
		mOldTime = mStartTime;
	}

	uint64 GetCounter()
	{
		uint64 time;
		QueryPerformanceCounter((LARGE_INTEGER *)&time);
		return time;
	}

	double GetElapsed()
	{
		uint64 time;
		uint64 frequency;
		QueryPerformanceCounter((LARGE_INTEGER *)&time);
		QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);
		return (double)(time - mStartTime) / (double)frequency;
	}

	double GetDelta()
	{
		uint64 time;
		uint64 frequency;
		QueryPerformanceCounter((LARGE_INTEGER *)&time);
		QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);
		double delta = (double)(time - mOldTime) / (double)frequency;
		mOldTime = time;
		return delta;
	}

	uint64 mStartTime;
	uint64 mOldTime;
};

//////////////////////////////////////////////////////////////////////
// Global timings

extern double	g_Time;				// time in seconds since App began
extern float	g_DeltaTime;		// time in seconds since last frame
