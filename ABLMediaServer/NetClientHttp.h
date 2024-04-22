#ifndef _NetClientHttp_H
#define _NetClientHttp_H
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
#else

#endif

#define Send_ResponseHttp_MaxPacketCount   1024*48  //�ظ�http�������һ���ֽ�

#define  MaxNetClientHttpBuffer        1024*1024*1 

#define  MaxClientRespnseInfoLength    1024*1024*1   

class CNetClientHttp : public CNetRevcBase
{
public:
	CNetClientHttp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
	~CNetClientHttp();

	virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address);
	virtual int ProcessNetData();

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec);//������Ƶ����
	virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate);//������Ƶ����
	virtual int SendVideo();//������Ƶ����
	virtual int SendAudio();//������Ƶ����
	virtual int SendFirstRequst();//���͵�һ������
	virtual bool RequestM3u8File();//����m3u8�ļ�

	char                    szResponseData[8192];
	char                    szResponseURL[string_length_2048];//֧���û��Զ����url 
	void                    HttpRequest(char* szUrl, char* szBody, int nLength);

	RequestKeyValueMap      requestKeyValueMap;
	CABLSipParse            httpParse;
	std::mutex              NetClientHTTPLock;
	unsigned char           netDataCache[MaxNetClientHttpBuffer]; //�������ݻ���
	int                     netDataCacheLength;//�������ݻ����С
	int                     nNetStart, nNetEnd; //����������ʼλ��\����λ��
	char                    szHttpHead[1024 * 64];
	char                    szHttpBody[1024 * 64];
	char                    szContentLength[string_length_2048];
	int                     nContent_Length = 0;
	char                    szHttpPath[1024 * 64];
	char                    szConnection[string_length_2048];
};

#endif