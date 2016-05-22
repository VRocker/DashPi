#include "OMXCore.h"
#include "Utils/MemUtils.h"

static void add_timespecs(struct timespec &time, long millisecs)
{
	long long nsec = time.tv_nsec + (long long)millisecs * 1000000;
	while (nsec > 1000000000)
	{
		time.tv_sec += 1;
		nsec -= 1000000000;
	}
	time.tv_nsec = nsec;
}

#pragma region Core Tunnel
void OMXCoreTunnel::Init(OMXCoreComponent * srcComponent, unsigned int srcPort, OMXCoreComponent * dstComponent, unsigned int dstPort)
{
	m_srcComponent = srcComponent;
	m_srcPort = srcPort;
	m_dstComponent = dstComponent;
	m_dstPort = dstPort;
}

OMX_ERRORTYPE OMXCoreTunnel::Flush()
{
	if ((!m_srcComponent) || (!m_dstComponent))
		return OMX_ErrorUndefined;

	Lock();

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;

	if (m_srcComponent->GetComponent())
	{
		omxErr = OMX_SendCommand(m_srcComponent->GetComponent(), OMX_CommandFlush, m_srcPort, NULL);
		if ((omxErr != OMX_ErrorNone) && (omxErr != OMX_ErrorSameState))
		{
			// Failed to flush port
		}
	}

	if (m_dstComponent->GetComponent())
	{
		omxErr = OMX_SendCommand(m_dstComponent->GetComponent(), OMX_CommandFlush, m_dstPort, NULL);
		if ((omxErr != OMX_ErrorNone) && (omxErr != OMX_ErrorSameState))
		{
			// Failed to flush port
		}
	}

	if (m_srcComponent->GetComponent())
		omxErr = m_srcComponent->WaitForCommand(OMX_CommandFlush, m_srcPort);

	if (m_dstComponent->GetComponent())
		omxErr = m_dstComponent->WaitForCommand(OMX_CommandFlush, m_dstPort);

	Unlock();

	return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXCoreTunnel::Establish(bool portSettingsChanged)
{
	if ((!m_srcComponent) || (!m_dstComponent))
		return OMX_ErrorUndefined;

	Lock();

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;

	if (m_srcComponent->GetState() == OMX_StateLoaded)
	{
		omxErr = m_srcComponent->SetStateForComponent(OMX_StateIdle);
		if (omxErr != OMX_ErrorNone)
		{
			Unlock();
			return omxErr;
		}
	}

	if (portSettingsChanged)
	{
		omxErr = m_srcComponent->WaitForEvent(OMX_EventPortSettingsChanged);
		if (omxErr != OMX_ErrorNone)
		{
			Unlock();
			return omxErr;
		}
	}

	if ( m_srcComponent->GetComponent() )
	{
		omxErr = m_srcComponent->DisablePort(m_srcPort, false);
		if ((omxErr != OMX_ErrorNone) && (omxErr != OMX_ErrorSameState))
		{
			// Error disabling port
		}
	}

	if (m_dstComponent->GetComponent())
	{
		omxErr = m_dstComponent->DisablePort(m_dstPort, false);
		if ((omxErr != OMX_ErrorNone) && (omxErr != OMX_ErrorSameState))
		{
			// Error disabling port
		}
	}
	
	if ((m_srcComponent->GetComponent()) && (m_dstComponent->GetComponent()))
	{
		omxErr = OMX_SetupTunnel(m_srcComponent->GetComponent(), m_srcPort, m_dstComponent->GetComponent(), m_dstPort);
		if (omxErr != OMX_ErrorNone)
		{
			// Couldn't setup tunnel
			Unlock();
			return omxErr;
		}
	}
	else
	{
		// Epic fail.
		Unlock();
		return OMX_ErrorUndefined;
	}

	if (m_srcComponent->GetComponent())
	{
		omxErr = m_srcComponent->EnablePort(m_srcPort, false);
		if (omxErr != OMX_ErrorNone)
		{
			// Error enabling port
			Unlock();
			return omxErr;
		}
	}

	if (m_dstComponent->GetComponent())
	{
		omxErr = m_dstComponent->EnablePort(m_dstPort, false);
		if (omxErr != OMX_ErrorNone)
		{
			// Error enabling port
			Unlock();
			return omxErr;
		}
	}

	if (m_dstComponent->GetComponent())
	{
		// Grab the state before waiting for command just incase it changes
		OMX_STATETYPE state = m_dstComponent->GetState();

		omxErr = m_dstComponent->WaitForCommand(OMX_CommandPortEnable, m_dstPort);
		if (omxErr != OMX_ErrorNone)
		{
			Unlock();
			return omxErr;
		}

		if (state == OMX_StateLoaded)
		{
			omxErr = m_dstComponent->SetStateForComponent(OMX_StateIdle);
			if (omxErr != OMX_ErrorNone)
			{
				// Error setting state to idle
				Unlock();
				return omxErr;
			}
		}
	}

	if (m_srcComponent->GetComponent())
	{
		omxErr = m_srcComponent->WaitForCommand(OMX_CommandPortEnable, m_srcPort);
		if (omxErr != OMX_ErrorNone)
		{
			Unlock();
			return omxErr;
		}
	}

	m_portSettingsChanged = portSettingsChanged;

	Unlock();

	return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXCoreTunnel::Deestablish(bool noWait)
{
	if ((!m_srcComponent) || (!m_dstComponent))
		return OMX_ErrorUndefined;

	Lock();

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;

	if ((m_srcComponent->GetComponent()) && (m_portSettingsChanged) && (!noWait))
	{
		omxErr = m_srcComponent->WaitForEvent(OMX_EventPortSettingsChanged);
		m_portSettingsChanged = false;
	}

	if (m_srcComponent->GetComponent())
	{
		omxErr = m_srcComponent->DisablePort(m_srcPort, false);
		if ((omxErr != OMX_ErrorNone) && (omxErr != OMX_ErrorSameState))
		{
			// Error disabling port
		}
	}

	if (m_dstComponent->GetComponent())
	{
		omxErr = m_dstComponent->DisablePort(m_dstPort, false);
		if ((omxErr != OMX_ErrorNone) && (omxErr != OMX_ErrorSameState))
		{
			// Error disabling port
		}
	}

	if (m_srcComponent->GetComponent())
	{
		omxErr = OMX_SetupTunnel(m_srcComponent->GetComponent(), m_srcPort, NULL, 0);
		if ((omxErr != OMX_ErrorNone) && (omxErr != OMX_ErrorIncorrectStateOperation))
		{
			// Failed to unset tunnel
		}
	}

	if (m_dstComponent->GetComponent())
	{
		omxErr = OMX_SetupTunnel(m_dstComponent->GetComponent(), m_dstPort, NULL, 0);
		if ((omxErr != OMX_ErrorNone) && (omxErr != OMX_ErrorIncorrectStateOperation))
		{
			// Failed to unset tunnel
		}
	}

	Unlock();

	return OMX_ErrorNone;
}
#pragma endregion

#pragma region Core Component
OMXCoreComponent::OMXCoreComponent()
{
	m_handle = NULL;
	m_inputPort = 0;
	m_outputPort = 0;

	m_inputAlignment = 0;
	m_inputBufferSize = 0;
	m_inputBufferCount = 0;

	m_outputAlignment = 0;
	m_outputBufferSize = 0;
	m_outputBufferCount = 0;

	m_exit = false;
	m_flushInput = false;
	m_flushOutput = false;

	pthread_mutex_init(&m_omxInputMutex, NULL);
	pthread_mutex_init(&m_omxOutputMutex, NULL);
	pthread_mutex_init(&m_omxEventMutex, NULL);
	pthread_mutex_init(&m_omxEosMutex, NULL);

	pthread_cond_init(&m_inputBufferCond, NULL);
	pthread_cond_init(&m_outputBufferCond, NULL);
	pthread_cond_init(&m_omxEventCond, NULL);

	for (unsigned int i = 0; i < OMX_MAX_PORTS; ++i)
		m_portsEnabled[i] = -1;

	pthread_mutex_init(&m_lock, NULL);
}

OMXCoreComponent::~OMXCoreComponent()
{
	Deinitialise();

	pthread_mutex_destroy(&m_omxInputMutex);
	pthread_mutex_destroy(&m_omxOutputMutex);
	pthread_mutex_destroy(&m_omxEventMutex);
	pthread_mutex_destroy(&m_omxEosMutex);

	pthread_cond_destroy(&m_inputBufferCond);
	pthread_cond_destroy(&m_outputBufferCond);
	pthread_cond_destroy(&m_omxEventCond);

	pthread_mutex_destroy(&m_lock);
}

bool OMXCoreComponent::Initialise(const char* componentName, OMX_INDEXTYPE index, OMX_CALLBACKTYPE* callbacks)
{
	OMX_ERRORTYPE omxErr = OMX_ErrorNone;

	if ((callbacks) && (callbacks->EventHandler))
		m_callbacks.EventHandler = callbacks->EventHandler;
	else
		m_callbacks.EventHandler = &OMXCoreComponent::DecoderEventHandlerCallback;

	if ((callbacks) && (callbacks->EmptyBufferDone))
		m_callbacks.EmptyBufferDone = callbacks->EmptyBufferDone;
	else
		m_callbacks.EmptyBufferDone = &OMXCoreComponent::DecoderEmptyBufferDoneCallback;

	if ((callbacks) && (callbacks->FillBufferDone))
		m_callbacks.FillBufferDone = callbacks->FillBufferDone;
	else
		m_callbacks.FillBufferDone = &OMXCoreComponent::DecoderFillBufferDoneCallback;

	omxErr = OMX_GetHandle(&m_handle, (char*)componentName, this, &m_callbacks);
	if (omxErr != OMX_ErrorNone)
	{
		// Failed to get component handle
		Deinitialise();
		return false;
	}

	OMX_PORT_PARAM_TYPE portParam;
	OMX_INIT_STRUCTURE(portParam);

	omxErr = OMX_GetParameter(m_handle, index, &portParam);
	if (omxErr != OMX_ErrorNone)
	{
		// Failed to get port param
	}

	omxErr = DisableAllPorts();
	if (omxErr != OMX_ErrorNone)
	{
		// Failed to disable all ports
	}

	m_inputPort = portParam.nStartPortNumber;
	m_outputPort = m_inputPort + 1;

	if (!strcmp(componentName, "OMX.broadcom.audio_mixer"))
	{
		m_inputPort = portParam.nStartPortNumber + 1;
		m_outputPort = portParam.nStartPortNumber;
	}

	if (!strcmp(componentName, "OMX.broadcom.camera"))
	{
		m_inputPort = portParam.nStartPortNumber + 3;
		m_outputPort = portParam.nStartPortNumber;
	}

	if (m_outputPort > portParam.nStartPortNumber + portParam.nPorts - 1)
		m_outputPort = portParam.nStartPortNumber + portParam.nPorts - 1;

	m_exit = false;
	m_flushInput = false;
	m_flushOutput = false;

	return true;
}

void OMXCoreComponent::Deinitialise(bool flush)
{
	OMX_ERRORTYPE omxErr = OMX_ErrorNone;

	m_exit = true;

	m_flushInput = true;
	m_flushOutput = true;

	if (m_handle)
	{
		if ( flush )
			FlushAll();

		if (GetState() == OMX_StateExecuting)
			SetStateForComponent(OMX_StatePause);

		if (GetState() != OMX_StateIdle)
			SetStateForComponent(OMX_StateIdle);

		FreeOutputBuffers(true);
		FreeInputBuffers(true);

		if (GetState() != OMX_StateIdle)
			SetStateForComponent(OMX_StateIdle);

		if (GetState() != OMX_StateLoaded)
			SetStateForComponent(OMX_StateLoaded);

		omxErr = OMX_FreeHandle(m_handle);
		if (omxErr != OMX_ErrorNone)
		{
			// Failed to free handle
		}
		m_handle = NULL;
	}

	m_inputPort = 0;
	m_outputPort = 0;

	for (unsigned int i = 0; i < OMX_MAX_PORTS; ++i)
		m_portsEnabled[i] = -1;

	m_omxEvents.clear();
}

OMX_ERRORTYPE OMXCoreComponent::EmptyThisBuffer(OMX_BUFFERHEADERTYPE* omxBuffer)
{
	OMX_ERRORTYPE omxErr = OMX_ErrorNone;

	if ((!m_handle) || (!omxBuffer))
		return OMX_ErrorUndefined;

	omxErr = OMX_EmptyThisBuffer(m_handle, omxBuffer);
	if (omxErr != OMX_ErrorNone)
	{
		// Failed to empty buffer
	}

	return omxErr;
}

OMX_ERRORTYPE OMXCoreComponent::FillThisBuffer(OMX_BUFFERHEADERTYPE* omxBuffer)
{
	OMX_ERRORTYPE omxErr = OMX_ErrorNone;

	if ((!m_handle) || (!omxBuffer))
		return OMX_ErrorUndefined;

	omxErr = OMX_FillThisBuffer(m_handle, omxBuffer);
	if (omxErr != OMX_ErrorNone)
	{
		// Failed to fill buffer
	}
	return omxErr;
}

OMX_ERRORTYPE OMXCoreComponent::FreeOutputBuffer(OMX_BUFFERHEADERTYPE* omxBuffer)
{
	OMX_ERRORTYPE omxErr = OMX_ErrorNone;

	if ((!m_handle) || (!omxBuffer))
		return OMX_ErrorUndefined;

	omxErr = OMX_FreeBuffer(m_handle, m_outputPort, omxBuffer);
	if (omxErr != OMX_ErrorNone)
	{
		// Failed to free output buffer
	}

	return omxErr;
}

void OMXCoreComponent::FlushAll()
{
	FlushInput();
	FlushOutput();
}

void OMXCoreComponent::FlushInput()
{
	Lock();

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	omxErr = OMX_SendCommand(m_handle, OMX_CommandFlush, m_inputPort, NULL);

	if (omxErr != OMX_ErrorNone)
	{
		// Failed to flush input
	}
	WaitForCommand(OMX_CommandFlush, m_inputPort);

	Unlock();
}

void OMXCoreComponent::FlushOutput()
{
	Lock();

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	omxErr = OMX_SendCommand(m_handle, OMX_CommandFlush, m_outputPort, NULL);

	if (omxErr != OMX_ErrorNone)
	{
		// Failed to flush input
	}
	WaitForCommand(OMX_CommandFlush, m_outputPort);

	Unlock();
}

OMX_BUFFERHEADERTYPE* OMXCoreComponent::GetInputBuffer(OMX_S32 timeout)
{
	if (!m_handle)
		return nullptr;

	OMX_BUFFERHEADERTYPE* inputBuffer = nullptr;

	pthread_mutex_lock(&m_omxInputMutex);
	struct timespec endtime;
	clock_gettime(CLOCK_REALTIME, &endtime);
	add_timespecs(endtime, timeout);

	while (!m_flushInput)
	{
		if (!m_omxInputAvailable.empty())
		{
			inputBuffer = m_omxInputAvailable.front();
			m_omxInputAvailable.pop();
			break;
		}

		int retcode = pthread_cond_timedwait(&m_inputBufferCond, &m_omxInputMutex, &endtime);
		if (retcode != 0)
		{
			// Timed out
			break;
		}
	}
	pthread_mutex_unlock(&m_omxInputMutex);
	return inputBuffer;
}

OMX_BUFFERHEADERTYPE* OMXCoreComponent::GetOutputBuffer(OMX_S32 timeout)
{
	if (!m_handle)
		return nullptr;

	OMX_BUFFERHEADERTYPE* outputBuffer = nullptr;

	pthread_mutex_lock(&m_omxOutputMutex);
	struct timespec endtime;
	clock_gettime(CLOCK_REALTIME, &endtime);
	add_timespecs(endtime, timeout);

	while (!m_flushOutput)
	{
		if (!m_omxOutputAvailable.empty())
		{
			outputBuffer = m_omxOutputAvailable.front();
			m_omxOutputAvailable.pop();
			break;
		}

		int retcode = pthread_cond_timedwait(&m_outputBufferCond, &m_omxOutputMutex, &endtime);
		if (retcode != 0)
		{
			// Timed out
			break;
		}
	}
	pthread_mutex_unlock(&m_omxOutputMutex);
	return outputBuffer;
}

OMX_ERRORTYPE OMXCoreComponent::AllocInputBuffers(bool useBuffers)
{
	if (!m_handle)
		return OMX_ErrorUndefined;

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;

	m_omxInputUseBuffers = useBuffers;

	OMX_PARAM_PORTDEFINITIONTYPE portFormat;
	OMX_INIT_STRUCTURE(portFormat);
	portFormat.nPortIndex = m_inputPort;

	omxErr = OMX_GetParameter(m_handle, OMX_IndexParamPortDefinition, &portFormat);
	if (omxErr != OMX_ErrorNone)
		return omxErr;

	{
		OMX_STATETYPE state = GetState();
		if (state != OMX_StateIdle)
		{
			if (state != OMX_StateLoaded)
				SetStateForComponent(OMX_StateLoaded);

			SetStateForComponent(OMX_StateIdle);
		}
	}

	omxErr = EnablePort(m_inputPort, false);
	if (omxErr != OMX_ErrorNone)
		return omxErr;

	if (GetState() == OMX_StateLoaded)
		SetStateForComponent(OMX_StateIdle);

	m_inputAlignment = portFormat.nBufferAlignment;
	m_inputBufferCount = portFormat.nBufferCountActual;
	m_inputBufferSize = portFormat.nBufferSize;

	for (OMX_U32 i = 0; i < portFormat.nBufferCountActual; ++i)
	{
		OMX_BUFFERHEADERTYPE* buffer = nullptr;
		OMX_U8* data = nullptr;

		if (m_omxInputUseBuffers)
		{
			data = (OMX_U8*)_aligned_malloc(portFormat.nBufferSize, m_inputAlignment);
			omxErr = OMX_UseBuffer(m_handle, &buffer, m_inputPort, NULL, portFormat.nBufferSize, data);
		}
		else
			omxErr = OMX_AllocateBuffer(m_handle, &buffer, m_inputPort, NULL, portFormat.nBufferSize);

		if (omxErr != OMX_ErrorNone)
		{
			if ((m_omxInputUseBuffers) && (data))
				_aligned_free(data);

			return omxErr;
		}

		buffer->nInputPortIndex = m_inputPort;
		buffer->nFilledLen = 0;
		buffer->nOffset = 0;
		buffer->pAppPrivate = (void*)i;
		m_omxInputBuffers.push_back(buffer);
		m_omxInputAvailable.push(buffer);
	}

	omxErr = WaitForCommand(OMX_CommandPortEnable, m_inputPort);

	m_flushInput = false;

	return omxErr;
}

OMX_ERRORTYPE OMXCoreComponent::AllocOutputBuffers(bool useBuffers)
{
	if (!m_handle)
		return OMX_ErrorUndefined;

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;

	m_omxOutputUseBuffers = useBuffers;

	OMX_PARAM_PORTDEFINITIONTYPE portFormat;
	OMX_INIT_STRUCTURE(portFormat);
	portFormat.nPortIndex = m_outputPort;

	omxErr = OMX_GetParameter(m_handle, OMX_IndexParamPortDefinition, &portFormat);
	if (omxErr != OMX_ErrorNone)
		return omxErr;

	{
		OMX_STATETYPE state = GetState();
		if (state != OMX_StateIdle)
		{
			if (state != OMX_StateLoaded)
				SetStateForComponent(OMX_StateLoaded);

			SetStateForComponent(OMX_StateIdle);
		}
	}

	omxErr = EnablePort(m_outputPort, false);
	if (omxErr != OMX_ErrorNone)
		return omxErr;

	if (GetState() == OMX_StateLoaded)
		SetStateForComponent(OMX_StateIdle);

	m_outputAlignment = portFormat.nBufferAlignment;
	m_outputBufferCount = portFormat.nBufferCountActual;
	m_outputBufferSize = portFormat.nBufferSize;

	for (OMX_U32 i = 0; i < portFormat.nBufferCountActual; ++i)
	{
		OMX_BUFFERHEADERTYPE* buffer = nullptr;
		OMX_U8* data = nullptr;

		if (m_omxOutputUseBuffers)
		{
			data = (OMX_U8*)_aligned_malloc(portFormat.nBufferSize, m_outputAlignment);
			omxErr = OMX_UseBuffer(m_handle, &buffer, m_outputPort, NULL, portFormat.nBufferSize, data);
		}
		else
			omxErr = OMX_AllocateBuffer(m_handle, &buffer, m_outputPort, NULL, portFormat.nBufferSize);

		if (omxErr != OMX_ErrorNone)
		{
			if ((m_omxOutputUseBuffers) && (data))
				_aligned_free(data);

			return omxErr;
		}

		buffer->nOutputPortIndex = m_outputPort;
		buffer->nFilledLen = 0;
		buffer->nOffset = 0;
		buffer->pAppPrivate = (void*)i;
		m_omxOutputBuffers.push_back(buffer);
		m_omxOutputAvailable.push(buffer);
	}

	omxErr = WaitForCommand(OMX_CommandPortEnable, m_outputPort);

	m_flushOutput = false;

	return omxErr;
}

OMX_ERRORTYPE OMXCoreComponent::FreeInputBuffers(bool wait)
{
	if (!m_handle)
		return OMX_ErrorUndefined;

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;

	if (m_omxInputBuffers.empty())
		return OMX_ErrorNone;

	m_flushInput = true;

	pthread_mutex_lock(&m_omxInputMutex);
	pthread_cond_broadcast(&m_inputBufferCond);

	omxErr = DisablePort(m_inputPort, wait);

	for (std::size_t i = 0; i < m_omxInputBuffers.size(); ++i)
	{
		OMX_U8* buf = m_omxInputBuffers[i]->pBuffer;

		omxErr = OMX_FreeBuffer(m_handle, m_inputPort, m_omxInputBuffers[i]);

		if ((m_omxInputUseBuffers) && (buf))
			_aligned_free(buf);
	}

	m_omxInputBuffers.clear();

	while (!m_omxInputAvailable.empty())
		m_omxInputAvailable.pop();

	m_inputAlignment = 0;
	m_inputBufferSize = 0;
	m_inputBufferCount = 0;

	pthread_mutex_unlock(&m_omxInputMutex);

	return omxErr;
}

OMX_ERRORTYPE OMXCoreComponent::FreeOutputBuffers(bool wait)
{
	if (!m_handle)
		return OMX_ErrorUndefined;

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;

	if (m_omxOutputBuffers.empty())
		return OMX_ErrorNone;

	m_flushOutput = true;

	pthread_mutex_lock(&m_omxOutputMutex);
	pthread_cond_broadcast(&m_outputBufferCond);

	omxErr = DisablePort(m_outputPort, wait);

	for (std::size_t i = 0; i < m_omxOutputBuffers.size(); ++i)
	{
		OMX_U8* buf = m_omxOutputBuffers[i]->pBuffer;

		omxErr = OMX_FreeBuffer(m_handle, m_outputPort, m_omxOutputBuffers[i]);

		if ((m_omxOutputUseBuffers) && (buf))
			_aligned_free(buf);
	}

	m_omxOutputBuffers.clear();

	while (!m_omxOutputAvailable.empty())
		m_omxOutputAvailable.pop();

	m_outputAlignment = 0;
	m_outputBufferSize = 0;
	m_outputBufferCount = 0;

	pthread_mutex_unlock(&m_omxOutputMutex);

	return omxErr;
}

OMX_ERRORTYPE OMXCoreComponent::DisableAllPorts()
{
	if (!m_handle)
		return OMX_ErrorUndefined;

	Lock();

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;

	OMX_INDEXTYPE idxTypes[] = {
		OMX_IndexParamAudioInit,
		OMX_IndexParamImageInit,
		OMX_IndexParamVideoInit,
		OMX_IndexParamOtherInit
	};

	OMX_PORT_PARAM_TYPE ports;
	OMX_INIT_STRUCTURE(ports);

	for (unsigned int i = 0; i < sizeof(idxTypes); ++i)
	{
		omxErr = OMX_GetParameter(m_handle, idxTypes[i], &ports);
		if (omxErr == OMX_ErrorNone)
		{
			for (OMX_U32 j = 0; j < ports.nPorts; ++j)
			{
				omxErr = OMX_SendCommand(m_handle, OMX_CommandPortDisable, ports.nStartPortNumber + j, NULL);
				if (omxErr != OMX_ErrorNone)
				{
					// Error disabling port
				}

				omxErr = WaitForCommand(OMX_CommandPortDisable, ports.nStartPortNumber + j);
				if ((omxErr != OMX_ErrorNone) && (omxErr != OMX_ErrorSameState))
				{
					Unlock();
					return omxErr;
				}
			}
		}
	}

	Unlock();

	return OMX_ErrorNone;
}

void OMXCoreComponent::RemoveEvent(OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2)
{
	std::vector< omx_event >::iterator iter = m_omxEvents.begin();
	std::vector< omx_event >::const_iterator iterEnd = m_omxEvents.end();
	for ( ; iter != iterEnd; ++iter )
	{
		omx_event event = *iter;

		if ((event.eEvent == eEvent) && (event.nData1 == nData1) && (event.nData2 == nData2))
		{
			m_omxEvents.erase(iter);
			return;
		}
	}
}

void OMXCoreComponent::AddEvent(OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2)
{
	omx_event event;

	event.eEvent = eEvent;
	event.nData1 = nData1;
	event.nData2 = nData2;

	pthread_mutex_lock(&m_omxEventMutex);
	RemoveEvent(eEvent, nData1, nData2);
	m_omxEvents.push_back(event);

	pthread_cond_broadcast(&m_omxEventCond);
	pthread_mutex_unlock(&m_omxEventMutex);
}

OMX_ERRORTYPE OMXCoreComponent::WaitForEvent(OMX_EVENTTYPE eventType, OMX_S32 timeout)
{
	pthread_mutex_lock(&m_omxEventMutex);
	struct timespec endtime;
	clock_gettime(CLOCK_REALTIME, &endtime);
	add_timespecs(endtime, timeout);

	while (true)
	{
		std::vector< omx_event >::iterator iter = m_omxEvents.begin();
		std::vector< omx_event >::const_iterator iterEnd = m_omxEvents.end();
		for (; iter != iterEnd; ++iter)
		{
			omx_event event = *iter;

			if ((event.eEvent == OMX_EventError) && (event.nData1 == (OMX_U32)OMX_ErrorSameState) && (event.nData2 == 1))
			{
				m_omxEvents.erase(iter);
				pthread_mutex_unlock(&m_omxEventMutex);
				return OMX_ErrorNone;
			}
			else if (event.eEvent == OMX_EventError)
			{
				m_omxEvents.erase(iter);
				pthread_mutex_unlock(&m_omxEventMutex);
				return (OMX_ERRORTYPE)event.nData1;
			}
			else if (event.eEvent == eventType)
			{
				m_omxEvents.erase(iter);
				pthread_mutex_unlock(&m_omxEventMutex);
				return OMX_ErrorNone;
			}
		}

		int retcode = pthread_cond_timedwait(&m_omxEventCond, &m_omxEventMutex, &endtime);
		if (retcode != 0)
		{
			pthread_mutex_unlock(&m_omxEventMutex);
			return OMX_ErrorTimeout;
		}
	}

	pthread_mutex_unlock(&m_omxEventMutex);
	return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXCoreComponent::WaitForCommand(OMX_U32 command, OMX_U32 nData2, OMX_S32 timeout)
{
	pthread_mutex_lock(&m_omxEventMutex);
	struct timespec endtime;
	clock_gettime(CLOCK_REALTIME, &endtime);
	add_timespecs(endtime, timeout);

	while (true)
	{
		std::vector< omx_event >::iterator iter = m_omxEvents.begin();
		std::vector< omx_event >::const_iterator iterEnd = m_omxEvents.end();
		for (; iter != iterEnd; ++iter)
		{
			omx_event event = *iter;

			if ((event.eEvent == OMX_EventError) && (event.nData1 == (OMX_U32)OMX_ErrorSameState) && (event.nData2 == 1))
			{
				m_omxEvents.erase(iter);
				pthread_mutex_unlock(&m_omxEventMutex);
				return OMX_ErrorNone;
			}
			else if (event.eEvent == OMX_EventError)
			{
				m_omxEvents.erase(iter);
				pthread_mutex_unlock(&m_omxEventMutex);
				return (OMX_ERRORTYPE)event.nData1;
			}
			else if ((event.eEvent == OMX_EventCmdComplete) && (event.nData1 == command) && (event.nData2 == nData2))
			{
				m_omxEvents.erase(iter);
				pthread_mutex_unlock(&m_omxEventMutex);
				return OMX_ErrorNone;
			}
		}

		int retcode = pthread_cond_timedwait(&m_omxEventCond, &m_omxEventMutex, &endtime);
		if (retcode != 0)
		{
			pthread_mutex_unlock(&m_omxEventMutex);
			return OMX_ErrorTimeout;
		}
	}

	pthread_mutex_unlock(&m_omxEventMutex);
	return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXCoreComponent::SetStateForComponent(OMX_STATETYPE state)
{
	if (!m_handle)
		return OMX_ErrorUndefined;

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	OMX_STATETYPE stateActual = OMX_StateMax;

	Lock();

	OMX_GetState(m_handle, &stateActual);
	if (state == stateActual)
	{
		Unlock();
		return OMX_ErrorNone;
	}

	omxErr = OMX_SendCommand(m_handle, OMX_CommandStateSet, state, 0);
	if (omxErr != OMX_ErrorNone)
	{
		if (omxErr == OMX_ErrorSameState)
			omxErr = OMX_ErrorNone;
	}
	else
	{
		omxErr = WaitForCommand(OMX_CommandStateSet, state);
		if (omxErr == OMX_ErrorSameState)
		{
			Unlock();
			return OMX_ErrorNone;
		}
	}

	Unlock();
	return omxErr;
}

OMX_STATETYPE OMXCoreComponent::GetState()
{
	if (!m_handle)
		return (OMX_STATETYPE)0;

	Lock();

	OMX_STATETYPE state;
	OMX_GetState(m_handle, &state);

	Unlock();

	return state;
}

OMX_ERRORTYPE OMXCoreComponent::SetParameter(OMX_INDEXTYPE paramIndex, OMX_PTR paramStruct)
{
	Lock();

	OMX_ERRORTYPE omxErr = OMX_SetParameter(m_handle, paramIndex, paramStruct);
	if (omxErr != OMX_ErrorNone)
	{
		// Fail
	}
	Unlock();

	return omxErr;
}

OMX_ERRORTYPE OMXCoreComponent::GetParameter(OMX_INDEXTYPE paramIndex, OMX_PTR paramStruct)
{
	Lock();

	OMX_ERRORTYPE omxErr = OMX_GetParameter(m_handle, paramIndex, paramStruct);
	if (omxErr != OMX_ErrorNone)
	{
		// Fail
	}

	Unlock();

	return omxErr;
}

OMX_ERRORTYPE OMXCoreComponent::SetConfig(OMX_INDEXTYPE configIndex, OMX_PTR configStruct)
{
	Lock();

	OMX_ERRORTYPE omxErr = OMX_SetConfig(m_handle, configIndex, configStruct);
	if (omxErr != OMX_ErrorNone)
	{
		// Fail
	}
	Unlock();

	return omxErr;
}
OMX_ERRORTYPE OMXCoreComponent::GetConfig(OMX_INDEXTYPE configIndex, OMX_PTR configStruct)
{
	Lock();

	OMX_ERRORTYPE omxErr = OMX_GetConfig(m_handle, configIndex, configStruct);
	if (omxErr != OMX_ErrorNone)
	{
		// Fail
	}

	Unlock();

	return omxErr;
}

OMX_ERRORTYPE OMXCoreComponent::SendCommand(OMX_COMMANDTYPE cmd, OMX_U32 cmdParam, OMX_PTR cmdParamData)
{
	Lock();

	OMX_ERRORTYPE omxErr = OMX_SendCommand(m_handle, cmd, cmdParam, cmdParamData);
	if (omxErr != OMX_ErrorNone)
	{
		// Fail
	}

	Unlock();

	return omxErr;
}

OMX_ERRORTYPE OMXCoreComponent::EnablePort(unsigned int port, bool wait)
{
	Lock();
	
	bool bEnabled = false;

	for (unsigned int i = 0; i < OMX_MAX_PORTS; ++i)
	{
		if (m_portsEnabled[i] == port)
		{
			bEnabled = true;
			break;
		}
	}
	
	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	
	if (!bEnabled)
	{
		omxErr = OMX_SendCommand(m_handle, OMX_CommandPortEnable, port, NULL);
		if (omxErr != OMX_ErrorNone)
		{
			Unlock();
			return omxErr;
		}
		else
		{
			if (wait)
				omxErr = WaitForEvent(OMX_EventCmdComplete);

			for (unsigned int i = 0; i < OMX_MAX_PORTS; ++i)
			{
				if (m_portsEnabled[i] == -1)
				{
					m_portsEnabled[i] = port;
					break;
				}
			}
		}
	}
	
	Unlock();
	return omxErr;
}

OMX_ERRORTYPE OMXCoreComponent::DisablePort(unsigned int port, bool wait)
{
	Lock();
	
	bool bEnabled = false;

	for (unsigned int i = 0; i < OMX_MAX_PORTS; ++i)
	{
		if (m_portsEnabled[i] == port)
		{
			bEnabled = true;
			break;
		}
	}
	
	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	
	if (!bEnabled)
	{
		omxErr = OMX_SendCommand(m_handle, OMX_CommandPortDisable, port, NULL);
		if (omxErr != OMX_ErrorNone)
		{
			Unlock();
			return omxErr;
		}
		else
		{
			if (wait)
				omxErr = WaitForEvent(OMX_EventCmdComplete);

			for (unsigned int i = 0; i < OMX_MAX_PORTS; ++i)
			{
				if (m_portsEnabled[i] == port)
				{
					m_portsEnabled[i] = -1;
					break;
				}
			}
		}
	}
	
	
	Unlock();
	return omxErr;
}

#pragma region Callbacks
OMX_ERRORTYPE OMXCoreComponent::DecoderEventHandlerCallback(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{
	if (!pAppData)
		return OMX_ErrorNone;

	OMXCoreComponent* comp = static_cast<OMXCoreComponent*>(pAppData);
	return comp->DecoderEventHandler(hComponent, eEvent, nData1, nData2, pEventData);
}

OMX_ERRORTYPE OMXCoreComponent::DecoderEmptyBufferDoneCallback(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE * pBuffer)
{
	if (!pAppData)
		return OMX_ErrorNone;

	OMXCoreComponent* comp = static_cast<OMXCoreComponent*>(pAppData);
	return comp->DecoderEmptyBufferDone(hComponent, pBuffer);
}

#include <stdio.h>
OMX_ERRORTYPE OMXCoreComponent::DecoderFillBufferDoneCallback(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE * pBufferHeader)
{
	if (!pAppData)
		return OMX_ErrorNone;

	OMXCoreComponent* comp = static_cast<OMXCoreComponent*>(pAppData);
	return comp->DecoderFillBufferDone(hComponent, pBufferHeader);
}

OMX_ERRORTYPE OMXCoreComponent::DecoderEmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE * pBuffer)
{
	if (m_exit)
		return OMX_ErrorNone;

	pthread_mutex_lock(&m_omxInputMutex);
	m_omxInputAvailable.push(pBuffer);

	pthread_cond_broadcast(&m_inputBufferCond);
	pthread_mutex_unlock(&m_omxInputMutex);

	return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXCoreComponent::DecoderFillBufferDone(OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE * pBuffer)
{
	if (m_exit)
		return OMX_ErrorNone;

	pthread_mutex_lock(&m_omxOutputMutex);
	m_omxOutputAvailable.push(pBuffer);

	pthread_cond_broadcast(&m_outputBufferCond);


	pthread_mutex_unlock(&m_omxOutputMutex);

	return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXCoreComponent::DecoderEventHandler(OMX_HANDLETYPE hComponent, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{
	AddEvent(eEvent, nData1, nData2);

	switch (eEvent)
	{
	case OMX_EventCmdComplete:
	{
		switch (nData1)
		{

		}
	}
	break;

	case OMX_EventBufferFlag:
	{
		if (nData2 & OMX_BUFFERFLAG_EOS)
		{
			m_eos = true;
		}
	}
	break;

	}

	return OMX_ErrorNone;
}
#pragma endregion

#pragma endregion