#include "Main.h"
#include <stdio.h>

#include <bcm_host.h>
#include <IL/OMX_Core.h>

#include "../libs/OMXHelper/OMXCore.h"
#include "../libs/OMXHelper/OMXClock.h"
#include "../libs/OMXHelper/OMXCamera.h"
#include "../libs/OMXHelper/OMXVideoEncoder.h"
#include "../libs/OMXHelper/OMXNull.h"

static const char* dump_compression_format(OMX_VIDEO_CODINGTYPE c) {
	char *f;
	switch (c) {
	case OMX_VIDEO_CodingUnused:     return "not used";
	case OMX_VIDEO_CodingAutoDetect: return "autodetect";
	case OMX_VIDEO_CodingMPEG2:      return "MPEG2";
	case OMX_VIDEO_CodingH263:       return "H.263";
	case OMX_VIDEO_CodingMPEG4:      return "MPEG4";
	case OMX_VIDEO_CodingWMV:        return "Windows Media Video";
	case OMX_VIDEO_CodingRV:         return "RealVideo";
	case OMX_VIDEO_CodingAVC:        return "H.264/AVC";
	case OMX_VIDEO_CodingMJPEG:      return "Motion JPEG";
	case OMX_VIDEO_CodingVP6:        return "VP6";
	case OMX_VIDEO_CodingVP7:        return "VP7";
	case OMX_VIDEO_CodingVP8:        return "VP8";
	case OMX_VIDEO_CodingYUV:        return "Raw YUV video";
	case OMX_VIDEO_CodingSorenson:   return "Sorenson";
	case OMX_VIDEO_CodingTheora:     return "OGG Theora";
	case OMX_VIDEO_CodingMVC:        return "H.264/MVC";

	default:
		f = (char*)calloc(23, sizeof(char));
		if (f == NULL) {
			printf("Failed to allocate memory");
		}
		snprintf(f, 23 * sizeof(char) - 1, "format type 0x%08x", c);
		return f;
	}
}
static const char* dump_color_format(OMX_COLOR_FORMATTYPE c) {
	char *f;
	switch (c) {
	case OMX_COLOR_FormatUnused:                 return "OMX_COLOR_FormatUnused: not used";
	case OMX_COLOR_FormatMonochrome:             return "OMX_COLOR_FormatMonochrome";
	case OMX_COLOR_Format8bitRGB332:             return "OMX_COLOR_Format8bitRGB332";
	case OMX_COLOR_Format12bitRGB444:            return "OMX_COLOR_Format12bitRGB444";
	case OMX_COLOR_Format16bitARGB4444:          return "OMX_COLOR_Format16bitARGB4444";
	case OMX_COLOR_Format16bitARGB1555:          return "OMX_COLOR_Format16bitARGB1555";
	case OMX_COLOR_Format16bitRGB565:            return "OMX_COLOR_Format16bitRGB565";
	case OMX_COLOR_Format16bitBGR565:            return "OMX_COLOR_Format16bitBGR565";
	case OMX_COLOR_Format18bitRGB666:            return "OMX_COLOR_Format18bitRGB666";
	case OMX_COLOR_Format18bitARGB1665:          return "OMX_COLOR_Format18bitARGB1665";
	case OMX_COLOR_Format19bitARGB1666:          return "OMX_COLOR_Format19bitARGB1666";
	case OMX_COLOR_Format24bitRGB888:            return "OMX_COLOR_Format24bitRGB888";
	case OMX_COLOR_Format24bitBGR888:            return "OMX_COLOR_Format24bitBGR888";
	case OMX_COLOR_Format24bitARGB1887:          return "OMX_COLOR_Format24bitARGB1887";
	case OMX_COLOR_Format25bitARGB1888:          return "OMX_COLOR_Format25bitARGB1888";
	case OMX_COLOR_Format32bitBGRA8888:          return "OMX_COLOR_Format32bitBGRA8888";
	case OMX_COLOR_Format32bitARGB8888:          return "OMX_COLOR_Format32bitARGB8888";
	case OMX_COLOR_FormatYUV411Planar:           return "OMX_COLOR_FormatYUV411Planar";
	case OMX_COLOR_FormatYUV411PackedPlanar:     return "OMX_COLOR_FormatYUV411PackedPlanar: Planes fragmented when a frame is split in multiple buffers";
	case OMX_COLOR_FormatYUV420Planar:           return "OMX_COLOR_FormatYUV420Planar: Planar YUV, 4:2:0 (I420)";
	case OMX_COLOR_FormatYUV420PackedPlanar:     return "OMX_COLOR_FormatYUV420PackedPlanar: Planar YUV, 4:2:0 (I420), planes fragmented when a frame is split in multiple buffers";
	case OMX_COLOR_FormatYUV420SemiPlanar:       return "OMX_COLOR_FormatYUV420SemiPlanar, Planar YUV, 4:2:0 (NV12), U and V planes interleaved with first U value";
	case OMX_COLOR_FormatYUV422Planar:           return "OMX_COLOR_FormatYUV422Planar";
	case OMX_COLOR_FormatYUV422PackedPlanar:     return "OMX_COLOR_FormatYUV422PackedPlanar: Planes fragmented when a frame is split in multiple buffers";
	case OMX_COLOR_FormatYUV422SemiPlanar:       return "OMX_COLOR_FormatYUV422SemiPlanar";
	case OMX_COLOR_FormatYCbYCr:                 return "OMX_COLOR_FormatYCbYCr";
	case OMX_COLOR_FormatYCrYCb:                 return "OMX_COLOR_FormatYCrYCb";
	case OMX_COLOR_FormatCbYCrY:                 return "OMX_COLOR_FormatCbYCrY";
	case OMX_COLOR_FormatCrYCbY:                 return "OMX_COLOR_FormatCrYCbY";
	case OMX_COLOR_FormatYUV444Interleaved:      return "OMX_COLOR_FormatYUV444Interleaved";
	case OMX_COLOR_FormatRawBayer8bit:           return "OMX_COLOR_FormatRawBayer8bit";
	case OMX_COLOR_FormatRawBayer10bit:          return "OMX_COLOR_FormatRawBayer10bit";
	case OMX_COLOR_FormatRawBayer8bitcompressed: return "OMX_COLOR_FormatRawBayer8bitcompressed";
	case OMX_COLOR_FormatL2:                     return "OMX_COLOR_FormatL2";
	case OMX_COLOR_FormatL4:                     return "OMX_COLOR_FormatL4";
	case OMX_COLOR_FormatL8:                     return "OMX_COLOR_FormatL8";
	case OMX_COLOR_FormatL16:                    return "OMX_COLOR_FormatL16";
	case OMX_COLOR_FormatL24:                    return "OMX_COLOR_FormatL24";
	case OMX_COLOR_FormatL32:                    return "OMX_COLOR_FormatL32";
	case OMX_COLOR_FormatYUV420PackedSemiPlanar: return "OMX_COLOR_FormatYUV420PackedSemiPlanar: Planar YUV, 4:2:0 (NV12), planes fragmented when a frame is split in multiple buffers, U and V planes interleaved with first U value";
	case OMX_COLOR_FormatYUV422PackedSemiPlanar: return "OMX_COLOR_FormatYUV422PackedSemiPlanar: Planes fragmented when a frame is split in multiple buffers";
	case OMX_COLOR_Format18BitBGR666:            return "OMX_COLOR_Format18BitBGR666";
	case OMX_COLOR_Format24BitARGB6666:          return "OMX_COLOR_Format24BitARGB6666";
	case OMX_COLOR_Format24BitABGR6666:          return "OMX_COLOR_Format24BitABGR6666";
	case OMX_COLOR_Format32bitABGR8888:          return "OMX_COLOR_Format32bitABGR8888";
	case OMX_COLOR_Format8bitPalette:            return "OMX_COLOR_Format8bitPalette";
	case OMX_COLOR_FormatYUVUV128:               return "OMX_COLOR_FormatYUVUV128";
	case OMX_COLOR_FormatRawBayer12bit:          return "OMX_COLOR_FormatRawBayer12bit";
	case OMX_COLOR_FormatBRCMEGL:                return "OMX_COLOR_FormatBRCMEGL";
	case OMX_COLOR_FormatBRCMOpaque:             return "OMX_COLOR_FormatBRCMOpaque";
	case OMX_COLOR_FormatYVU420PackedPlanar:     return "OMX_COLOR_FormatYVU420PackedPlanar";
	case OMX_COLOR_FormatYVU420PackedSemiPlanar: return "OMX_COLOR_FormatYVU420PackedSemiPlanar";
	default:
		f = (char*)calloc(23, sizeof(char));
		if (f == NULL) {
			printf("Failed to allocate memory");
		}
		snprintf(f, 23 * sizeof(char) - 1, "format type 0x%08x", c);
		return f;
	}
}

static void dump_portdef(OMX_PARAM_PORTDEFINITIONTYPE* portdef) {
	printf("Port %d is %s, %s, buffers wants:%d needs:%d, size:%d, pop:%d, aligned:%d\n",
		portdef->nPortIndex,
		(portdef->eDir == OMX_DirInput ? "input" : "output"),
		(portdef->bEnabled == OMX_TRUE ? "enabled" : "disabled"),
		portdef->nBufferCountActual,
		portdef->nBufferCountMin,
		portdef->nBufferSize,
		portdef->bPopulated,
		portdef->nBufferAlignment);

	OMX_VIDEO_PORTDEFINITIONTYPE *viddef = &portdef->format.video;
	OMX_IMAGE_PORTDEFINITIONTYPE *imgdef = &portdef->format.image;
	switch (portdef->eDomain) {
	case OMX_PortDomainVideo:
		printf("Video type:\n"
			"\tWidth:\t\t%d\n"
			"\tHeight:\t\t%d\n"
			"\tStride:\t\t%d\n"
			"\tSliceHeight:\t%d\n"
			"\tBitrate:\t%d\n"
			"\tFramerate:\t%.02f\n"
			"\tError hiding:\t%s\n",
			viddef->nFrameWidth,
			viddef->nFrameHeight,
			viddef->nStride,
			viddef->nSliceHeight,
			viddef->nBitrate,
			((float)viddef->xFramerate / (float)65536),
			(viddef->bFlagErrorConcealment == OMX_TRUE ? "yes" : "no"));
		break;
	case OMX_PortDomainImage:
		printf("Image type:\n"
			"\tWidth:\t\t%d\n"
			"\tHeight:\t\t%d\n"
			"\tStride:\t\t%d\n"
			"\tSliceHeight:\t%d\n"
			"\tError hiding:\t%s\n",
			imgdef->nFrameWidth,
			imgdef->nFrameHeight,
			imgdef->nStride,
			imgdef->nSliceHeight,
			(imgdef->bFlagErrorConcealment == OMX_TRUE ? "yes" : "no"));
		break;
	default:
		break;
	}

}
static void dump_port(OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex, OMX_BOOL dumpformats) {
	OMX_ERRORTYPE r;
	OMX_PARAM_PORTDEFINITIONTYPE portdef;
	OMX_INIT_STRUCTURE(portdef);
	portdef.nPortIndex = nPortIndex;
	if ((r = OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &portdef)) != OMX_ErrorNone) {
		printf( "Failed to get port definition for port %d", nPortIndex);
	}
	dump_portdef(&portdef);
	if (dumpformats) {
		OMX_VIDEO_PARAM_PORTFORMATTYPE portformat;
		OMX_INIT_STRUCTURE(portformat);
		portformat.nPortIndex = nPortIndex;
		portformat.nIndex = 0;
		r = OMX_ErrorNone;
		printf("Port %d supports these video formats:\n", nPortIndex);
		while (r == OMX_ErrorNone) {
			if ((r = OMX_GetParameter(hComponent, OMX_IndexParamVideoPortFormat, &portformat)) == OMX_ErrorNone) {
				printf("\t%s, compression: %s\n", dump_color_format(portformat.eColorFormat), dump_compression_format(portformat.eCompressionFormat));
				portformat.nIndex++;
			}
		}
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

	dump_port(camera->GetComponent()->GetComponent(), 73, OMX_TRUE);
	dump_port(camera->GetComponent()->GetComponent(), 70, OMX_TRUE);
	dump_port(camera->GetComponent()->GetComponent(), 71, OMX_TRUE);

	camera->SetFrameInfo(1280, 720, 25);
	camera->SetRotation(180);

	OMXVideoEncoder* encoder = new OMXVideoEncoder();
	encoder->Open();

	dump_port(encoder->GetComponent()->GetComponent(), 200, OMX_TRUE);
	dump_port(encoder->GetComponent()->GetComponent(), 201, OMX_TRUE);

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

	

	printf("Camera port 73: \n");
	dump_port(camera->GetComponent()->GetComponent(), 73, OMX_TRUE);
	printf("Camera port 70: \n");
	dump_port(camera->GetComponent()->GetComponent(), 70, OMX_TRUE);
	printf("Camera port 71: \n");
	dump_port(camera->GetComponent()->GetComponent(), 71, OMX_TRUE);
	printf("Encoder port 200: \n");
	dump_port(encoder->GetComponent()->GetComponent(), 200, OMX_TRUE);
	printf("Encoder port 201: \n");
	dump_port(encoder->GetComponent()->GetComponent(), 201, OMX_TRUE);

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
