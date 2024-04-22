#ifndef _NetClientRecvFLV_H
#define _NetClientRecvFLV_H

#include "flv-reader.h"
#include "flv-demuxer.h"
#include "flv-muxer.h"

#include "MediaStreamSource.h"
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
#else
#include <memory>
#endif


//#define          SaveNetDataToFlvFile          1 
//#define         WriteHTTPFlvToEsFileFlag       1 

#define       HttpFlvReadPacketSize     1024*1024*2

class CNetClientRecvFLV : public CNetRevcBase
{
public:
	CNetClientRecvFLV(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
	~CNetClientRecvFLV();

	std::mutex  NetClientRecvFLVLock;
	int         type;
	size_t      taglen;
	uint32_t    timestamp;

	volatile  bool bRecvHttp200OKFlag;
	uint32_t       nVideoDTS;
	uint32_t       nAudioDTS;
	uint32_t       nWriteRet;
	volatile  int  nWriteErrorCount;
	char           szRtmpName[string_length_2048];
	unsigned char  packet[MaxNetDataCacheBufferLength];
	unsigned char           netDataCache[HttpFlvReadPacketSize]; //�������ݻ���
	int                     netDataCacheLength;//�������ݻ����С
	int                     nNetStart, nNetEnd; //����������ʼλ��\����λ��

	virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address);
	virtual int ProcessNetData();

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec);//������Ƶ����
	virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate);//������Ƶ����
	virtual int SendVideo();//������Ƶ����
	virtual int SendAudio();//������Ƶ����
	virtual int SendFirstRequst();//���͵�һ������
	virtual bool RequestM3u8File();//����m3u8�ļ�

	volatile bool                bCheckRtspVersionFlag;
	void* reader;

	flv_demuxer_t* flvDemuxer;
	char                         szURL[string_length_2048];
	char                         szRequestFLVFile[string_length_2048];

#ifdef USE_BOOST
   boost::shared_ptr<CMediaStreamSource> pMediaSource;
#else
   std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif
	volatile bool                         bDeleteRtmpPushH265Flag; //��Ϊ��rtmp265��ɾ����־ 

#ifdef  SaveNetDataToFlvFile
	FILE* fileFLV;
	int64_t                      nNetPacketNumber;
#endif

#ifdef  WriteHTTPFlvToEsFileFlag
	bool    bStartWriteFlag;
	FILE* fWriteVideo;
#endif
};

#endif