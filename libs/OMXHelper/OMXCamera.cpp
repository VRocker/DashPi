#include "OMXCamera.h"



OMXCamera::OMXCamera()
{
	m_omxCamera = nullptr;
	m_clock = nullptr;
	m_omxTunnelClock = nullptr;
}


OMXCamera::~OMXCamera()
{
	if (m_omxTunnelPreview)
	{
		m_omxTunnelPreview->Deestablish();
		delete m_omxTunnelPreview;
		m_omxTunnelPreview = nullptr;
	}

	if (m_omxTunnelCapture)
	{
		m_omxTunnelCapture->Deestablish();
		delete m_omxTunnelCapture;
		m_omxTunnelCapture = nullptr;
	}

	if (m_omxTunnelClock)
	{
		m_omxTunnelClock->Deestablish();
		delete m_omxTunnelClock;
		m_omxTunnelClock = nullptr;
	}

	if (m_omxCamera)
	{
		m_omxCamera->Deinitialise();
		delete m_omxCamera;
		m_omxCamera = nullptr;
	}

	m_clock = nullptr;
}

bool OMXCamera::Open(OMXClock* clock)
{
	if (!clock)
		return false;

	if (!clock->GetComponent())
		return false;

	m_omxCamera = new OMXCoreComponent();

	if (!m_omxCamera->Initialise("OMX.broadcom.camera", OMX_IndexParamVideoInit))
	{
		// Failed
		delete m_omxCamera;
		m_omxCamera = nullptr;

		return false;
	}

	m_clock = clock;

	/*m_omxTunnelClock = new OMXCoreTunnel();
	m_omxTunnelClock->Init(m_clock, m_clock->GetOutputPort(), m_omxCamera, m_omxCamera->GetInputPort());

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	omxErr = m_omxTunnelClock->Establish(false);
	if (omxErr != OMX_ErrorNone)
	{
		// Failed to establish tunnel

		delete m_omxTunnelClock;
		m_omxTunnelClock = nullptr;

		return false;
	}*/

	return true;
}

void OMXCamera::SetFrameInfo(unsigned int width, unsigned int height, unsigned int framerate)
{
	if (m_omxCamera)
	{
		OMX_ERRORTYPE omxErr = OMX_ErrorNone;

		OMX_PARAM_PORTDEFINITIONTYPE portDef;
		OMX_INIT_STRUCTURE(portDef);
		portDef.nPortIndex = m_omxCamera->GetOutputPort();

		if ((omxErr = m_omxCamera->GetParameter(OMX_IndexParamPortDefinition, &portDef)) != OMX_ErrorNone)
		{
			// Unable to get port defs
		}

		portDef.format.video.nFrameWidth = width;
		portDef.format.video.nFrameHeight = height;
		portDef.format.video.xFramerate = framerate << 16;
		portDef.format.video.nStride = (portDef.format.video.nFrameWidth + portDef.nBufferAlignment - 1) & (~(portDef.nBufferAlignment - 1));
		portDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;

		if ((omxErr = m_omxCamera->SetParameter(OMX_IndexParamPortDefinition, &portDef)) != OMX_ErrorNone)
		{
			// Unable to set port def
		}

		portDef.nPortIndex = m_omxCamera->GetOutputPort();
		if ((omxErr = m_omxCamera->GetParameter(OMX_IndexParamPortDefinition, &portDef)) != OMX_ErrorNone)
		{
			// Unable to get port defs
		}
		portDef.nPortIndex = m_omxCamera->GetOutputPort() + 1; // Port 71
		if ((omxErr = m_omxCamera->SetParameter(OMX_IndexParamPortDefinition, &portDef)) != OMX_ErrorNone)
		{
			// Unable to set port def
		}

		OMX_CONFIG_FRAMERATETYPE frameratePortSet;
		OMX_INIT_STRUCTURE(frameratePortSet);
		frameratePortSet.nPortIndex = m_omxCamera->GetOutputPort();
		frameratePortSet.xEncodeFramerate = portDef.format.video.xFramerate;
		if ((omxErr = m_omxCamera->SetConfig(OMX_IndexConfigVideoFramerate, &frameratePortSet)) != OMX_ErrorNone)
		{

		}
		frameratePortSet.nPortIndex = 71;
		if ((omxErr = m_omxCamera->SetConfig(OMX_IndexConfigVideoFramerate, &frameratePortSet)) != OMX_ErrorNone)
		{
		}
	}
}

void OMXCamera::SetRotation(OMX_S32 deg)
{
	if (m_omxCamera)
	{
		OMX_CONFIG_ROTATIONTYPE rot;
		OMX_INIT_STRUCTURE(rot);
		rot.nPortIndex = 71;
		rot.nRotation = deg;

		OMX_ERRORTYPE omxErr = m_omxCamera->SetConfig(OMX_IndexConfigCommonRotate, &rot);
		if (omxErr != OMX_ErrorNone)
		{
		}
	}
}

void OMXCamera::SetMirror(OMX_MIRRORTYPE eMirror)
{
	if (m_omxCamera)
	{
		OMX_CONFIG_MIRRORTYPE mirror;
		OMX_INIT_STRUCTURE(mirror);
		mirror.nPortIndex = 71;
		mirror.eMirror = eMirror;
		
		OMX_ERRORTYPE omxErr = m_omxCamera->SetConfig(OMX_IndexConfigCommonMirror, &mirror);
		if (omxErr != OMX_ErrorNone)
		{

		}
	}
}

bool OMXCamera::SetupPreviewTunnel(OMXCoreComponent * component, OMX_U32 dstPort)
{
	m_omxTunnelPreview = new OMXCoreTunnel();
	m_omxTunnelPreview->Init(m_omxCamera, 70, component, dstPort);

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	omxErr = m_omxTunnelPreview->Establish(false);
	if (omxErr != OMX_ErrorNone)
	{
		// Failed to establish tunnel

		delete m_omxTunnelPreview;
		m_omxTunnelPreview = nullptr;

		return false;
	}

	return true;
}

bool OMXCamera::SetupCaptureTunnel(OMXCoreComponent * component, OMX_U32 dstPort)
{
	m_omxTunnelCapture = new OMXCoreTunnel();
	m_omxTunnelCapture->Init(m_omxCamera, 71, component, dstPort);

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	omxErr = m_omxTunnelCapture->Establish(false);
	if (omxErr != OMX_ErrorNone)
	{
		// Failed to establish tunnel

		delete m_omxTunnelCapture;
		m_omxTunnelCapture = nullptr;

		return false;
	}

	return true;
}

void OMXCamera::StopCaptureTunnel()
{
	if (m_omxTunnelCapture)
	{
		m_omxTunnelCapture->Deestablish();
		delete m_omxTunnelCapture;
		m_omxTunnelCapture = nullptr;
	}
}

#include <stdio.h>
void OMXCamera::Execute()
{
	if (m_omxCamera)
	{
		m_omxCamera->SetStateForComponent(OMX_StateExecuting);
	}
}

void OMXCamera::EnableCapture(bool enabled)
{
	if (m_omxCamera)
	{
		OMX_CONFIG_PORTBOOLEANTYPE capture;
		OMX_INIT_STRUCTURE(capture);
		capture.nPortIndex = 71;
		capture.bEnabled = (enabled) ? OMX_TRUE : OMX_FALSE;

		OMX_ERRORTYPE omxErr = m_omxCamera->SetParameter(OMX_IndexConfigPortCapturing, &capture);
		if (omxErr != OMX_ErrorNone)
		{
			printf("Error: %u\n", omxErr);
		}

		printf("Done\n");
	}
}
