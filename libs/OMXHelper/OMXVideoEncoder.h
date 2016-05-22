#pragma once

#include "OMXCore.h"

class OMXVideoEncoder
{
public:
	OMXVideoEncoder();
	~OMXVideoEncoder();
	
	bool Open();

	void SetFrameInfo(unsigned int width, unsigned int height, unsigned int framerate, OMX_U32 bitrate);
	void SetBitrate(OMX_U32 bitrate);
	void SetOutputFormat(OMX_VIDEO_CODINGTYPE type);
	void SetAVCProfile( OMX_VIDEO_AVCPROFILETYPE type );

	void AllocateBuffers();

	void Execute();

	bool SetupOutputTunnel(OMXCoreComponent* component, OMX_U32 dstPort);

public:
	OMXCoreComponent* GetComponent() const {
		return m_omxEncoder;
	}

private:
	OMXCoreComponent* m_omxEncoder;

	OMXCoreTunnel* m_omxTunnelOutput;
};

