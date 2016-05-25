#include "Main.h"
#include <stdio.h>

#include <bcm_host.h>
#include <IL/OMX_Core.h>

#include "../libs/OMXHelper/OMXCore.h"
#include "../libs/OMXHelper/OMXClock.h"
#include "../libs/OMXHelper/OMXCamera.h"
#include "../libs/OMXHelper/OMXVideoEncoder.h"
#include "../libs/OMXHelper/OMXNull.h"

static bool g_shouldExit = false;

void exited()
{
	printf("Done recording...\n");
	OMX_Deinit();

	bcm_host_deinit();
}

void sig_handler(int signo)
{
	// Terminate the recording gracefully
	g_shouldExit = true;
}

void sighup_handler(int signo)
{
	// Flush to disk on HUP
}

int main()
{
	atexit(exited);
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

	signal(SIGTERM, sig_handler);
	signal(SIGINT, sig_handler);
	signal(SIGHUP, sighup_handler);

	printf("Pi Recorder - Version 1\n");
	printf("\tCreated by Craig Richards\n");

	char directory[255] = { 0 };
	time_t startTime;
	{
		struct tm* timeinfo;
		time(&startTime);
		timeinfo = localtime(&startTime);

		strftime(directory, sizeof(directory), "/recordings/%d-%m-%y %H-%M-%S/", timeinfo);
	}
	char fileName[255] = { 0 };
	sprintf(fileName, "%s/00000000-recording.h264", directory);

	char cmd[128] = { 0 };
	printf("Creating directory %s...\n", directory);
	sprintf(cmd, "mkdir -p \"%s\"", directory);
	system(cmd);

	FILE* outFile = fopen(fileName, "w+");
	if (!outFile)
	{
		printf("Failed to open initial file. Uber fail...\n");
		return 1;
	}

	OMXCamera* camera = new OMXCamera();
	camera->Open(nullptr);

	camera->SetFrameInfo(1920, 1080, 25);
	camera->SetRotation(180);

	OMXVideoEncoder* encoder = new OMXVideoEncoder();
	encoder->Open();

	encoder->SetFrameInfo(1920, 1080, 25, 25000000);
	encoder->SetBitrate(25000000);
	encoder->SetOutputFormat(OMX_VIDEO_CodingAVC);
	encoder->SetAVCProfile(OMX_VIDEO_AVCProfileHigh);

	OMXNull* nullsink = new OMXNull();
	camera->SetupPreviewTunnel(nullsink, 240);
	OMXCoreComponent* encodingComponent = encoder->GetComponent();
	camera->SetupCaptureTunnel(encodingComponent, encodingComponent->GetInputPort());

	// Allocate the buffer AFTER setting up the tunnel else bad things ahppen
	encoder->AllocateBuffers();
	
	encoder->Execute();
	camera->Execute();
	camera->EnableCapture(true);

	OMX_BUFFERHEADERTYPE* buffer = nullptr;

	unsigned int i = 0;
	unsigned int initialFrames = 0;

	unsigned int headerByteCount = 0;
	char headerBytes[29] = { 0 };

	unsigned int bufferLen = 0;
	bool bChangeFile = false;
	time(&startTime);

	while (true)
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

				// 50MB
				// Change file every 50MB at the keyframe
				if (bufferLen > 52428800)
					bChangeFile = true;

				if (bChangeFile)
				{
					if (buffer->nFlags & OMX_BUFFERFLAG_SYNCFRAME)
					{
						fclose(outFile);

						// File name is <sequence>-recording.h264
						sprintf(fileName, "%s/%.8u-recording.h264", directory, ++i);
						printf("Changing file to %s...\n", fileName);
						outFile = fopen(fileName, "w+");
						if (!outFile)
							break;

						// Write the headers to the file
						fwrite(headerBytes, 1, headerByteCount, outFile);

						bufferLen = 0;
						bChangeFile = false;
					}
				}
			}
			
			if (g_shouldExit)
			{
				// Wait for a keyframe before exiting
				if (buffer->nFlags & OMX_BUFFERFLAG_SYNCFRAME)
				{
					printf("Exit was requested and keyframe reached. Exiting main loop...\n");
					break;
				}
			}

			OMX_ERRORTYPE omxErr = encodingComponent->FillThisBuffer(buffer);
			if (omxErr == OMX_ErrorNone)
			{
				//printf("Fill request done.\n");
			}
		}
		/*else
		{
			if (g_shouldExit)
			{
				printf("Buffer no longer has data. Time to exit...\n");
				break;
			}
		}*/
		usleep(1000);		
	}

	// Disable capture on exit
	camera->EnableCapture(false);
	camera->StopPreviewTunnel();
	camera->StopCaptureTunnel();

	fclose(outFile);

	sprintf( cmd, "echo \"%u seconds\n\" > \"%s/length.txt\"", time(0) - startTime, directory);
	system(cmd);
	// Create the file list
	// Place the file list in the same dir as the recordings so we can pass that to ffmpeg
	sprintf(cmd, "(for f in \"%s\"*.h264; do echo \"file '$f'\"; done) > \"%s/filelist.txt\"", directory, directory);
	system(cmd);

	/*system("ffmpeg -f concat -safe 0 -i /tmp/filelist.txt -vcodec copy recording.mkv");
	system("rm /tmp/filelist.txt");*/
	
	delete camera;
	delete encoder;

	return 0;
}
