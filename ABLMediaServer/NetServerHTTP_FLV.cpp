/*
���ܣ�
       ʵ��http-flv��������ý�����ݷ��͹��� 
����    2021-03-29
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetServerHTTP_FLV.h"
#ifdef USE_BOOST

extern             bool                DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource>  GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);

extern CMediaFifo                      pDisconnectBaseNetFifo; //������ѵ����� 
extern bool                            DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                 ABL_MediaServerPort;
extern bool                            AddClientToMapAddMutePacketList(uint64_t nClient);
extern bool                            DelClientToMapFromMutePacketList(uint64_t nClient);

#else

extern             bool                DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource>  GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);

extern CMediaFifo                      pDisconnectBaseNetFifo; //������ѵ����� 
extern bool                            DeleteClientMediaStreamSource(uint64_t nClient);
extern MediaServerPort                 ABL_MediaServerPort;
extern bool                            AddClientToMapAddMutePacketList(uint64_t nClient);
extern bool                            DelClientToMapFromMutePacketList(uint64_t nClient);

#endif//USE_BOOST


//FLV�ϳɻص����� 
static int NetServerHTTP_FLV_MuxerCB(void* flv, int type, const void* data, size_t bytes, uint32_t timestamp)
{
	CNetServerHTTP_FLV* pHttpFLV = (CNetServerHTTP_FLV*)flv;

	if (!pHttpFLV->bRunFlag)
		return -1;

#ifdef WriteHttp_FlvFileFlag
	if (pHttpFLV)
 		return flv_writer_input(pHttpFLV->flvWrite, type, data, bytes, timestamp);
#else 
	if (pHttpFLV)
		return flv_writer_input(pHttpFLV->flvWrite, type, data, bytes, timestamp);
#endif
}

int  NetServerHTTP_FLV_OnWrite_CB(void* param, const struct flv_vec_t* vec, int n)
{
	CNetServerHTTP_FLV* pHttpFLV = (CNetServerHTTP_FLV*)param;

	if (pHttpFLV != NULL && pHttpFLV->bRunFlag )
	{
		for (int i = 0; i < n; i++)
		{
			pHttpFLV->nWriteRet = XHNetSDK_Write(pHttpFLV->nClient,(unsigned char*)vec[i].ptr, vec[i].len,true);
			if (pHttpFLV->nWriteRet != 0)
			{
				pHttpFLV->nWriteErrorCount ++;//���ͳ����ۼ� 
				if (pHttpFLV->nWriteErrorCount >= 30)
				{
					pHttpFLV->bRunFlag = false;
					WriteLog(Log_Debug, "NetServerHTTP_FLV_OnWrite_CB ����ʧ�ܣ����� nWriteErrorCount = %d ", pHttpFLV->nWriteErrorCount);
					DeleteNetRevcBaseClient(pHttpFLV->nClient);
				}
			}
			else
				pHttpFLV->nWriteErrorCount = 0;//��λ 
  		}
	}
	return 0;
}

CNetServerHTTP_FLV::CNetServerHTTP_FLV(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	memset(netDataCache, 0x00, sizeof(netDataCache));
	nServer = hServer;
	nClient = hClient;
	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	bCheckHttpFlvFlag = false;
	strcpy(m_szShareMediaURL, szShareMediaURL);

	MaxNetDataCacheCount = MaxHttp_FlvNetCacheBufferLength;
	netDataCacheLength = data_Length = nNetStart = nNetEnd = 0;//�������ݻ����С
	bFindFlvNameFlag = false;
	memset(szFlvName, 0x00, sizeof(szFlvName));
	flvMuxer = NULL;

	flvWrite  = NULL ;
	nWriteRet = 0;
	nWriteErrorCount = 0;

	netBaseNetType = NetBaseNetType_HttpFLVServerSendPush ;
	bRunFlag = true;

    nNewAddAudioTimeStamp = 64;
	bUserNewAudioTimeStamp = false;
	nUseNewAddAudioTimeStamp = 0;
	nPrintTime = GetTickCount64();
 
	WriteLog(Log_Debug, "CNetServerHTTP_FLV ���� = %X, nClient = %llu ",this, nClient);
}

CNetServerHTTP_FLV::~CNetServerHTTP_FLV()
{
	std::lock_guard<std::mutex> lock(NetServerHTTP_FLVLock);

	WriteLog(Log_Debug, "CNetServerHTTP_FLV =%X Step 1 nClient = %llu ",this, nClient);

	WriteLog(Log_Debug, "CNetServerHTTP_FLV =%X Step 2 nClient = %llu ",this, nClient);
	
	if (flvMuxer)
	{
		flv_muxer_destroy(flvMuxer);
		flvMuxer = NULL;
	}
	WriteLog(Log_Debug, "CNetServerHTTP_FLV =%X Step 3 nClient = %llu ",this, nClient);

	if (flvWrite)
	{
		flv_writer_destroy(flvWrite);
		flvWrite = NULL;
	}
	WriteLog(Log_Debug, "CNetServerHTTP_FLV =%X Step 4 nClient = %llu ",this, nClient);

	m_videoFifo.FreeFifo();
	m_audioFifo.FreeFifo();
	
	//�Ӿ�������ɾ�� 
	if (bAddMuteFlag)
	  DelClientToMapFromMutePacketList(nClient);
	
	WriteLog(Log_Debug, "CNetServerHTTP_FLV ���� =%X szFlvName = %s, nClient = %llu \r\n", this, szFlvName, nClient);
	
	malloc_trim(0);
}

int CNetServerHTTP_FLV::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	nRecvDataTimerBySecond = 0;
	m_videoFifo.push(pVideoData, nDataLength);
	return 0 ;
}

int CNetServerHTTP_FLV::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	nRecvDataTimerBySecond = 0;

	if (ABL_MediaServerPort.nEnableAudio == 0)
		return -1;

	if ( !(strcmp(szAudioCodec, "AAC") == 0 ||  strcmp(szAudioCodec,"MP3") == 0 ))
		return 0;

	m_audioFifo.push(pAudioData, nDataLength);

	return 0;
}

void  CNetServerHTTP_FLV::MuxerVideoFlV(char* codeName, unsigned char* pVideo, int nLength)
{
	//ֻ����Ƶ������������Ƶ
	if(ABL_MediaServerPort.nEnableAudio == 0 || strcmp(mediaCodecInfo.szAudioName,"G711_A") == 0 || strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
	   nVideoStampAdd = 1000 / mediaCodecInfo.nVideoFrameRate;

	if (strcmp(codeName, "H264") == 0)
	{
		if (flvMuxer)
			flv_muxer_avc(flvMuxer, pVideo, nLength, videoDts, videoDts);
	}
	else if (strcmp(codeName, "H265") == 0)
	{
		if (flvMuxer)
			flv_muxer_hevc(flvMuxer, pVideo, nLength, videoDts, videoDts);
	}

	//printf("flvPS = %d \r\n", videoDts);
	videoDts += nVideoStampAdd;
}

void  CNetServerHTTP_FLV::MuxerAudioFlV(char* codeName, unsigned char* pAudio, int nLength)
{
	if (ABL_MediaServerPort.nEnableAudio == 0)
		return;

	if (nAsyncAudioStamp == -1)
		nAsyncAudioStamp = GetTickCount();

	if (flvMuxer)
	{
		if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
			flv_muxer_aac(flvMuxer, pAudio, nLength, audioDts, audioDts);
 		else if (strcmp(mediaCodecInfo.szAudioName, "MP3") == 0)
			flv_muxer_mp3(flvMuxer, pAudio, nLength, audioDts, audioDts);
 	}

	if(bUserNewAudioTimeStamp == false)
		audioDts += mediaCodecInfo.nBaseAddAudioTimeStamp ;
	else
	{
		nUseNewAddAudioTimeStamp --;
		audioDts += nNewAddAudioTimeStamp;
		if (nUseNewAddAudioTimeStamp <= 0)
		{
			bUserNewAudioTimeStamp = false;
		}
	}

	//ͬ������Ƶ 
	SyncVideoAudioTimestamp();
}

int CNetServerHTTP_FLV::SendVideo()
{
	std::lock_guard<std::mutex> lock(NetServerHTTP_FLVLock);
	
	if (nWriteErrorCount >= 30)
	{
		WriteLog(Log_Debug, "����flv ʧ��,nClient = %llu ",nClient);
		DeleteNetRevcBaseClient(nClient);
 		return -1;
	}

	unsigned char* pData = NULL;
	int            nLength = 0;
	if((pData = m_videoFifo.pop(&nLength)) != NULL )
	{
		if (nMediaSourceType == MediaSourceType_LiveMedia)
		{
			MuxerVideoFlV(mediaCodecInfo.szVideoName, pData, nLength);
 		}
		else
			MuxerVideoFlV(mediaCodecInfo.szVideoName, pData+4, nLength-4);
		m_videoFifo.pop_front();
	}

	if (nWriteErrorCount >= 30)
	{
		WriteLog(Log_Debug, "����flv ʧ��,nClient = %llu ", nClient);
		DeleteNetRevcBaseClient(nClient);
	}

	return 0;
}

int CNetServerHTTP_FLV::SendAudio()
{
	std::lock_guard<std::mutex> lock(NetServerHTTP_FLVLock);
	
	if (nWriteErrorCount >= 30)
	{
		WriteLog(Log_Debug, "����flv ʧ��,nClient = %llu ", nClient);
		DeleteNetRevcBaseClient(nClient);
		return -1;
	}

	//����AAC,mp3
	if (!(strcmp(mediaCodecInfo.szAudioName, "AAC") == 0 || strcmp(mediaCodecInfo.szAudioName, "MP3") == 0))
		return -1;

	unsigned char* pData = NULL;
	int            nLength = 0;
	if((pData = m_audioFifo.pop(&nLength)) != NULL)
	{
		if (nMediaSourceType == MediaSourceType_LiveMedia)
			MuxerAudioFlV(mediaCodecInfo.szAudioName, pData, nLength);
		else
			MuxerAudioFlV(mediaCodecInfo.szAudioName, pData + 4, nLength - 4);

		m_audioFifo.pop_front();
  	}
	if (nWriteErrorCount >= 30)
	{
		DeleteNetRevcBaseClient(nClient);
		WriteLog(Log_Debug, "����flv ʧ��,nClient = %llu ", nClient);
	}

	return 0;
}

int CNetServerHTTP_FLV::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
	if (MaxNetDataCacheCount - nNetEnd >= nDataLength)
	{//ʣ��ռ��㹻
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}
	else
	{//ʣ��ռ䲻������Ҫ��ʣ���buffer��ǰ�ƶ�
		if (netDataCacheLength > 0)
		{//���������ʣ��
			memmove(netDataCache, netDataCache + nNetStart, netDataCacheLength);
			nNetStart = 0;
			nNetEnd = netDataCacheLength;

			if (MaxNetDataCacheCount - nNetEnd < nDataLength)
			{
				nNetStart = nNetEnd = netDataCacheLength = 0;
				WriteLog(Log_Debug, "CNetServerHTTP_FLV = %X nClient = %llu �����쳣 , ִ��ɾ��", this, nClient);
				DeleteNetRevcBaseClient(nClient);
				return 0;
			}
 		}
		else
		{//û��ʣ�࣬��ô �ף�βָ�붼Ҫ��λ 
			nNetStart = 0;
			nNetEnd = 0;
			netDataCacheLength = 0;
 		}
		memcpy(netDataCache + nNetEnd, pData, nDataLength);
		netDataCacheLength += nDataLength;
		nNetEnd += nDataLength;
	}

	WriteLog(Log_Debug, "InputNetData() ... ");

	return true;
}

int CNetServerHTTP_FLV::ProcessNetData()
{
	if (!bFindFlvNameFlag)
	{
		if (netDataCacheLength > 512 || strstr((char*)netDataCache, "%") != NULL)
		{
			WriteLog(Log_Debug, "CNetServerHTTP_FLV = %X , nClient = %llu ,netDataCacheLength = %d, ���͹�����url���ݳ��ȷǷ� ,����ɾ�� ", this, nClient, netDataCacheLength);
			DeleteNetRevcBaseClient(nClient);
		}

		if (strstr((char*)netDataCache, "\r\n\r\n") == NULL)
		{
			WriteLog(Log_Debug, "������δ�������� ");
			if (memcmp(netDataCache, "GET ", 4) != 0)
			{
				WriteLog(Log_Debug, "CNetServerHTTP_FLV = %X , nClient = %llu , ���յ����ݷǷ� ", this, nClient);
				DeleteNetRevcBaseClient(nClient);
			}
			return -1;
 		}
	}

	if (!bCheckHttpFlvFlag)
	{
		bCheckHttpFlvFlag = true;

		//�������FLV�ļ���ȡ����
		char    szTempName[string_length_2048] = { 0 };
		string  strHttpHead = (char*)netDataCache;
		int     nPos1, nPos2;
		nPos1 = strHttpHead.find("GET ", 0);
		if (nPos1 >= 0 && nPos1 != string::npos )
		{
			nPos2 = strHttpHead.find(" HTTP/", 0);
			if (nPos2 > 0 && nPos2 != string::npos)
			{
				if ((nPos2 - nPos1 - 4) > string_length_2048)
				{
					WriteLog(Log_Debug, "CNetServerHTTP_FLV=%X,�����ļ����Ƴ��ȷǷ� nClient = %llu ", this, nClient);
					DeleteNetRevcBaseClient(nClient);
					return -1;
 				}

				bFindFlvNameFlag = true;
				memset(szTempName, 0x00, sizeof(szTempName));
				memcpy(szTempName, netDataCache + nPos1 + 4, nPos2 - nPos1 - 4);

				string strFlvName = szTempName;
				nPos2 = strFlvName.find("?", 0);
				if (nPos2 > 0)
				{//�У�����Ҫȥ����������ַ��� 
					if(strlen(szPlayParams) == 0)//������Ȩ����
					  memcpy(szPlayParams, szTempName + (nPos2 + 1), strlen(szTempName) - nPos2 - 1);

					memset(szFlvName, 0x00, sizeof(szFlvName));
					memcpy(szFlvName, szTempName, nPos2);
				}
				else//û�У���ֱ�ӿ��� 
					strcpy(szFlvName, szTempName);

				if(strlen(szFlvName) < 512 )
				   WriteLog(Log_Debug, "CNetServerHTTP_FLV=%X ,nClient = %llu ,������FLV �ļ����� %s ", this, nClient, szFlvName);
			}
		}

		if (!bFindFlvNameFlag)
		{
			WriteLog(Log_Debug, "CNetServerHTTP_FLV=%X, ���� �Ƿ��� Http-flv Э�����ݰ� nClient = %llu ", this, nClient);
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		//����FLV�ļ������м��ж��Ƿ�Ϸ�
		if (!(strstr(szFlvName, ".flv") != NULL || strstr(szFlvName, ".FLV") != NULL))
		{
			WriteLog(Log_Debug, "CNetServerHTTP_FLV = %X,  nClient = %llu ", this, nClient);
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		//����FLV�ļ������в����������� 
		if (strstr(szFlvName, ".flv") != NULL || strstr(szFlvName, ".FLV") != NULL)
 			szFlvName[strlen(szFlvName) - 4] = 0x00;

		strcpy(szMediaSourceURL, szFlvName);
#ifdef USE_BOOST
		boost::shared_ptr<CMediaStreamSource> pushClient = NULL;
#else
		std::shared_ptr<CMediaStreamSource> pushClient = NULL;
#endif
 	
		if (strstr(szFlvName, RecordFileReplaySplitter) == NULL)
		{//ʵ���㲥
		     pushClient = GetMediaStreamSource(szFlvName, true);
			if (pushClient == NULL)
			{
				WriteLog(Log_Debug, "CNetServerHTTP_FLV=%X, û����������ĵ�ַ %s nClient = %llu ", this, szFlvName, nClient);

				sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: keep-alive\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
				nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

				DeleteNetRevcBaseClient(nClient);
				return -1;
			}
		}
		else
		{//¼��㲥
		    //��ѯ�㲥��¼���Ƿ����
			if (QueryRecordFileIsExiting(szFlvName) == false)
			{
				WriteLog(Log_Debug, "CNetServerHTTP_FLV = %X, û�е㲥��¼���ļ� %s nClient = %llu ", this, szFlvName, nClient);
 				sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: keep-alive\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
				nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);
				DeleteNetRevcBaseClient(nClient);
				return -1;
 			}

			//����¼���ļ��㲥
			pushClient = CreateReplayClient(szFlvName, &nReplayClient);
			if (pushClient == NULL)
			{
 				WriteLog(Log_Debug, "CNetServerHTTP_FLV=%X, ��¼���ļ��㲥ʧ�� %s nClient = %llu ", this, szFlvName, nClient);
 				sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: keep-alive\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
				nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);
				DeleteNetRevcBaseClient(nClient);
				return -1;
			}
		}

		//����ý��Դ
		SplitterAppStream(szFlvName);
		sprintf(m_addStreamProxyStruct.url, "http://%s:%d/%s/%s.flv", ABL_MediaServerPort.ABL_szLocalIP, ABL_MediaServerPort.nHttpFlvPort, m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);
 
		char szOrigin[string_length_1024] = { 0 };
		flvParse.ParseSipString((char*)netDataCache);
		flvParse.GetFieldValue("Origin", szOrigin);
		if (strlen(szOrigin) == 0)
			strcpy(szOrigin, "*");

		sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: keep-alive\r\nContent-Type: video/x-flv; charset=utf-8\r\nDate: Wed, Apr 20 2022 10:04:31 GMT\r\nKeep-Alive: timeout=30, max=100\r\nServer: %s\r\n\r\n", szOrigin, MediaServerVerson);
		XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);
		if (nWriteRet != 0)
		{
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

 		flvMuxer = flv_muxer_create(NetServerHTTP_FLV_MuxerCB, this);
		if (flvMuxer == NULL)
		{
			WriteLog(Log_Debug, "���� flv �����ʧ�� ");
			DeleteNetRevcBaseClient(nClient);
			return -1;
 		}

#ifdef WriteHttp_FlvFileFlag //д��FLV�ļ�
		char  szWriteFlvName[256] = { 0 };
		sprintf(szWriteFlvName, ".\\%X_%llu.flv", this, nClient);
		flvWrite = flv_writer_create(szWriteFlvName);
#else //ͨ�����紫�� 
		if ((strcmp(pushClient->m_mediaCodecInfo.szVideoName, "H264") == 0 || strcmp(pushClient->m_mediaCodecInfo.szVideoName, "H265") == 0) &&
			strcmp(pushClient->m_mediaCodecInfo.szAudioName, "AAC") == 0 && ABL_MediaServerPort.nEnableAudio == 1)
		{//H264��H265  && AAC��������Ƶ����Ƶ
		  flvWrite = flv_writer_create2(1, 1, NetServerHTTP_FLV_OnWrite_CB, (void*)this);
		  WriteLog(Log_Debug, "����http-flv �����ʽΪ�� ��Ƶ %s����Ƶ %s  nClient = %llu ", pushClient->m_mediaCodecInfo.szVideoName, pushClient->m_mediaCodecInfo.szAudioName, nClient);
		}
		else if ( strcmp(pushClient->m_mediaCodecInfo.szVideoName, "H264") == 0 || strcmp(pushClient->m_mediaCodecInfo.szVideoName, "H265") == 0)
		{//H264��H265 ֻ������Ƶ
			if (ABL_MediaServerPort.nEnableAudio == 0 || ABL_MediaServerPort.flvPlayAddMute == 0)
			{//û����Ƶ���������û�п�������
		  	   flvWrite = flv_writer_create2(0, 1, NetServerHTTP_FLV_OnWrite_CB, (void*)this);
			   WriteLog(Log_Debug, "����http-flv �����ʽΪ�� ��Ƶ %s����Ƶ������Ƶ  nClient = %llu ", pushClient->m_mediaCodecInfo.szVideoName, nClient);
			}
			else 
			{
#ifdef  OS_System_Windows //window �����Ӿ���
				flvWrite = flv_writer_create2(0, 1, NetServerHTTP_FLV_OnWrite_CB, (void*)this);
				WriteLog(Log_Debug, "����http-flv �����ʽΪ�� ��Ƶ %s����Ƶ������Ƶ  nClient = %llu ", pushClient->m_mediaCodecInfo.szVideoName, nClient);
#else 
				flvWrite = flv_writer_create2(1, 1, NetServerHTTP_FLV_OnWrite_CB, (void*)this);
				bAddMuteFlag = true;
				strcpy(mediaCodecInfo.szAudioName, "AAC");
				mediaCodecInfo.nChannels = 1;
				mediaCodecInfo.nSampleRate = 16000;
				mediaCodecInfo.nBaseAddAudioTimeStamp = 64;
				AddClientToMapAddMutePacketList(nClient);
				WriteLog(Log_Debug, "����http-flv �����ʽΪ�� ��Ƶ %s����Ƶ��AAC(chans:1,sampleRate:16000)  nClient = %llu ", pushClient->m_mediaCodecInfo.szVideoName, nClient);
#endif
			}
		}
		else if (strlen(pushClient->m_mediaCodecInfo.szVideoName) == 0 && (strcmp(pushClient->m_mediaCodecInfo.szAudioName, "AAC") == 0 || strcmp(pushClient->m_mediaCodecInfo.szAudioName, "MP3") == 0))
		{//ֻ������Ƶ
			flvWrite = flv_writer_create2(1,0, NetServerHTTP_FLV_OnWrite_CB, (void*)this);
			WriteLog(Log_Debug, "����http-flv �����ʽΪ�� ����Ƶ ��ֻ����Ƶ %s  nClient = %llu ", pushClient->m_mediaCodecInfo.szAudioName, nClient);
		}
		else
		{
			WriteLog(Log_Debug, "��Ƶ %s����Ƶ %s ��ʽ���󣬲�֧��http-flv ���,����ɾ�� nClient = %llu ",pushClient->m_mediaCodecInfo.szVideoName,pushClient->m_mediaCodecInfo.szAudioName,nClient);
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}

		if (flvWrite == NULL)
		{
			WriteLog(Log_Debug, "���� flv �������ʧ�� ");
			DeleteNetRevcBaseClient(nClient);
			return -1;
		}
#endif

		m_videoFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
		m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);

		//�ѿͻ��� ����Դ��ý�忽������ 
		pushClient->AddClientToMap(nClient);

	
	}
  
	return 0;
}

//���͵�һ������
int CNetServerHTTP_FLV::SendFirstRequst()
{
	return 0;
}

//����m3u8�ļ�
bool  CNetServerHTTP_FLV::RequestM3u8File()
{
	return true;
}