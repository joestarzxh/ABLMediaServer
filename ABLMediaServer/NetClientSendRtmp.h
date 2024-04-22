#ifndef _NetClientSendRtmp_H
#define _NetClientSendRtmp_H

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

#endif


//#define  WriteFlvFileByDebug   1 //���ڶ����Ƿ�дFLV�ļ� 

//#define    WriteFlvToEsFileFlagSend  1 //����дFLV������ES��������flv�����Ƶ����Ƶ�Ƿ���ȷ 

class CNetClientSendRtmp : public CNetRevcBase
{
public:
	CNetClientSendRtmp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetClientSendRtmp() ;

   uint32_t       nVideoDTS ;
   uint32_t       nAudioDTS;
   uint32_t       nWriteRet;
   volatile  int  nWriteErrorCount;
   char           szRtmpName[string_length_1024];

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
   virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����
   virtual int SendVideo();//������Ƶ����
   virtual int SendAudio();//������Ƶ����
   virtual int SendFirstRequst();//���͵�һ������
   virtual bool RequestM3u8File();//����m3u8�ļ�

   struct rtmp_client_handler_t handler;
   rtmp_client_t*               rtmp;
   volatile bool                bCheckRtspVersionFlag;

   int64_t                      nAsyncAudioStamp;

   flv_muxer_t*                 flvMuxer;
   char                         szURL[string_length_2048];

   volatile bool                bDeleteRtmpPushH265Flag; //��Ϊ��rtmp265��ɾ����־ 

   volatile bool                bAddMediaSourceFlag;//�Ƿ����ý���
   int                          nRtmpState;        //rtmp״̬ 
   int                          nRtmpState3Count; //״̬3�Ĵ��� 

#ifdef  WriteFlvFileByDebug
   void*                        s_flv;
#endif

#ifdef  WriteFlvToEsFileFlagSend
   FILE*                      fWriteVideo;
   FILE*                      fWriteAudio;
#endif
};

#endif