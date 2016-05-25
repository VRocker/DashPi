#pragma once

#include "OMXCore.h"
#include "OMXClock.h"

class OMXCamera
{
public:
	OMXCamera();
	~OMXCamera();

	bool Open(OMXClock* clock);

	void SetFrameInfo(unsigned int width, unsigned int height, unsigned int framerate);
	void SetRotation(OMX_S32 deg);
	void SetMirror(OMX_MIRRORTYPE mirror);

	bool SetupPreviewTunnel(OMXCoreComponent* component, OMX_U32 dstPort);
	bool SetupCaptureTunnel(OMXCoreComponent* component, OMX_U32 dstPort);
	void StopCaptureTunnel();

	void Execute();
	void EnableCapture(bool enabled);

public:
	OMXCoreComponent* GetComponent() const {
		return m_omxCamera;
	}

private:
	OMXClock* m_clock;

	OMXCoreComponent* m_omxCamera;
	OMXCoreTunnel* m_omxTunnelClock;
	OMXCoreTunnel* m_omxTunnelPreview;
	OMXCoreTunnel* m_omxTunnelCapture;
};

