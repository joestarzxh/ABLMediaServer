
/*!
 * \file FFmpegDecoderAPI.h
 * \date 2021/09/02 15:48
 *
 * \author admin
 * Contact: user@company.com
 *
 * \brief
 *
 * TODO: long description
 *
 * \note
*/
#pragma once

#include "ffmpegTypes.h"
#include <string.h>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <queue>
#include <iostream>
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>

typedef std::function<void(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp)> FrameCallBackFunc;

typedef std::function<void(uint8_t* yuv, int nWidth, int nHeight, int strideY, int strideU, int strideV, bool bKey, int64_t nTimeStamp)> LocalYuvCallBackFunc;

typedef std::function<void(unsigned char* aPcm, int size, int64_t nTimeStamp)> LocalPcmCallBackFunc;

// �ص���������
using DecodedFrameCallback = std::function<void(uint8_t* buffer, int bufferSize, int fmt, int width, int height)>;




// ý��֡��Ϣ����
class FrameInfo
{
public:
	
	FrameInfo() {
	};
	~FrameInfo() {};

public:
	// ��Ƶ�����ʽ
	FFmpeg_CodecType VCodec= CAREYE_CODEC_NONE;
	// ��Ƶ�����ʽ������Ƶ����ΪCAREYE_CODEC_NONE
	FFmpeg_CodecType ACodec= CAREYE_CODEC_NONE;
	//������ʽ 
	DecodeType    DecType= DecodeType::kDecodeSoft;
	// ��Ƶ֡��(FPS)
	unsigned char	FramesPerSecond;
	// ��Ƶ�������
	unsigned short	Width=0;
	// ��Ƶ�ĸ߶�����
	unsigned short  Height=0;
	// ��Ƶ���ʣ�Խ����ƵԽ�������Ӧ���ҲԽ�� �磺4000000
	unsigned int	VideoBitrate=0;
	// ��Ƶ������
	unsigned int	SampleRate=0;
	// ��Ƶ������
	unsigned int	Channels=0;
	// ��Ƶ�������� 16λ 8λ�ȣ����ڲ��̶�Ϊ16λ
	unsigned int	BitsPerSample=16;
	// ��Ƶ������ �磺64000��Խ������Խ�������Ӧ���ҲԽ��
	unsigned int	AudioBitrate=0;

	unsigned int    bits_per_coded_sample;




};




class  FFmpegDecoder;

namespace H265
{
	enum NaluType : uint8_t {
		kIdr = 19,
		kSps = 33,
		kPps = 34,
		kSei = 39,
	};

	// Get the NAL type from the header byte immediately following start sequence.
	 NaluType   ParseNaluType(uint8_t data);

}




class  FFmpegDecoderAPI
{
public:
	class Frame
	{
	public:
		Frame() {

		};
		Frame(unsigned char* pBytes, int nSize, bool bKey, int64_t timestamp_ms) :
			m_nSize(nSize),
			m_timestamp_ms(timestamp_ms) {
			m_pBytes = static_cast<unsigned char*>(malloc(nSize));
			memcpy(m_pBytes, pBytes, m_nSize);
			m_bKey = bKey;

		}
		~Frame()
		{
			free(m_pBytes);
		};

		unsigned char* m_pBytes = nullptr;
		int64_t               m_timestamp_ms = 0;
		int					   m_nSize = 0;
		bool m_bKey = false;
	};


public:
	FFmpegDecoderAPI() {};

	FFmpegDecoderAPI(const std::map<std::string, std::string>& opts, bool wait)
	{
	}

	virtual ~FFmpegDecoderAPI()
	{
	}

	static FFmpegDecoderAPI* CreateDecoder(std::map<std::string, std::string> opts = {});


	virtual void Start()=0;

	/*
	* Comments: ����һ������������
	* Param aInfo: Ҫ�����ý����Ϣ
	* @Return �ɹ�����true�����򷵻�NULL
	*/
	virtual bool createDecoder(FrameInfo pFrameInfo)=0;

	virtual bool createDecoder(char* szCodecName, int nWidth, int nHeight)=0;

	/*
	* Comments: �ͷŽ�������Դ
	* Param aDecoder: Ҫ�ͷŵĽ�����
	* @Return None
	*/
	virtual void stopDecode()=0;

	virtual bool hasDecoder()=0;

	virtual int getYUVSize()=0;

	virtual bool CaptureJpegFromAVFrame(char* OutputFileName, int quality)=0;

	virtual void PostFrame(unsigned char* aBytes, int aSize, bool bKey, uint64_t ts) = 0;

	/*
	* Comments: ��������Ƶ����ΪYUV420��ʽ�������
	* Param aBytes: Ҫ���н������Ƶ��
	* Param aSize: Ҫ������Ƶ���ֽ���
	* Param aYuv: [���] ����ɹ��������YUV420����
	* @Return int < 0����ʧ�ܣ�> 0Ϊ�����YUV420���ֽڸ��� ==0��ʾ������Ч
	*/
	virtual int DecoderYUV420(unsigned char* aBytes, int aSize, unsigned char* aYuv)=0;

	/*
* Comments: ��������Ƶ����ΪPCM��ʽ�������
* Param aDecoder: ���뵽����Ч������
* Param aBytes: Ҫ���н������Ƶ��
* Param aSize: Ҫ������Ƶ���ֽ���
* Param aYuv: [���] ����ɹ��������PCM����
* @Return int < 0����ʧ�ܣ�> 0Ϊ�����PCM���ֽڸ��� ==0��ʾ������Ч
*/
	virtual int DecoderPCM(unsigned char* aBytes, int aSize, unsigned char* aPcm, int& nb_samples)=0;

	virtual bool FFMPEGGetWidthHeight(unsigned char* videooutdata, int videooutdatasize, char* videoName, int* outwidth, int* outheight)=0;
	// ע��ص�
	void RegisterDecodeCallback(FrameCallBackFunc callbackfuc)
	{
		m_callbackfuc = callbackfuc;
	};

	// ע��ص�
	void RegisterDecodeCallback(LocalYuvCallBackFunc callbackfuc)
	{
		m_yuvcallbackfuc = callbackfuc;
	};
	// ע��ص�
	void RegisterDecodeCallback(LocalPcmCallBackFunc callbackfuc)
	{
		m_pcmcallbackfuc = callbackfuc;
	};
	// ע��ص�
	void RegisterDecodeCallback(DecodedFrameCallback callbackfuc)
	{
		m_DecodedFrameCallback = callbackfuc;
	};


	FrameCallBackFunc m_callbackfuc = nullptr; //���ز��Żص�

	LocalYuvCallBackFunc   m_yuvcallbackfuc = nullptr;//���ز��Żص�

	LocalPcmCallBackFunc    m_pcmcallbackfuc = nullptr;

	// ȫ�ֻص���������
	DecodedFrameCallback m_DecodedFrameCallback=nullptr;
	//unsigned char* m_pPcm = NULL;


};

class  AVPacket;
class AVFrame;
class  FFmpegAudioDecoderAPI
{
#define AV_NUM_DATA_POINTERS 8
public:
	FFmpegAudioDecoderAPI() {};
	virtual ~FFmpegAudioDecoderAPI() {};

	static FFmpegAudioDecoderAPI* CreateDecoder(std::map<std::string, std::string> opts = {});

	
	virtual bool PushAudioPacket( uint8_t* aBytes, int aSize, int64_t ts) = 0;
	
	virtual bool PushAudioPacket( AVPacket* packet, AVFrame* frame) = 0;


	virtual bool DecodeData(uint8_t* inData, int inSize, uint8_t* outData[AV_NUM_DATA_POINTERS], int* outSize)=0;

	virtual bool DecodeData(AVPacket* packet, uint8_t* outData[AV_NUM_DATA_POINTERS], int* outSize)=0;

	virtual void RegisterDecodeCallback(std::function <void(uint8_t** data, int size)> result_callback)=0;
	virtual void RegisterDecodeCallback( std::function <void(AVFrame* data)> result_callback)=0;
	virtual    AVFrame* frame_alloc()=0;
	virtual bool DecodeData(uint8_t* inData, int inSize)=0;
};