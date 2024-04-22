#ifndef _CNetServerHTTP_FLV_H
#define _CNetServerHTTP_FLV_H

#include "flv-proto.h"
#include "flv-muxer.h"

#define  MaxHttp_FlvNetCacheBufferLength    1024*32 

//#define  WriteHttp_FlvFileFlag              1  //дFLV�ļ������ڵ��� 

class CNetServerHTTP_FLV : public CNetRevcBase
{
public:
	CNetServerHTTP_FLV(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
	~CNetServerHTTP_FLV();

	virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address);
	virtual int ProcessNetData();

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec);//������Ƶ����
	virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate);//������Ƶ����

	virtual int SendVideo();//������Ƶ����
	virtual int SendAudio();//������Ƶ����
	virtual int SendFirstRequst();//���͵�һ������
	virtual bool RequestM3u8File();//����m3u8�ļ�

	int64_t        nPrintTime;

	std::mutex     NetServerHTTP_FLVLock;
	flv_muxer_t* flvMuxer;
	void* flvWrite;
	volatile uint32_t   nWriteRet;
	volatile int        nWriteErrorCount;
	CABLSipParse   flvParse;
private:
	volatile  bool bCheckHttpFlvFlag; //����Ƿ�Ϊhttp-flvЭ�� 

	void                    MuxerVideoFlV(char* codeName, unsigned char* pVideo, int nLength);
	void                    MuxerAudioFlV(char* codeName, unsigned char* pAudio, int nLength);

	char                    httpResponseData[512];
	unsigned char           netDataCache[MaxHttp_FlvNetCacheBufferLength]; //�������ݻ���
	int                     netDataCacheLength;//�������ݻ����С
	int                     nNetStart, nNetEnd; //����������ʼλ��\����λ��
	int                     MaxNetDataCacheCount;
	int                     data_Length;
	char                    szFlvName[string_length_2048];
	volatile bool           bFindFlvNameFlag;
};

#endif