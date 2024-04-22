#ifndef _RtpTSStreamInput_H
#define _RtpTSStreamInput_H
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
#else

#endif


class CRtpTSStreamInput : public CNetRevcBase
{
public:
	CRtpTSStreamInput(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CRtpTSStreamInput() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����
   virtual int SendVideo();//������Ƶ����
   virtual int SendAudio();//������Ƶ����
   virtual int SendFirstRequst();//���͵�һ������
   virtual bool RequestM3u8File();//����m3u8�ļ�

   mpeg4_aac_t                           aacInfo;
   std::mutex                            tsRecvLock;
   ts_demuxer_t                          *ts;
#ifdef USE_BOOST
   boost::shared_ptr<CMediaStreamSource> pMediaSource;
#else
   std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif

   _rtp_header                           rtpHead;

};

#endif