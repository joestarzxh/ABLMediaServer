#ifndef _NetClientSnap_H
#define _NetClientSnap_H

#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
using namespace boost;
#else

#endif

#include "VideoDecode.h"


class CNetClientSnap : public CNetRevcBase
{
public:
	CNetClientSnap(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
	~CNetClientSnap();

	virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address);
	virtual int ProcessNetData();

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec);//������Ƶ����
	virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate);//������Ƶ����
	virtual int SendVideo();//������Ƶ����
	virtual int SendAudio();//������Ƶ����
	virtual int SendFirstRequst();//���͵�һ������
	virtual bool RequestM3u8File();//����m3u8�ļ�

	volatile bool   bWaitIFrameFlag;
	static  int     nPictureNumber; //ͬһ���ڵ����
	std::mutex      NetClientSnapLock;
	CVideoDecode    videoDecode;
	char            szFileNameOrder[64];
	char            szFileName[string_length_2048];
	bool            bUpdateFlag;
	char            szPictureFileName[string_length_2048];
};

#endif