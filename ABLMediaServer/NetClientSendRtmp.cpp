/*
���ܣ�
    ʵ��rtmp�ͻ��˵�����ģ�� 
	 
����    2021-07-23
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetClientSendRtmp.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
extern std::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
#endif

extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern int                                   SampleRateArray[] ;
extern int64_t                               nTestRtmpPushID;
extern MediaServerPort                       ABL_MediaServerPort;

extern void LIBNET_CALLMETHOD	onconnect(NETHANDLE clihandle,
	uint8_t result, uint16_t nLocalPort);

extern void LIBNET_CALLMETHOD onread(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	uint8_t* data,
	uint32_t datasize,
	void* address);

extern void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle,
	NETHANDLE clihandle);

static int rtmp_client_pushCB(void* param, const void* header, size_t len, const void* data, size_t bytes)
{
	CNetClientSendRtmp* pClient = (CNetClientSendRtmp*)param;

	if (pClient != NULL && pClient->bRunFlag )
	{
		if (len > 0 && header != NULL)
		{
			pClient->nWriteRet = XHNetSDK_Write(pClient->nClient, (uint8_t*)header, len, true);
			if (pClient->nWriteRet != 0)
			{
				pClient->nWriteErrorCount ++;
				if (pClient->nWriteErrorCount >= 30)
				{
					pClient->bRunFlag = false;
					WriteLog(Log_Debug, "rtmp_client_pushCB ����ʧ�ܣ����� nWriteErrorCount = %d ", pClient->nWriteErrorCount);

				    pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
				}
			}
			else
				pClient->nWriteErrorCount = 0;
		}
		if (bytes > 0 && data != NULL)
		{
			pClient->nRecvDataTimerBySecond = 0;

			pClient->nWriteRet = XHNetSDK_Write(pClient->nClient, (uint8_t*)data, bytes, true);
			if (pClient->nWriteRet != 0)
			{
				pClient->nWriteErrorCount++;

				if (!pClient->bResponseHttpFlag)
				{//����ʧ�ܻظ�
 					sprintf(pClient->szResponseBody, "{\"code\":%d,\"memo\":\"rtmp push Error \",\"key\":%d}", IndexApiCode_RtmpPushError, 0);
					pClient->ResponseHttp(pClient->nClient_http, pClient->szResponseBody, false);
				}

				WriteLog(Log_Debug, "rtmp_client_pushCB ����ʧ�ܣ����� nWriteErrorCount = %d ", pClient->nWriteErrorCount);
 			}
			else
			{
				pClient->nWriteErrorCount = 0;

				//�ظ�http 
				if (!pClient->bResponseHttpFlag && GetTickCount64() - pClient->nPrintTime >= 1000 )
				{//�����ɹ��ظ�
  					sprintf(pClient->szResponseBody, "{\"code\":0,\"memo\":\"success\",\"key\":%llu}", pClient->hParent);
					pClient->ResponseHttp(pClient->nClient_http, pClient->szResponseBody, false);
 				}
			}
		}

		if (pClient->bAddMediaSourceFlag == false)
		{
 			pClient->nRtmpState = rtmp_client_getstate(pClient->rtmp);
			if (pClient->nRtmpState == 3)
				pClient->nRtmpState3Count ++;

			WriteLog(Log_Debug, "rtmp_client_pushCB  nRtmpState = %d ", pClient->nRtmpState);
			if (pClient->nRtmpState3Count >= 2)
			{
				//�ڸ����ǳɹ�
				auto   pParentPtr = GetNetRevcBaseClient(pClient->hParent);
				if (pParentPtr && pParentPtr->bProxySuccessFlag == false)
					pClient->bProxySuccessFlag = pParentPtr->bProxySuccessFlag = true;

				pClient->bUpdateVideoFrameSpeedFlag = true; //���ڳɹ�����
				pClient->bAddMediaSourceFlag = true;
				auto  pMediaSource = GetMediaStreamSource(pClient->m_szShareMediaURL, true);
				if (pMediaSource != NULL)
				{
 					//����ý��Դ
					pClient->SplitterAppStream(pClient->m_szShareMediaURL);
					sprintf(pClient->m_addStreamProxyStruct.url, "rtmp://localhost:%d/%s/%s", ABL_MediaServerPort.nRtmpPort, pClient->m_addStreamProxyStruct.app, pClient->m_addStreamProxyStruct.stream);

					pMediaSource->AddClientToMap(pClient->nClient);
					WriteLog(Log_Debug, "rtmp_client_pushCB  ��rtmp���� = %llu ������ý��Դ�ַ��� ", pClient->nClient);
				}
				else
				{
					WriteLog(Log_Debug, "rtmp_client_pushCB ������ý��Դ %s ������ɾ�� nClient = %llu ", pClient->m_szShareMediaURL,pClient->nClient);
					pDisconnectBaseNetFifo.push((unsigned char*)&pClient->nClient, sizeof(pClient->nClient));
 				}
			}
		}
	}
 
	return len + bytes;
}

static int NetClientSendRtmp_MuxerFlv(void* param, int type, const void* data, size_t bytes, uint32_t timestamp)
{
	CNetClientSendRtmp* pClient = (CNetClientSendRtmp*)param;
	int r;

 	if (pClient == NULL || pClient->rtmp == NULL || pClient->flvMuxer == NULL || !pClient->bRunFlag)
		return 0;

	if (FLV_TYPE_AUDIO == type)
	{
 		r = rtmp_client_push_audio(pClient->rtmp, data, bytes, timestamp);
	}
	else if (FLV_TYPE_VIDEO == type)
	{
		r = rtmp_client_push_video(pClient->rtmp, data, bytes, timestamp);
	}
	return 0;
}

CNetClientSendRtmp::CNetClientSendRtmp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	rtmp = NULL ;
	bCheckRtspVersionFlag = false;
	bDeleteRtmpPushH265Flag = false;
	nServer = hServer;
	nClient = hClient;
	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	NetDataFifo.InitFifo(MaxNetDataCacheBufferLength);
	strcpy(m_szShareMediaURL, szShareMediaURL);

	int r;
	flvMuxer = NULL;
	nWriteRet = 0;
	nWriteErrorCount = 0;
	videoDts = 0;
	nAsyncAudioStamp = -1;

#ifdef  WriteFlvFileByDebug
	char  szFlvFile[256] = { 0 };
	sprintf(szFlvFile,".\\%X_%d.flv", this, rand());
	s_flv = flv_writer_create(szFlvFile);
#endif
#ifdef  WriteFlvToEsFileFlagSend
	char    szVideoFile[256] = { 0 };
	char    szAudioFile[256] = { 0 };
	sprintf(szVideoFile, ".\\%X_%d.264", this, rand());
	sprintf(szAudioFile, ".\\%X_%d.aac", this, rand());
	fWriteVideo = fopen(szVideoFile, "wb");
	fWriteAudio = fopen(szAudioFile, "wb");
#endif
	memset(&handler, 0, sizeof(handler));
	handler.send = rtmp_client_pushCB;

	if (ParseRtspRtmpHttpURL(szIP) == true)
	 uint32_t ret = XHNetSDK_Connect((int8_t*)m_rtspStruct.szIP, atoi(m_rtspStruct.szPort), (int8_t*)(NULL), 0, (uint64_t*)&nClient, onread, onclose, onconnect, 0, MaxClientConnectTimerout, 1);

	nVideoDTS = 0 ;
	nAudioDTS = 0 ;
	nRtmpState3Count = 0;
	bAddMediaSourceFlag = false;
	memset(szRtmpName, 0x00, sizeof(szRtmpName));
	bRunFlag = true;
	netBaseNetType = NetBaseNetType_RtmpClientPush;
 
	WriteLog(Log_Debug, "CNetClientSendRtmp ���� = %X  nClient = %llu ",this, nClient);
}

CNetClientSendRtmp::~CNetClientSendRtmp()
{
	bRunFlag = false;
 	std::lock_guard<std::mutex> lock(businessProcMutex);

	if(flvMuxer)
	  flv_muxer_destroy(flvMuxer);

	if (rtmp != NULL)
	{
		if(nRtmpState >= 3)
		  rtmp_client_stop(rtmp);

		rtmp_client_destroy(rtmp);
	}
 
#ifdef  WriteFlvFileByDebug
	flv_writer_destroy(s_flv);
#endif
#ifdef  WriteFlvToEsFileFlagSend
 	fclose(fWriteVideo);
	fclose(fWriteAudio);
#endif

	NetDataFifo.FreeFifo();

	WriteLog(Log_Debug, "CNetClientSendRtmp ���� = %X  nClient = %llu \r\n", this, nClient);
	
	malloc_trim(0);
}

int CNetClientSendRtmp::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	nRecvDataTimerBySecond = 0;
	if (!bRunFlag || m_addPushProxyStruct.disableVideo[0] != 0x30 )
		return -1;
	std::lock_guard<std::mutex> lock(businessProcMutex);

	//ֻ����Ƶ������������Ƶ
	if (ABL_MediaServerPort.nEnableAudio == 0 || strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0 || strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
		nVideoStampAdd = 1000 / mediaCodecInfo.nVideoFrameRate;

	if (rtmp == NULL || flvMuxer == NULL)
		return -2;

	//rtmp�������Ҫ���� sps\pps ֡����ΪI֡���������
	if (nDataLength < 2048)
	{
		if (CheckVideoIsIFrame(szVideoCodec, pVideoData, nDataLength))
		{
			WriteLog(Log_Debug, "CNetClientSendRtmp = %X SPS\PPS nClient = %llu nDataLength = %d \r\n", this, nClient, nDataLength);
			return 0;
		}
	}

	if (strcmp(szVideoCodec, "H264") == 0)
	{
		if (flvMuxer)
			flv_muxer_avc(flvMuxer, pVideoData, nDataLength, videoDts, videoDts);
	}
	else if (strcmp(szVideoCodec, "H265") == 0)
	{
		if (flvMuxer)
			flv_muxer_hevc(flvMuxer, pVideoData, nDataLength, videoDts, videoDts);
	}

	//printf("flvPS = %d \r\n", videoDts);
	videoDts += nVideoStampAdd;

	return 0;
}

int CNetClientSendRtmp::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	nRecvDataTimerBySecond = 0;
	if (!bRunFlag || m_addPushProxyStruct.disableAudio[0] != 0x30 )
		return -1;
	std::lock_guard<std::mutex> lock(businessProcMutex);

	if (nAsyncAudioStamp == -1)
		nAsyncAudioStamp = GetTickCount();

	if (rtmp == NULL || flvMuxer == NULL || ABL_MediaServerPort.nEnableAudio == 0)
		return -2;

	if (strcmp(szAudioCodec, "AAC") == 0)
	{
		if (flvMuxer)
			flv_muxer_aac(flvMuxer, pAudioData, nDataLength, audioDts, audioDts);

		if (bUserNewAudioTimeStamp == false)
			audioDts += mediaCodecInfo.nBaseAddAudioTimeStamp;
		else
		{
			nUseNewAddAudioTimeStamp --;
			audioDts += nNewAddAudioTimeStamp;
			if (nUseNewAddAudioTimeStamp <= 0)
			{
				bUserNewAudioTimeStamp = false;
			}
		}
	}

	//ͬ������Ƶ 
	SyncVideoAudioTimestamp();

	return 0;
}
int CNetClientSendRtmp::SendVideo()
{
	return 0;
}

int CNetClientSendRtmp::SendAudio()
{

	return 0;
}

int CNetClientSendRtmp::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	nRecvDataTimerBySecond = 0;
	NetDataFifo.push(pData, nDataLength);
	return 0;
}

int CNetClientSendRtmp::ProcessNetData()
{
 	unsigned char* pData = NULL;
	int            nLength;
	int            nRet;

	pData = NetDataFifo.pop(&nLength);
	if(pData != NULL && rtmp != NULL )
	{
		if (nLength > 0)
			nRet = rtmp_client_input(rtmp, pData, nLength);
 
 		NetDataFifo.pop_front();
	}
	return 0;
}

//���͵�һ������
int CNetClientSendRtmp::SendFirstRequst()
{
	int nPos1, nPos2,nPos3;
	string strURL = szClientIP;
	char   szApp[string_length_1024] = { 0 };
	char   szStream[string_length_1024] = { 0 };
	char   tcurl[string_length_2048];
	nPos1 = strURL.find("/", 10);
	if (nPos1 > 10)
		nPos2 = strURL.find("/", nPos1 + 1);
	if (nPos1 > 0 && nPos2 > nPos1)
	{
		flvMuxer = flv_muxer_create(NetClientSendRtmp_MuxerFlv, this);

		memcpy(szApp, szClientIP + nPos1 + 1, nPos2 - nPos1 - 1);
		memcpy(szStream, szClientIP + nPos2 + 1, strlen(szClientIP) - nPos2 - 1);

		strcpy(tcurl, szClientIP);
		nPos3 = strURL.rfind("/", strlen(szClientIP));
		if (nPos3 > 0 && nPos3 < strlen(szClientIP))
			tcurl[nPos3] = 0x00;

		rtmp = rtmp_client_create(szApp, szStream, tcurl, this, &handler);
		if(rtmp != NULL)
		{
		  int  r = rtmp_client_start(rtmp, 0);
	 	  r = rtmp_client_getstate(rtmp);
		}
	}

	return 0;
}

//����m3u8�ļ�
bool  CNetClientSendRtmp::RequestM3u8File()
{
	return true;
}