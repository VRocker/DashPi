#include "OMXVideoEncoder.h"
#include <stdio.h>

OMXVideoEncoder::OMXVideoEncoder()
{
	m_omxEncoder = nullptr;
	m_omxTunnelOutput = nullptr;
}


OMXVideoEncoder::~OMXVideoEncoder()
{
	if (m_omxTunnelOutput)
	{
		m_omxTunnelOutput->Deestablish();
		delete m_omxTunnelOutput;
		m_omxTunnelOutput = nullptr;
	}

	if (m_omxEncoder)
	{
		delete m_omxEncoder;
		m_omxEncoder = nullptr;
	}
}

bool OMXVideoEncoder::Open()
{
	m_omxEncoder = new OMXCoreComponent();

	if (!m_omxEncoder->Initialise("OMX.broadcom.video_encode", OMX_IndexParamVideoInit))
	{
		// Failed
		delete m_omxEncoder;
		m_omxEncoder = nullptr;

		return false;
	}


	return true;
}

void OMXVideoEncoder::SetFrameInfo(unsigned int width, unsigned int height, unsigned int framerate, OMX_U32 bitrate)
{
	if (m_omxEncoder)
	{
		OMX_ERRORTYPE omxErr = OMX_ErrorNone;

		OMX_PARAM_PORTDEFINITIONTYPE portDef;
		OMX_INIT_STRUCTURE(portDef);
		portDef.nPortIndex = m_omxEncoder->GetOutputPort();

		if ((omxErr = m_omxEncoder->GetParameter(OMX_IndexParamPortDefinition, &portDef)) != OMX_ErrorNone)
		{
			// Unable to get port defs
		}

		portDef.format.video.nFrameWidth = width;
		portDef.format.video.nFrameHeight = height;
		portDef.format.video.xFramerate = framerate << 16;
		portDef.format.video.nStride = (portDef.format.video.nFrameWidth + portDef.nBufferAlignment - 1) & (~(portDef.nBufferAlignment - 1));

		portDef.format.video.nBitrate = bitrate;

		if ((omxErr = m_omxEncoder->SetParameter(OMX_IndexParamPortDefinition, &portDef)) != OMX_ErrorNone)
		{
			// Unable to set port def
			printf("Failed to set port definition. (%u)\n", omxErr);
		}
	}
}

void OMXVideoEncoder::SetBitrate(OMX_U32 rate)
{
	if (m_omxEncoder)
	{
		OMX_ERRORTYPE omxErr = OMX_ErrorNone;

		OMX_VIDEO_PARAM_BITRATETYPE bitrate;
		OMX_INIT_STRUCTURE(bitrate);
		bitrate.nPortIndex = m_omxEncoder->GetOutputPort();

		if ((omxErr = m_omxEncoder->GetParameter(OMX_IndexParamVideoBitrate, &bitrate)) != OMX_ErrorNone)
		{
			// Unable to get port defs
		}

		bitrate.eControlRate = OMX_Video_ControlRateVariable;
		bitrate.nTargetBitrate = rate;

		if ((omxErr = m_omxEncoder->SetParameter(OMX_IndexParamVideoBitrate, &bitrate)) != OMX_ErrorNone)
		{
			printf("Failed to set bitrate. (%u)\n", omxErr);
		}
	}
}

void OMXVideoEncoder::SetOutputFormat(OMX_VIDEO_CODINGTYPE type)
{
	if (m_omxEncoder)
	{
		OMX_ERRORTYPE omxErr = OMX_ErrorNone;

		OMX_VIDEO_PARAM_PORTFORMATTYPE format;
		OMX_INIT_STRUCTURE(format);
		format.nPortIndex = m_omxEncoder->GetOutputPort();
		format.eCompressionFormat = type;

		if ((omxErr = m_omxEncoder->SetParameter(OMX_IndexParamVideoPortFormat, &format)) != OMX_ErrorNone)
		{
			printf("Failed to set output format. (%u)\n", omxErr);
		}
	}
}

void OMXVideoEncoder::SetAVCProfile(OMX_VIDEO_AVCPROFILETYPE type)
{
	if (m_omxEncoder)
	{
		OMX_ERRORTYPE omxErr = OMX_ErrorNone;

		OMX_VIDEO_PARAM_AVCTYPE avcPortDef;
		OMX_INIT_STRUCTURE(avcPortDef);
		avcPortDef.nPortIndex = m_omxEncoder->GetOutputPort();

		if ((omxErr = m_omxEncoder->GetParameter(OMX_IndexParamVideoAvc, &avcPortDef)) != OMX_ErrorNone)
		{
			// Unable to get port defs
		}

		avcPortDef.eProfile = type;

		if ((omxErr = m_omxEncoder->SetParameter(OMX_IndexParamVideoAvc, &avcPortDef)) != OMX_ErrorNone)
		{
			printf("Failed to set AVC Profile. (%u)\n", omxErr);
		}
	}
}

void OMXVideoEncoder::AllocateBuffers()
{
	if (m_omxEncoder)
	{
		m_omxEncoder->AllocOutputBuffers();
	}
}

void OMXVideoEncoder::Execute()
{
	if (m_omxEncoder)
	{
		
		m_omxEncoder->SetStateForComponent(OMX_StateExecuting);
	}
}

bool OMXVideoEncoder::SetupOutputTunnel(OMXCoreComponent * component, OMX_U32 dstPort)
{
	m_omxTunnelOutput = new OMXCoreTunnel();
	m_omxTunnelOutput->Init(m_omxEncoder, 201, component, dstPort);

	OMX_ERRORTYPE omxErr = OMX_ErrorNone;
	omxErr = m_omxTunnelOutput->Establish(false);
	if (omxErr != OMX_ErrorNone)
	{
		// Failed to establish tunnel

		delete m_omxTunnelOutput;
		m_omxTunnelOutput = nullptr;

		return false;
	}

	return true;
}
