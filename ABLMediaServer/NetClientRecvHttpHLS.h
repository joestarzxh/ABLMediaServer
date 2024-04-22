#ifndef _NetClientRecvHttpHLS_H
#define _NetClientRecvHttpHLS_H

#include "mpeg-ps.h"
#include "mpeg-ts.h"


#define   DefaultM3u8Number                 -88888 
#define  MaxHttp_HLSCNetCacheBufferLength   1024*128 
#define  MaxDefaultContentBodyLength        1024*1024*3 //ȱʡ����
#define  TsStreamBlockBufferLength          188         //TS�����ݿ鳤��
#define  MaxDefaultMediaFifoLength          1024*1024*12 //HLS��Ƶ���泤��

//#define  SaveAudioToAACFile                 1
//#define  SaveTSBufferToFile                 1 //����TS���ļ�

enum HLSRequestFileStatus
{
	HLSRequestFileStatus_NoRequsetFile   = 0, //δִ������
	HLSRequestFileStatus_SendRequest     = 1,//�Ѿ���������
	HLSRequestFileStatus_RecvHttpHead    = 2,//�յ�Httpͷ
	HLSRequestFileStatus_RequestSuccess  = 3,//��������
};

struct HistoryM3u8
{
	int64_t nRecvTime;
	char   szM3u8Data[512];

	HistoryM3u8()
	{
		nRecvTime = 0;
		memset(szM3u8Data, 0x00, sizeof(szM3u8Data));
	}
};

typedef list<HistoryM3u8> HistoryM3u8List;

class CNetClientRecvHttpHLS : public CNetRevcBase
{
public:
	CNetClientRecvHttpHLS(NETHANDLE hServer,NETHANDLE hClient,char* szIP,unsigned short nPort, char* szShareMediaURL);
	~CNetClientRecvHttpHLS();
   
    virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
	virtual int ProcessNetData() ;

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
	virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����

	virtual int SendVideo() ;//������Ƶ����
	virtual int SendAudio() ;//������Ƶ����

	virtual int  SendFirstRequst() ;//���͵�һ������
	virtual bool RequestM3u8File();//����m3u8�ļ�

	void         AddAdtsToAACData(unsigned char* szData, int nAACLength);

	int64_t         nOldPTS;
	uint64_t        nCallBackVideoTime;
	ts_demuxer_t *   ts;
	char             szSourceURL[string_length_2048];
#ifdef USE_BOOST
	boost::shared_ptr<CMediaStreamSource> pMediaSource;
#else
	std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif

	CMediaFifo       hlsVideoFifo;
	CMediaFifo       hlsAudioFifo;

	volatile bool    bExitCallbackThreadFlag ;
	unsigned char    aacData[string_length_2048]; 
#ifdef SaveAudioToAACFile
	FILE*           fileSaveAAC;
#endif
private :
#ifdef  SaveTSBufferToFile
	int64_t                 nTsFileOrder;
#endif
	HistoryM3u8List         historyM3u8List;
	bool                    FindTsFileAtHistoryList(char* szTsFile);

	volatile   int          nHLSRequestFileStatus;//�����ļ�״̬
	int64_t                 nRequestM3u8Time;//���һ�η���m3u8�ļ�ʱ��
	int64_t                 nSendTsFileTime;//���һ��������Ƶ�ļ�ʱ��
    volatile  bool          bCanRequestM3u8File;//��������m3u8�ļ� 

	bool                    AddM3u8ToFifo(char* szM3u8Data, int nDataLength);
	CABLSipParse            httpParse;
	std::mutex              netDataLock;
	unsigned char           szResponseHead[string_length_2048];//http��Ӧͷ
	char                    szRequestFile[string_length_2048];

	int                     nContentLength; //ʵ�ʳ���
	int                     nRecvContentLength;//�Ѿ��յ��ĳ���
	volatile  bool          bRecvHttpHeadFlag;//�Ѿ��������Http ͷ
	unsigned   char*        pContentBody;//���� 
	int                     nContentBodyLength;//ContentBody Buffer  ���� 

	unsigned char           netDataCache[MaxHttp_HLSCNetCacheBufferLength + 4]; //�������ݻ���
	int                     netDataCacheLength;//�������ݻ����С
	int                     nNetStart, nNetEnd; //����������ʼλ��\����λ��
	int                     MaxNetDataCacheCount;

	char                    szHttpURL[string_length_2048];
	char                    szRequestM3u8File[string_length_2048];
	char                    szRequestBuffer[string_length_2048];

	CMediaFifo              requestFileFifo;
	int64_t                 nOldRequestM3u8Number;
	int                     nAudioSize;
};

#endif