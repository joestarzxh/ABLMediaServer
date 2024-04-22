#ifndef _NetClientRecvRtmp_H
#define _NetClientRecvRtmp_H

#include "rtmp-client.h"
#include "flv-writer.h"
#include "flv-proto.h"
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


//#define  WriteFlvFileByDebug   1 //���ڶ����Ƿ�дFLV�ļ� 

//#define    WriteFlvToEsFileFlag  1 //����дFLV������ES��������flv�����Ƶ����Ƶ�Ƿ���ȷ 

class CNetClientRecvRtmp : public CNetRevcBase
{
public:
	CNetClientRecvRtmp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
	~CNetClientRecvRtmp();

	uint32_t       nVideoDTS;
	uint32_t       nAudioDTS;
	uint32_t       nWriteRet;
	volatile  int  nWriteErrorCount;
	char           szRtmpName[256];

	virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address);
	virtual int ProcessNetData();

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec);//������Ƶ����
	virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate);//������Ƶ����
	virtual int SendVideo();//������Ƶ����
	virtual int SendAudio();//������Ƶ����
	virtual int SendFirstRequst();//���͵�һ������
	virtual bool RequestM3u8File();//����m3u8�ļ�

	bool                         GetAppStreamByURL(char* app, char* stream);
	struct rtmp_client_handler_t handler;
	rtmp_client_t* rtmp;
	volatile bool                bCheckRtspVersionFlag;

	flv_demuxer_t* flvDemuxer;
	char                         szURL[string_length_2048];
#ifdef USE_BOOST
	boost::shared_ptr<CMediaStreamSource> pMediaSource;
#else
	std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif
	volatile bool                         bDeleteRtmpPushH265Flag; //��Ϊ��rtmp265��ɾ����־ 

#ifdef  WriteFlvFileByDebug
	void* s_flv;
#endif

#ifdef  WriteFlvToEsFileFlag
	FILE* fWriteVideo;
	FILE* fWriteAudio;
#endif
};

#endif