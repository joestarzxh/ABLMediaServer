
/*
 *
 * ����ʹ�õ����������Ͷ�������
 */
#pragma once

#ifdef _WIN32
#ifdef   LIBMPEG_EXPORTS
#define CE_API __declspec(dllexport)
#define CE_APICALL  __stdcall
#else
#define CE_API __declspec(dllimport)
#define CE_APICALL  __stdcall
#endif 

#else
#define CE_API
#define CE_APICALL __attribute__((visibility("default")))
#endif



#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include <stdint.h>
#endif



#ifdef __ANDROID__       //android�ı��������Զ�ʶ�����Ϊ�档

#include <android/log.h>

#undef printf  
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "Car-eye-ffmpeg", __VA_ARGS__)


#define __STDC_CONSTANT_MACROS
#define CarEyeLog(...) __android_log_print(ANDROID_LOG_DEBUG, "Car-eye-ffmpeg", __VA_ARGS__)

#endif

 //error number
#define NO_ERROR1			0
#define PARAMTER_ERROR		1
#define NULL_MEMORY			2
#define MAX_FILTER_DESCR	512

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <functional>
#include <atomic>
#include <vector>
#include <string>
#include <memory>
#include <cstdio>
#include <map>

#define DebugPrintf(fmt,args,...)  printf("%s(%d)-%s -> " #fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##args);
typedef std::function<void(uint8_t* raw_data, const char* codecid, int raw_len, bool bKey, int nWidth, int nHeight, int64_t nTimeStamp)> VideoCllBack;
typedef std::function<void(uint8_t* raw_data, const char* codecid, int raw_len, int channels, int sample_rate, int bytes, int64_t nTimeStamp)>AudioCallBack;
typedef std::function<void(uint8_t* pcm, int datalen, int nSampleRate, int nChannel, int64_t nTimeStamp)> PcmCallBack;
typedef std::function<void(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp)> YuvCallBack;


// ����������������
#define CarEye_Encoder_Handle void*
 // ����������������
#define CarEye_Decoder_Handle void*
 // ˮӡ����������������
#define CarEye_OSD_Handle void*

 // �����Ƶ֡��С 1 second of 48khz 32bit audio
#define MAX_AUDIO_FRAME_SIZE 192000

 // ý��������Ͷ��� ��FFMPEG��һһ��Ӧ��H265�����������ⶨ����Ҫת��
typedef enum
{
	// �����б���
	CAREYE_CODEC_NONE = 0,
	// H264����
	CAREYE_CODEC_H264 = 27,
	// H265����
	CAREYE_CODEC_H265 = 173,
	// MJPEG����
	CAREYE_CODEC_MJPEG = 7,
	// MPEG4����
	CAREYE_CODEC_MPEG4 = 12,

	// G711 Ulaw���� ��ӦFFMPEG�е�AV_CODEC_ID_PCM_MULAW����
	CAREYE_CODEC_G711U = 0x10006,
	// G711 Alaw���� ��ӦFFMPEG�е�AV_CODEC_ID_PCM_ALAW����
	CAREYE_CODEC_G711A = 0x10007,
	// G726���� ��ӦFFMPEG�е�AV_CODEC_ID_ADPCM_G726����
	CAREYE_CODEC_G726 = 0x1100B,
	CAREYE_CODEC_G726LE = 0x11804,

	// AAC����
	CAREYE_CODEC_AAC = 0x15002,
	// OPUS���� ��ӦFFMPEG�е�AV_CODEC_ID_OPUS����
	CAREYE_CODEC_OPUS = 0x1503C


}FFmpeg_CodecType;

enum DecodeType {
	kDecodeSoft = 0, //���
	kNvencDecode = 1, //Ӣΰ����� 
	kQsvDecode =2, //AMD����
	kJestonDecode = 3 //Ӣΰ����� 
};


// YUV��Ƶ����ʽ���壬��FFMPEG��һһ��Ӧ
typedef enum
{
	CAREYE_FMT_YUV420P = 0,///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
	CAREYE__FMT_YUV420P,   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
	CAREYE__FMT_YUYV422,   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
	CAREYE_FMT_RGB24=2,
	CAREYE_FMT_YUV422P = 4,
	CAREYE_FMT_YUV444P = 5,
	CAREYE_FMT_YUV410P = 6,
	CAREYE_FMT_YUV411P = 7,
	CAREYE_FMT_NV12 = 23
}FFMPEG_AVFormat;

// YUVý�����ṹ����
typedef struct
{
	// Y�������ݴ洢��
	unsigned char* Y;
	// Y���������ֽ���
	int YSize;
	// U�������ݴ洢��
	unsigned char* U;
	// U���������ֽ���
	int USize;
	// V�������ݴ洢��
	unsigned char* V;
	// V���������ֽ���
	int VSize;
}CarEye_YUVFrame;


// Supported rotation.
typedef enum  {
	kRotate0 = 0,      // No rotation.
	kRotate90 = 90,    // Rotate 90 degrees clockwise.
	kRotate180 = 180,  // Rotate 180 degrees.
	kRotate270 = 270,  // Rotate 270 degrees clockwise.

	// Deprecated.
	kRotateNone = 0,
	kRotateClockwise = 90,
	kRotateCounterClockwise = 270,
} RotationEnum;


#define MAX_STRING_LENGTH 1024
#define MAX_FILE_NAME 64
// ˮӡ�������
typedef struct
{
	// ��Ƶ���
	int Width;
	// ��Ƶ�߶�
	int Height;
	// ��Ƶ֡�ʣ�FPS��
	int FramesPerSecond;
	// ���ˮӡ����Ƶ��ʽ
	FFMPEG_AVFormat YUVType;
	// ˮӡ��ʼX������
	int X;
	// ˮӡ��ʼY������
	int Y;
	// ˮӡ�����С
	int FontSize;
	// 16���Ƶ�RGB��ɫֵ������ɫ��0x00FF00
	unsigned int FontColor;
	// ˮӡ͸���� 0~1
	float Transparency;
	// ˮӡ����
	char SubTitle[MAX_STRING_LENGTH];
	// �������ƣ������ļ��ŵ����ͬĿ¼�£��硰arial.ttf��
	// Windows��ϵͳĿ¼ʹ�ø�ʽ��"C\\\\:/Windows/Fonts/msyh.ttc"
	char FontName[MAX_FILE_NAME];
}CarEye_OSDParam;


struct I420Frame
{
	uint8_t* buffer;
	bool key_frame;
};

class H264Packet
{
public:
	H264Packet()
	{
	
	
	
	};
	~H264Packet()
	{
		if (data)
		{
			delete []data;
			data = nullptr;
		}
	

	};

	uint8_t* data=nullptr;
	int size;

};
struct  ACCHLSContext
{
	int64_t startReadPacktTime;
	int timeout;//��ʱʱ��
};

/*******************
//1 ��ʱ 0 �����ȴ�
********************/
#ifdef __cplusplus
extern "C"
{
#endif
	CE_API int  interruptCallBack(void* ctx);

	CE_API int interrupt_cb(void* context);


#ifdef __cplusplus
}
#endif


/*
* Comments: ʹ�ñ���֮ǰ�������һ�α�����
* Param : None
* @Return void
*/
#ifdef __cplusplus
extern "C"
{
#endif
	CE_API void CE_APICALL ffmpeg_init();

#ifdef __cplusplus
}
#endif




