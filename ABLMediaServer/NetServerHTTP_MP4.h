#ifndef _NetServerHTTP_MP4_H
#define _NetServerHTTP_MP4_H

#include "hls-fmp4.h"
#include "mpeg-ps.h"
#include "hls-m3u8.h"
#include "hls-media.h"
#include "hls-param.h"
#include "mpeg-ps.h"
#include "mov-format.h"
#include "mpeg-ts.h"

#include "mov-format.h"
#include "fmp4-writer.h"

#include "MediaFifo.h"
#include "mpeg4-hevc.h"
#include "mpeg4-aac.h"

#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
#else

#endif
#define     Send_MP4File_MaxPacketCount          1024*16
#define     Send_DownloadFile_MaxPacketCount     1024*256

//��mp4��Ƭд���ļ�
//#define  WriteMp4BufferToFile     1

enum HttpMP4Type
{
	HttpMp4Type_Unknow = 0 ,    //δ֪
	HttpMp4Type_Play = 1 ,      //����
	HttpMp4Type_Download = 2 ,  //���� 
};

class CNetServerHTTP_MP4 : public CNetRevcBase
{
public:
	CNetServerHTTP_MP4(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetServerHTTP_MP4() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
   virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����
   virtual int SendVideo();//������Ƶ����
   virtual int SendAudio();//������Ƶ����
   virtual int SendFirstRequst();//���͵�һ������
   virtual bool RequestM3u8File();//����m3u8�ļ�

   bool                    ResponseError(char* szErrorMsg);
   int                     nPos;
   int                     nSendErrorCount;
   bool                    SendTSBufferData(unsigned char* pTSData, int nLength);

   volatile    bool        bWaitIFrameSuccessFlag;
   volatile    bool        bAddSendThreadToolFlag;
   char                    httpResponseData[1024];
   unsigned char           netDataCache[1024*64]; //�������ݻ���
   int                     netDataCacheLength;//�������ݻ����С
   int                     nNetStart, nNetEnd; //����������ʼλ��\����λ��
   int                     MaxNetDataCacheCount;
   int                     data_Length;
   char                    szMP4Name[string_length_2048];
   volatile bool           bFindMP4NameFlag;
   volatile  bool          bCheckHttpMP4Flag; //����Ƿ�Ϊhttp-MP4Э�� 
   int                     nWriteRet;

#ifdef WriteMp4BufferToFile
   FILE*               fWriteMP4;
#endif
   std::mutex            mediaMP4MapLock;
   int                   avtype;

   unsigned char*       pMP4Buffer;
   int                  nMp4BufferLength;
   int                  vcl;
   int                  update;
   unsigned char        s_packet[1024*256];
   int                  s_packetLength;
   unsigned char        szExtenVideoData[4 * 1024];
   int                  extra_data_size;
   hls_fmp4_t*          hlsFMP4;
   int                  track_video;
   struct mpeg4_aac_t   aacHandle;
   int                  track_aac;
   unsigned char        szExtenAudioData[256];
   int                  nExtenAudioDataLength;
   int                  nAACLength;
   int                   flags;
   volatile      bool     hls_init_segmentFlag;
   unsigned char          pFmp4SPSPPSBuffer[Send_DownloadFile_MaxPacketCount];
   int                    nFmp4SPSPPSLength;
   int                    fTSFileWriteByteCount;

   struct mpeg4_hevc_t    hevc;
   struct mpeg4_avc_t     avc;
   bool                   VideoFrameToFMP4File(unsigned char* szVideoData, int nLength);
   int                    nHttpDownloadSpeed;//�����ٶ�
   HttpMP4Type            httpMp4Type;
   FILE*                  fFileMp4;//��ȡmp4�ļ�
   int                    nReadLength;
   int                    nRecordFileSize ;
   CABLSipParse           mp4Parse;

};

#endif