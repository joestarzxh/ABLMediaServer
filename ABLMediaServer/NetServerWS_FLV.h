#ifndef _CNetServerWS_FLV_H
#define _CNetServerWS_FLV_H

#include "flv-proto.h"
#include "flv-muxer.h"

#define  MaxHttp_WsFlvNetCacheBufferLength    1024*64 

//#define  WriteHttp_FlvFileFlag              1  //дFLV�ļ������ڵ��� 

class CNetServerWS_FLV : public CNetRevcBase
{
public:
	CNetServerWS_FLV(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
	~CNetServerWS_FLV();

	virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address);
	virtual int ProcessNetData();

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec);//������Ƶ����
	virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate);//������Ƶ����

	virtual int SendVideo();//������Ƶ����
	virtual int SendAudio();//������Ƶ����
	virtual int SendFirstRequst();//���͵�һ������
	virtual bool RequestM3u8File();//����m3u8�ļ�

	int            WSSendFlvData(unsigned char* pData, int nDataLength);
	unsigned char  webSocketHead[64];
	unsigned short wsLength16;
	uint64_t       wsLenght64;

	CABLSipParse   wsParse;
	bool           Create_WS_FLV_Handle();
	int            nWebSocketCommStatus;

	std::mutex     NetServerWS_FLVLock;
	flv_muxer_t* flvMuxer;
	void* flvWrite;
	volatile uint32_t   nWriteRet;
	volatile int        nWriteErrorCount;

private:
	volatile  bool         bCheckHttpFlvFlag; //����Ƿ�Ϊhttp-flvЭ�� 

	char                    szSec_WebSocket_Key[256];
	char                    szWebSocketResponse[512];
	char                    szSec_WebSocket_Protocol[256];

	void                    MuxerVideoFlV(char* codeName, unsigned char* pVideo, int nLength);
	void                    MuxerAudioFlV(char* codeName, unsigned char* pAudio, int nLength);

	char                    httpResponseData[512];
	unsigned char           netDataCache[MaxHttp_WsFlvNetCacheBufferLength]; //�������ݻ���
	int                     netDataCacheLength;//�������ݻ����С
	int                     nNetStart, nNetEnd; //����������ʼλ��\����λ��
	int                     MaxNetDataCacheCount;
	int                     data_Length;
	char                    szFlvName[string_length_2048];
	volatile bool           bFindFlvNameFlag;
};

#endif