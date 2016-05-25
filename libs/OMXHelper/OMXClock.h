#pragma once

#include "OMXCore.h"

#ifdef OMX_SKIP64BIT
static inline OMX_TICKS ToOMXTime(int64_t pts)
{
	OMX_TICKS ticks;
	ticks.nLowPart = pts;
	ticks.nHighPart = pts >> 32;
	return ticks;
}
static inline uint64_t FromOMXTime(OMX_TICKS ticks)
{
	uint64_t pts = ticks.nLowPart | ((uint64_t)ticks.nHighPart << 32);
	return pts;
}
#else
#define FromOMXTime(x) (x)
#define ToOMXTime(x) (x)
#endif

#define CLOCK_TIME_BASE 1000000
#define CLOCK_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and __int64

#define CLOCK_TIME_TO_SEC(x)  ((int)((double)(x) / CLOCK_TIME_BASE))
#define CLOCK_TIME_TO_MSEC(x) ((int)((double)(x) * 1000 / CLOCK_TIME_BASE))
#define CLOCK_SEC_TO_TIME(x)  ((double)(x) * CLOCK_TIME_BASE)
#define CLOCK_MSEC_TO_TIME(x) ((double)(x) * CLOCK_TIME_BASE / 1000)

#define CLOCK_PLAYSPEED_PAUSE       0       // frame stepping
#define CLOCK_PLAYSPEED_NORMAL      1000

class OMXClock : public OMXCoreComponent
{
public:
	OMXClock(void);
	~OMXClock(void);

	bool Init(void);
	void Deinit(void);

	void Lock(void);
	void Unlock(void);

	void SetClockPorts(OMX_TIME_CONFIG_CLOCKSTATETYPE* clock, bool hasVideo, bool hasAudio);
	bool SetReferenceClock(bool hasAudio, bool lock = true);

	bool Pause(bool lock = true);
	bool Resume(bool lock = true);
	bool Stop(bool lock = true);
	bool Start(bool lock = true);
	bool Step(int steps = 1, bool lock = true);
	bool Reset(bool hasVideo, bool hasAudio, bool lock = true);

	double GetMediaTime(bool lock = true);
	bool SetMediaTime(double pts, bool lock = true);
	double ClockAdjustment(bool lock = true);

	bool SetSpeed(int speed, bool lock = true, bool pause_resume = false);

	bool StateExecute(bool lock = true);
	void StateIdle(bool lock = true);

	bool HDMIClockSync(bool lock = true);

	uint64_t GetAbsoluteClock(void) const;
	double GetClock(void) const;

	/** Sleep for x milliseconds
	*	@param milliseconds How many seconds to sleep for
	*/
	static void Sleep(unsigned int milliseconds);

public:
	bool IsPaused(void) const { return m_pause; }
	int PlaySpeed(void) const { return m_omxSpeed; }

private:
	double m_lastMediaTime;
	double m_lastMediaTimeRead;

protected:
	int m_omxSpeed;
	OMX_U32 m_waitMask;
	OMX_TIME_CLOCKSTATE m_eState;
	OMX_TIME_REFCLOCKTYPE m_eClock;
	bool m_pause;

	pthread_mutex_t m_lock;
};

