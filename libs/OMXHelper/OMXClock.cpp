#include "OMXClock.h"

#define OMX_PRE_ROLL 200
#define TP(speed) ((speed) < 0 || (speed) > 4*CLOCK_PLAYSPEED_NORMAL)

OMXClock::OMXClock(void)
	: m_pause(false), m_omxSpeed(CLOCK_PLAYSPEED_NORMAL), m_waitMask(0), m_eState(OMX_TIME_ClockStateStopped),
	m_eClock(OMX_TIME_RefClockNone), m_lastMediaTime(0.0f), m_lastMediaTimeRead(0.0f)
{
	pthread_mutex_init(&m_lock, NULL);
}

OMXClock::~OMXClock(void)
{
	Deinit();

	pthread_mutex_destroy(&m_lock);
}

bool OMXClock::Init(void)
{
	m_pause = false;

	if (!Initialise("OMX.broadcom.clock", OMX_IndexParamOtherInit))
		return false;

	OMX_TIME_CONFIG_CLOCKSTATETYPE clock;
	OMX_INIT_STRUCTURE(clock);

	clock.eState = OMX_TIME_ClockStateWaitingForStartTime;

	OMX_ERRORTYPE omxErr = SetConfig(OMX_IndexConfigTimeClockState, &clock);
	if (omxErr != OMX_ErrorNone)
	{
	}

	m_eState = clock.eState;

	return true;
}

void OMXClock::Deinit(void)
{
	if (!GetComponent())
		return;

	Deinitialise();

	m_omxSpeed = CLOCK_PLAYSPEED_NORMAL;
	m_lastMediaTime = 0.0f;
}

void OMXClock::Lock(void)
{
	pthread_mutex_lock(&m_lock);
}

void OMXClock::Unlock(void)
{
	pthread_mutex_unlock(&m_lock);
}

void OMXClock::SetClockPorts(OMX_TIME_CONFIG_CLOCKSTATETYPE* clock, bool hasVideo, bool hasAudio)
{
	if (!GetComponent())
		return;

	if (!clock)
		return;

	clock->nWaitMask = 0;

	if (hasAudio)
		clock->nWaitMask |= OMX_CLOCKPORT0;

	if (hasVideo)
		clock->nWaitMask |= OMX_CLOCKPORT1;
}

bool OMXClock::SetReferenceClock(bool hasAudio, bool lock)
{
	if (lock)
		Lock();

	bool ret = true;
	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE refClock;
	OMX_INIT_STRUCTURE(refClock);

	if (hasAudio)
		refClock.eClock = OMX_TIME_RefClockAudio;
	else
		refClock.eClock = OMX_TIME_RefClockVideo;

	if (refClock.eClock != m_eClock)
	{
		omxErr = SetConfig(OMX_IndexConfigTimeActiveRefClock, &refClock);
		if (omxErr != OMX_ErrorNone)
		{
			ret = false;
		}
		m_eClock = refClock.eClock;
	}

	m_lastMediaTime = 0.0f;

	if (lock)
		Unlock();

	return ret;
}

bool OMXClock::Pause(bool lock)
{
	if (!GetComponent())
		return false;

	if (!m_pause)
	{
		if (lock)
			Lock();

		if (SetSpeed(0, false, true))
			m_pause = true;

		m_lastMediaTime = 0.0f;

		if (lock)
			Unlock();
	}

	return (m_pause);
}

bool OMXClock::Resume(bool lock)
{
	if (!GetComponent())
		return false;

	if (m_pause)
	{
		if (lock)
			Lock();

		if (SetSpeed(m_omxSpeed, false, true))
			m_pause = false;

		m_lastMediaTime = 0.0f;
		if (lock)
			Unlock();
	}

	return !(m_pause);
}

bool OMXClock::Stop(bool lock)
{
	if (!GetComponent())
		return false;

	if (lock)
		Lock();

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	OMX_TIME_CONFIG_CLOCKSTATETYPE clock;
	OMX_INIT_STRUCTURE(clock);

	clock.eState = OMX_TIME_ClockStateStopped;
	clock.nOffset = ToOMXTime(-1000LL * OMX_PRE_ROLL);

	omxErr = SetConfig(OMX_IndexConfigTimeClockState, &clock);
	if (omxErr != OMX_ErrorNone)
	{
		if (lock)
			Unlock();

		return false;
	}

	m_eState = clock.eState;

	m_lastMediaTime = 0.0f;

	if (lock)
		Unlock();

	return true;
}

bool OMXClock::Start(bool lock)
{
	if (!GetComponent())
		return false;

	if (lock)
		Lock();

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	OMX_TIME_CONFIG_CLOCKSTATETYPE clock;
	OMX_INIT_STRUCTURE(clock);

	clock.eState = OMX_TIME_ClockStateRunning;

	omxErr = SetConfig(OMX_IndexConfigTimeClockState, &clock);
	if (omxErr != OMX_ErrorNone)
	{
		if (lock)
			Unlock();

		return false;
	}

	m_eState = clock.eState;

	m_lastMediaTime = 0.0f;

	if (lock)
		Unlock();

	return true;
}

bool OMXClock::Step(int steps, bool lock)
{
	if (!GetComponent())
		return false;

	if (lock)
		Lock();

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	OMX_PARAM_U32TYPE param;
	OMX_INIT_STRUCTURE(param);

	param.nPortIndex = OMX_ALL;
	param.nU32 = steps;

	omxErr = SetConfig(OMX_IndexConfigSingleStep, &param);
	if (omxErr != OMX_ErrorNone)
	{
		if (lock)
			Unlock();

		return false;
	}

	m_lastMediaTime = 0.0f;
	if (lock)
		Unlock();

	return true;
}

bool OMXClock::Reset(bool hasVideo, bool hasAudio, bool lock)
{
	if (!GetComponent())
		return false;

	if (lock)
		Lock();

	if (!SetReferenceClock(hasAudio, false))
	{
		if (lock)
			Unlock();

		return false;
	}

	if (m_eState == OMX_TIME_ClockStateStopped)
	{
		OMX_TIME_CONFIG_CLOCKSTATETYPE clock;
		OMX_INIT_STRUCTURE(clock);

		clock.eState = OMX_TIME_ClockStateWaitingForStartTime;
		clock.nOffset = ToOMXTime(-1000LL * OMX_PRE_ROLL);

		SetClockPorts(&clock, hasVideo, hasAudio);

		if (clock.nWaitMask)
		{
			OMX_ERRORTYPE omxErr = SetConfig(OMX_IndexConfigTimeClockState, &clock);
			if (omxErr != OMX_ErrorNone)
			{
				if (lock)
					Unlock();

				return false;
			}

			if (m_eState != OMX_TIME_ClockStateStopped)
				m_waitMask = clock.nWaitMask;

			m_eState = clock.eState;
		}
	}

	m_lastMediaTime = 0.0f;
	if (lock)
		Unlock();

	return true;
}

double OMXClock::GetMediaTime(bool lock)
{
	if (!GetComponent())
	{
		return 0.0;
	}

	double pts = 0.0;

	double now = GetAbsoluteClock();
	if (((now - m_lastMediaTimeRead) > CLOCK_MSEC_TO_TIME(100)) || (m_lastMediaTime == 0.0))
	{
		if (lock)
			Lock();

		OMX_ERRORTYPE omxErr = OMX_ErrorNone;

		OMX_TIME_CONFIG_TIMESTAMPTYPE timestamp;
		OMX_INIT_STRUCTURE(timestamp);
		timestamp.nPortIndex = GetInputPort();

		omxErr = GetConfig(OMX_IndexConfigTimeCurrentMediaTime, &timestamp);
		if (omxErr != OMX_ErrorNone)
		{
			if (lock)
				Unlock();

			return 0.0;
		}

		pts = (double)FromOMXTime(timestamp.nTimestamp);

		//OMXLogger::OutputLog( "OMXClock::GetMediaTime - %.2f (%.2f, %.2f)", LogLevel::Debug, pts, m_lastMediaTime, now - m_lastMediaTime );

		m_lastMediaTime = pts;
		m_lastMediaTimeRead = now;

		if (lock)
			Unlock();
	}
	else
	{
		double speed = m_pause ? 0.0 : (double)m_omxSpeed / CLOCK_PLAYSPEED_NORMAL;
		pts = m_lastMediaTime + (now - m_lastMediaTimeRead) * speed;

		//OMXLogger::OutputLog( "OMXClock::GetMediaTime - cached %.2f (%.2f, %.2f)", LogLevel::Debug, pts, m_lastMediaTime, now - m_lastMediaTime );
	}

	//OMXLogger::OutputLog( "OMXClock::GetMediaTime - Returning PTS %f", LogLevel::Debug, pts );

	return pts;
}

double OMXClock::ClockAdjustment(bool lock)
{
	if (!GetComponent())
		return 0.0;

	if (lock)
		Lock();

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	double pts = 0.0;

	OMX_TIME_CONFIG_TIMESTAMPTYPE timestamp;
	OMX_INIT_STRUCTURE(timestamp);
	timestamp.nPortIndex = GetInputPort();

	omxErr = GetConfig(OMX_IndexConfigClockAdjustment, &timestamp);
	if (omxErr != OMX_ErrorNone)
	{

		if (lock)
			Unlock();

		return 0.0;
	}

	pts = (double)FromOMXTime(timestamp.nTimestamp);

	if (lock)
		Unlock();

	return pts;
}

bool OMXClock::SetMediaTime(double pts, bool lock)
{
	if (!GetComponent())
		return false;

	if (lock)
		Lock();

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	OMX_INDEXTYPE index;
	OMX_TIME_CONFIG_TIMESTAMPTYPE timestamp;
	OMX_INIT_STRUCTURE(timestamp);
	timestamp.nPortIndex = GetInputPort();

	if (m_eClock == OMX_TIME_RefClockAudio)
		index = OMX_IndexConfigTimeCurrentAudioReference;
	else
		index = OMX_IndexConfigTimeCurrentVideoReference;

	timestamp.nTimestamp = ToOMXTime(pts);

	omxErr = SetConfig(index, &timestamp);
	if (omxErr != OMX_ErrorNone)
	{
		if (lock)
			Unlock();

		return false;
	}

	m_lastMediaTime = 0.0f;

	if (lock)
		Unlock();

	return true;
}

bool OMXClock::SetSpeed(int speed, bool lock, bool pause_resume)
{
	if (!GetComponent())
		return false;

	if (lock)
		Lock();

	if (pause_resume)
	{
		OMX_ERRORTYPE omxErr = OMX_ErrorNone;
		OMX_TIME_CONFIG_SCALETYPE scaleType;
		OMX_INIT_STRUCTURE(scaleType);

		if (TP(speed))
			scaleType.xScale = 0;
		else
			scaleType.xScale = (speed << 16) / CLOCK_PLAYSPEED_NORMAL;

		omxErr = SetConfig(OMX_IndexConfigTimeScale, &scaleType);
		if (omxErr != OMX_ErrorNone)
		{
			if (lock)
				Unlock();

			return false;
		}
	}
	else
		m_omxSpeed = speed;

	m_lastMediaTime = 0.0f;

	if (lock)
		Unlock();

	return true;
}

bool OMXClock::StateExecute(bool lock)
{
	if (!GetComponent())
		return false;

	if (lock)
		Lock();

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;

	if (GetState() != OMX_StateExecuting)
	{
		StateIdle(false);

		omxErr = SetStateForComponent(OMX_StateExecuting);
		if (omxErr != OMX_ErrorNone)
		{
			if (lock)
				Unlock();

			return false;
		}
	}

	m_lastMediaTime = 0.0f;
	if (lock)
		Unlock();

	return true;
}


void OMXClock::StateIdle(bool lock)
{
	if (!GetComponent())
		return;

	if (lock)
		Lock();

	if (GetState() != OMX_StateIdle)
		SetStateForComponent(OMX_StateIdle);

	m_lastMediaTime = 0.0f;

	if (lock)
		Unlock();
}

bool OMXClock::HDMIClockSync(bool lock)
{
	if (!GetComponent())
		return false;

	if (lock)
		Lock();

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	OMX_CONFIG_LATENCYTARGETTYPE latencyTarget;
	OMX_INIT_STRUCTURE(latencyTarget);

	latencyTarget.nPortIndex = OMX_ALL;
	latencyTarget.bEnabled = OMX_TRUE;
	latencyTarget.nFilter = 10;
	latencyTarget.nTarget = 0;
	latencyTarget.nShift = 3;
	latencyTarget.nSpeedFactor = -60;
	latencyTarget.nInterFactor = 100;
	latencyTarget.nAdjCap = 100;

	omxErr = SetConfig(OMX_IndexConfigLatencyTarget, &latencyTarget);
	if (omxErr != OMX_ErrorNone)
	{
		if (lock)
			Unlock();

		return false;
	}

	m_lastMediaTime = 0.0f;
	if (lock)
		Unlock();

	return true;
}

static int64_t CurrentHostCounter(void)
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (((int64_t)now.tv_sec * 1000000000L) + now.tv_nsec);
}

uint64_t OMXClock::GetAbsoluteClock(void) const
{
	return CurrentHostCounter() / 1000;
}

double OMXClock::GetClock(void) const
{
	return GetAbsoluteClock();
}

void OMXClock::Sleep(unsigned int milliseconds)
{
	struct timespec req;
	req.tv_sec = milliseconds / 1000;
	req.tv_nsec = (milliseconds % 1000) * 1000000;

	while ((nanosleep(&req, &req) == -1) && (errno == EINTR) && ((req.tv_nsec > 0) || (req.tv_sec > 0)));
}