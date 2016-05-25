#pragma once
/*
 *	OMXCore
 *	Based on OMXCore from OMXPlayer - Created by Team XBMC
 *	Rewritten by Craig Richards
*/

#ifdef __cplusplus
extern "C" {
#endif
#include "IL/OMX_Broadcom.h"
#include "interface/vcos/vcos.h"
#include "interface/vmcs_host/vchost.h"
#include "interface/vmcs_host/vc_tvservice.h"
#ifdef __cplusplus
}
#endif

#include <queue>

#define OMX_INIT_STRUCTURE( a ) \
	memset( &(a), 0, sizeof( a ) ); \
	(a).nSize = sizeof(a); \
	(a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
	(a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
	(a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
	(a).nVersion.s.nStep = OMX_VERSION_STEP

typedef struct omx_event
{
	OMX_EVENTTYPE eEvent;
	OMX_U32 nData1;
	OMX_U32 nData2;
} omx_event;

#define OMX_MAX_PORTS 10

class OMXCoreComponent;

class OMXCoreTunnel
{
public:
	OMXCoreTunnel()
		: m_srcComponent(nullptr), m_dstComponent(nullptr), m_srcPort(0), m_dstPort(0), m_portSettingsChanged(false)
	{
		pthread_mutex_init(&m_lock, NULL);
	}
	~OMXCoreTunnel()
	{
		Deestablish();

		pthread_mutex_destroy(&m_lock);
		m_srcComponent = nullptr;
		m_dstComponent = nullptr;
		m_srcPort = 0;
		m_dstPort = 0;
	}

	void Init(OMXCoreComponent* srcComponent, unsigned int srcPort, OMXCoreComponent* dstComponent, unsigned int dstPort);

	OMX_ERRORTYPE Flush();
	OMX_ERRORTYPE Establish(bool portSettingsChanged);
	OMX_ERRORTYPE Deestablish(bool noWait = false);

private:
	void Lock(void) { pthread_mutex_lock(&m_lock); }
	void Unlock(void) { pthread_mutex_unlock(&m_lock); }

private:
	OMXCoreComponent* m_srcComponent;
	OMXCoreComponent* m_dstComponent;
	unsigned int m_srcPort;
	unsigned int m_dstPort;

	pthread_mutex_t m_lock;

	bool m_portSettingsChanged;
};

class OMXCoreComponent
{
public:
	OMXCoreComponent();
	virtual ~OMXCoreComponent();

	bool Initialise(const char* componentName, OMX_INDEXTYPE index, OMX_CALLBACKTYPE* callbacks = nullptr);
	void Deinitialise(bool flush = true);
	bool IsInitialised(void) const { return (m_handle); }

	OMX_ERRORTYPE DisableAllPorts(void);

	void AddEvent(OMX_EVENTTYPE event, OMX_U32 nData1, OMX_U32 nData2);
	void RemoveEvent(OMX_EVENTTYPE event, OMX_U32 nData1, OMX_U32 nData2);
	OMX_ERRORTYPE WaitForEvent(OMX_EVENTTYPE event, OMX_S32 timeout = 300);
	OMX_ERRORTYPE WaitForCommand(OMX_U32 command, OMX_U32 nData2, OMX_S32 timeout = 2000);
	OMX_ERRORTYPE SetStateForComponent(OMX_STATETYPE state);
	OMX_STATETYPE GetState(void);

	OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE paramIndex, OMX_PTR paramStruct);
	OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE paramIndex, OMX_PTR paramStruct);
	OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE configIndex, OMX_PTR configStruct);
	OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE configIndex, OMX_PTR configStruct);
	OMX_ERRORTYPE SendCommand(OMX_COMMANDTYPE cmd, OMX_U32 cmdParam, OMX_PTR cmdParamData);

	OMX_ERRORTYPE EnablePort(unsigned int port, bool wait = true);
	OMX_ERRORTYPE DisablePort(unsigned int port, bool wait = true);

public:
	OMX_ERRORTYPE EmptyThisBuffer(OMX_BUFFERHEADERTYPE* omxBuffer);
	OMX_ERRORTYPE FillThisBuffer(OMX_BUFFERHEADERTYPE* omxBuffer);
	OMX_ERRORTYPE FreeOutputBuffer(OMX_BUFFERHEADERTYPE* omxBuffer);

	void FlushAll();
	void FlushInput();
	void FlushOutput();

	unsigned int GetInputBufferSize() const { return m_inputBufferCount * m_inputBufferSize; }
	unsigned int GetOutputBufferSize() const { return m_outputBufferCount * m_outputBufferSize; }

	unsigned int GetInputBufferSpace() const { return m_omxInputAvailable.size() * m_inputBufferSize; }
	unsigned int GetOutputBufferSpace() const { return m_omxOutputAvailable.size() * m_outputBufferSize; }

	OMX_BUFFERHEADERTYPE* GetInputBuffer(OMX_S32 timeout = 200);
	OMX_BUFFERHEADERTYPE* GetOutputBuffer(OMX_S32 timeout = 200);

	OMX_ERRORTYPE AllocInputBuffers(bool useBuffers = false);
	OMX_ERRORTYPE AllocOutputBuffers(bool useBuffers = false);

	OMX_ERRORTYPE FreeInputBuffers( bool wait );
	OMX_ERRORTYPE FreeOutputBuffers( bool wait );

	OMX_ERRORTYPE WaitForInputDone(OMX_S32 timeout = 200);
	OMX_ERRORTYPE WaitForOutputDone(OMX_S32 timeout = 200);

public:
	// Callback Routines

	// Delegates
	static OMX_ERRORTYPE DecoderEventHandlerCallback(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
	static OMX_ERRORTYPE DecoderEmptyBufferDoneCallback(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBuffer);
	static OMX_ERRORTYPE DecoderFillBufferDoneCallback(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE* pBufferHeader);

	// Callbacks
	OMX_ERRORTYPE DecoderEventHandler(OMX_HANDLETYPE hComponent, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
	OMX_ERRORTYPE DecoderEmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE* pBuffer);
	OMX_ERRORTYPE DecoderFillBufferDone(OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE* pBuffer);

public:
	OMX_HANDLETYPE GetComponent(void) const { return m_handle; }

	unsigned int GetInputPort() const { return m_inputPort; }
	unsigned int GetOutputPort() const { return m_outputPort; }

private:
	void Lock(void) { pthread_mutex_lock(&m_lock); }
	void Unlock(void) { pthread_mutex_unlock(&m_lock); }

private:
	OMX_HANDLETYPE m_handle;

	unsigned int m_inputPort;
	unsigned int m_outputPort;

	unsigned int m_inputAlignment;
	unsigned int m_inputBufferSize;
	unsigned int m_inputBufferCount;

	unsigned int m_outputAlignment;
	unsigned int m_outputBufferSize;
	unsigned int m_outputBufferCount;
	
	unsigned int m_portsEnabled[OMX_MAX_PORTS];

	OMX_CALLBACKTYPE m_callbacks;

	pthread_mutex_t m_lock;
	pthread_mutex_t m_omxEventMutex;
	pthread_mutex_t m_omxEosMutex;
	pthread_mutex_t m_omxInputMutex;
	pthread_mutex_t m_omxOutputMutex;

	pthread_cond_t m_inputBufferCond;
	pthread_cond_t m_outputBufferCond;
	pthread_cond_t m_omxEventCond;

	std::vector< omx_event > m_omxEvents;
	std::queue< OMX_BUFFERHEADERTYPE* > m_omxInputAvailable;
	std::vector< OMX_BUFFERHEADERTYPE* > m_omxInputBuffers;
	std::queue< OMX_BUFFERHEADERTYPE* > m_omxOutputAvailable;
	std::vector< OMX_BUFFERHEADERTYPE* > m_omxOutputBuffers;

	bool m_omxInputUseBuffers;
	bool m_omxOutputUseBuffers;

	bool m_exit;
	bool m_eos;
	bool m_flushInput;
	bool m_flushOutput;
};


