#include "Main.h"
#include <stdio.h>

#include <bcm_host.h>
#include <IL/OMX_Core.h>

#include "../libs/OMXHelper/OMXCore.h"
#include "../libs/OMXHelper/OMXClock.h"
#include "../libs/OMXHelper/OMXCamera.h"
#include "../libs/OMXHelper/OMXVideoEncoder.h"
#include "../libs/OMXHelper/OMXNull.h"



}
}

int main()
{
	bcm_host_init();

	OMX_ERRORTYPE omxerr;
	printf("Initialising OpenMAX...");
	if ((omxerr = OMX_Init()) != OMX_ErrorNone)
	{
		printf("FAILED! Err: %u\n", omxerr);
		return 1;
	}
	else
		printf("OK!\n");

	printf("Hello Pi!\n");

	OMXClock* clock = new OMXClock();
	clock->Init();

	clock->StateIdle();
	clock->Stop();

	OMXCamera* camera = new OMXCamera();
	camera->Open(clock);


	camera->SetFrameInfo(1280, 720, 25);
	camera->SetRotation(180);

	OMXVideoEncoder* encoder = new OMXVideoEncoder();
	encoder->Open();


	encoder->SetFrameInfo(1280, 720, 25, 17000000);
	encoder->SetBitrate(17000000);
	encoder->SetOutputFormat(OMX_VIDEO_CodingAVC);
	encoder->SetAVCProfile(OMX_VIDEO_AVCProfileHigh);

	OMXNull* nullsink = new OMXNull();
	camera->SetupPreviewTunnel(nullsink, 240);
	OMXCoreComponent* encodingComponent = encoder->GetComponent();
	camera->SetupCaptureTunnel(encodingComponent, encodingComponent->GetInputPort());

	// Allocate the buffer AFTER setting up the tunnel else bad things ahppen
	encoder->AllocateBuffers();
	
	clock->Reset(true, false);
	clock->StateExecute();
	//clock->Start();

	encoder->Execute();
	camera->Execute();
	camera->EnableCapture(true);

	


	char fileName[255] = { 0 };
	time_t startTime = time(0);
	sprintf(fileName, "/recordings/%u/recording.%u.h264", startTime, startTime);
	
	char cmd[128] = { 0 };
	sprintf(cmd, "mkdir -p /recordings/%u/", startTime);
	system(cmd);

	FILE* outFile = fopen(fileName, "w+");

	OMX_BUFFERHEADERTYPE* buffer = nullptr;

	unsigned int i = 0;
	unsigned int initialFrames = 0;

	unsigned int headerByteCount = 0;
	char headerBytes[29] = { 0 };

	unsigned int bufferLen = 0;
	while (i < 10)
	{
		buffer = encodingComponent->GetOutputBuffer();
		if (buffer)
		{
			if (buffer->nFilledLen)
			{
				fwrite(buffer->pBuffer + buffer->nOffset, 1, buffer->nFilledLen, outFile);
				//printf("Read from output buffer and wrote to output file %d/%d\n", buffer->nFilledLen, buffer->nAllocLen);

				if (initialFrames < 2)
				{
					memcpy(headerBytes + headerByteCount, buffer->pBuffer, buffer->nFilledLen);

					headerByteCount += buffer->nFilledLen;
					++initialFrames;
				}
				bufferLen += buffer->nFilledLen;

				// 10MB
				if (bufferLen > 10485760)
				{
					fclose(outFile);

					sprintf(fileName, "/recordings/%u/recording.%u.h264", startTime, time(0));
					printf("Changing file to %s...\n", fileName);
					outFile = fopen(fileName, "w+");
					if (!outFile)
						break;

					// Write the headers to the file
					fwrite(headerBytes, 1, headerByteCount, outFile);

					bufferLen = 0;

					++i;
				}
				/*if (buffer->nFilledLen)
					printf("Got buffer!\n");*/
			}

			OMX_ERRORTYPE omxErr = encodingComponent->FillThisBuffer(buffer);
			if (omxErr == OMX_ErrorNone)
			{
				//printf("Fill request done.\n");
			}
		}
		usleep(1000);		
	}

	// Create the file list
	// Place the file list in the same dir as the recordings so we can pass that to ffmpeg
	sprintf(cmd, "(for f in /recordings/%u/*.h264; do echo \"file '$f'\"; done) > /recordings/%u/filelist.txt", startTime, startTime);
	system(cmd);

	/*system("ffmpeg -f concat -safe 0 -i /tmp/filelist.txt -vcodec copy recording.mkv");
	system("rm /tmp/filelist.txt");*/

	camera->StopCaptureTunnel();
	delete camera;
	delete encoder;
	
	delete clock;

	fclose(outFile);

	OMX_Deinit();
	bcm_host_deinit();
	return 0;
}
