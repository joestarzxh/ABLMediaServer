#ifndef _NetClientRecvJTT1078_H
#define _NetClientRecvJTT1078_H

#include "flv-reader.h"
#include "flv-demuxer.h"
#include "flv-muxer.h"

#include "MediaStreamSource.h"

#pragma pack(1)
struct Head1708 {
	unsigned char m_headFlag[4];
	unsigned char m_vpFlag;
	unsigned char m_mpFlag;
	unsigned short m_sequence;
	unsigned char m_sim[6];
	char m_channel;
	unsigned char m_type;
	char m_timestamp[8];
	unsigned short m_iFrameInterval;
	unsigned short m_frameInterval;
	unsigned short m_length;
};
#pragma pack()
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
#else
#include <unordered_map>
#include <memory>
#endif


#define          SaveNetDataToJTT1078File          1 

#define       HttpFlvReadPacketSize     1024*1024*2

class CNetClientRecvJTT1078 : public CNetRevcBase
{
public:
	CNetClientRecvJTT1078(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetClientRecvJTT1078() ;

   int64_t  tPutVideoTime;
   string _m3u8 ;
   string m_es;

   std::mutex  NetClientRecvFLVLock;
   int         type ;
   size_t      taglen;
   uint32_t    timestamp;

   volatile  bool bRecvHttp200OKFlag;
   uint32_t       nVideoDTS ;
   uint32_t       nAudioDTS;
   uint32_t       nWriteRet;
   volatile  int  nWriteErrorCount;
   char           szRtmpName[512];
   unsigned char  packet[MaxNetDataCacheBufferLength];
   unsigned char           netDataCache[HttpFlvReadPacketSize]; //�������ݻ���
   int                     netDataCacheLength;//�������ݻ����С
   int                     nNetStart, nNetEnd; //����������ʼλ��\����λ��

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����
   virtual int SendVideo();//������Ƶ����
   virtual int SendAudio();//������Ƶ����
   virtual int SendFirstRequst();//���͵�һ������
   virtual bool RequestM3u8File();//����m3u8�ļ�

   volatile bool                bCheckRtspVersionFlag;
   char                         szURL[512];
   char                         szRequestFLVFile[512];

#ifdef USE_BOOST
   boost::shared_ptr<CMediaStreamSource> pMediaSource;
#else
   std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif

   volatile bool                         bDeleteRtmpPushH265Flag; //��Ϊ��rtmp265��ɾ����־ 

#ifdef  SaveNetDataToJTT1078File
   FILE*                        fileJTT1078;
#endif

#ifdef  WriteHTTPFlvToEsFileFlag
   bool    bStartWriteFlag;
   FILE*   fWriteVideo;
#endif
};

#endif