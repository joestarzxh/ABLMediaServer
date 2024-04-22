/*
���ܣ�
       ʵ��HLS��������ý�����ݷ��͹��� 
����    2021-05-20
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetServerHLS.h"

extern             bool                DeleteNetRevcBaseClient(NETHANDLE CltHandle);
boost::shared_ptr<CMediaStreamSource>  GetMediaStreamSource(char* szURL);
extern CMediaSendThreadPool*           pMediaSendThreadPool;
extern CMediaFifo                      pDisconnectBaseNetFifo; //������ѵ����� 
extern bool                            DeleteClientMediaStreamSource(uint64_t nClient);
extern CMediaFifo                      pRemoveBaseNetFromThreadFifo;       //��ý�忽���̡߳�ý�巢���߳��Ƴ���Client  
extern char                            ABL_wwwMediaPath[256]; //www ��·��
extern MediaServerPort                 ABL_MediaServerPort;
extern uint64_t                        ABL_nBaseCookieNumber ; //Cookie ��� 

CNetServerHLS::CNetServerHLS(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort)
{
	nServer = hServer;
	nClient = hClient;
	strcpy(szClientIP, szIP);
	nClientPort = nPort;
	memset(szOrigin, 0x00, sizeof(szOrigin));

	MaxNetDataCacheCount = MaxHttp_FlvNetCacheBufferLength;
	memset(netDataCache, 0x00, sizeof(netDataCache));
	netDataCacheLength = data_Length = nNetStart = nNetEnd = 0;//�������ݻ����С
	bFindHLSNameFlag = false;
	VideoFrameSpeed = 25;
	nRequestFileCount = 0;
	memset(szRequestFileName, 0x00, sizeof(szRequestFileName));

	netBaseNetType = NetBaseNetType_HttpHLSServerSendPush;
	bRequestHeadFlag = false;
	memset(szCookieNumber, 0x00, sizeof(szCookieNumber));

	WriteLog(Log_Debug, "CNetServerHLS = %X, ���� nClient = %llu ",this, nClient);
}

CNetServerHLS::~CNetServerHLS()
{
	XHNetSDK_Disconnect(nClient);

	//��ý�忽���̳߳��Ƴ�
	DeleteClientMediaStreamSource(nClient);

	WriteLog(Log_Debug, "CNetServerHLS = %X ���� szRequestFileName = %s, nClient = %llu \r\n", this, szRequestFileName, nClient);
}

int CNetServerHLS::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	if (strlen(mediaCodecInfo.szVideoName) == 0)
		strcpy(mediaCodecInfo.szVideoName, szVideoCodec);

	return 0 ;
}

int CNetServerHLS::PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	if (strlen(mediaCodecInfo.szAudioName) == 0)
	{
		strcpy(mediaCodecInfo.szAudioName, szAudioCodec);
		mediaCodecInfo.nChannels = nChannels;
		mediaCodecInfo.nSampleRate = SampleRate;
	}

	return 0;
}

int CNetServerHLS::SendVideo()
{

	return 0;
}

int CNetServerHLS::SendAudio()
{

	return 0;
}

int CNetServerHLS::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength)
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

//��HTTPͷ�л�ȡ������ļ���
bool CNetServerHLS::GetHttpRequestFileName(char* szGetRequestFile)
{
	string  strHttpHead = (char*)netDataCache;
	int     nPos1, nPos2;
	char    szFindHead[64] = { 0 };
 	int     nHeadLength = 4 ;

	strcpy(szFindHead, "HEAD ");
	nPos1 = strHttpHead.find(szFindHead, 0);
	if (nPos1 < 0)
	{
		memset(szFindHead, 0x00, sizeof(szFindHead));
		strcpy(szFindHead, "GET ");
		nPos1 = strHttpHead.find(szFindHead, 0);
	}
	else//����head ����m3u8 �ļ� 
		bRequestHeadFlag = true;

	nHeadLength = strlen(szFindHead);

	if (nPos1 >= 0)
	{
		nPos2 = strHttpHead.find(" HTTP/", 0);
		if (nPos2 > 0)
		{
			memcpy(szGetRequestFile, netDataCache + nPos1 + nHeadLength, nPos2 - nPos1 - nHeadLength);
 			WriteLog(Log_Debug, "CNetServerHLS=%X ,nClient = %llu ,������HTTP ���� �ļ����� %s ", this, nClient, szGetRequestFile);
			return true;
		}
	}
	return false;
}

int CNetServerHLS::ProcessNetData()
{
  	if (strstr((char*)netDataCache, "\r\n\r\n") == NULL)
	{
		WriteLog(Log_Debug, "������δ�������� ");
		return -1;
 	}

	memset(szDateTime1, 0x00, sizeof(szDateTime1));
	SYSTEMTIME st;
	GetLocalTime(&st);//Tue, Jun 31 2021 06:19:02 GMT
	sprintf(szDateTime1,"Tue, Jun %02d %04d %02d:%02d:%02d GMT", st.wDay, st.wYear, st.wHour - 8,st.wMinute, st.wSecond);
	sprintf(szDateTime2, "Tue, Jun %02d %04d %02d:%02d:%02d GMT", st.wDay, st.wYear, st.wHour - 8, st.wMinute+1, st.wSecond);

	netDataCache[netDataCacheLength] = 0x00;
	WriteLog(Log_Debug, "CNetServerHLS=%X ,�յ�HLSЭ��\r\n %s ", this, netDataCache);

	//����httpͷ
	httpParse.ParseSipString((char*)netDataCache);

	//��ȡ��HTTP������ļ����� 
	memset(szRequestFileName, 0x00, sizeof(szRequestFileName));
	if (!GetHttpRequestFileName(szRequestFileName))
	{
		WriteLog(Log_Debug, "CNetServerHLS=%X, ��ȡhttp �����ļ�ʧ�ܣ� nClient = %llu ", this, nClient);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return -1;
	}  

 	string strRequestFileName = szRequestFileName;
	int    nPos;
	nPos = strRequestFileName.rfind( "/",strlen(szRequestFileName));
	if (nPos > 0)
	{
		memset(szPushName, 0x00, sizeof(szPushName));
		memcpy(szPushName,szRequestFileName,nPos);
 	}
 
  	WriteLog(Log_Debug, "CNetServerHLS=%X, ��ȡ��HLS������Դ���� szPushName = %s , nClient = %llu ", this, szPushName, nClient);

	//��ȡConnection ���ӷ�ʽ�� Close ,Keep-Live  
	memset(szConnectionType, 0x00, sizeof(szConnectionType));
	if (!httpParse.GetFieldValue("Connection", szConnectionType))
		strcpy(szConnectionType, "Close");

	//����Cookie 
	char szTemp2[256] = { 0 };
	memset(szCookieNumber, 0x00, sizeof(szCookieNumber));
	srand(GetTickCount());
	while (true)
	{
		sprintf(szTemp2, "fd%dab%d%cf%ded%dcd%def%dba%ddc%dea%dea%dac%db%dcd%daf%dbc%daf%dbd%d", rand(), rand(), rand(), rand(), rand(), rand(), rand(),
			rand(), rand(), rand(), rand(), rand(), rand(), rand(), rand(), rand(), rand());
	  for (int i = 0; i < strlen(szTemp2); i++)
	  {
		  if (szTemp2[i] == '.')
			  szTemp2[i] = 'a';
		  else if (szTemp2[i] == '-')
			  szTemp2[i] = 'b';
	  }
	  if (strlen(szTemp2) >= 32)
	  {
		  memcpy(szCookieNumber, szTemp2, 32);
		  break;
	  }
	}
	//strcpy(szCookieNumber, "fab5a4eb786e9396a48c84011cad3679");
 
	//��ȡ szOrigin
	memset(szOrigin, 0x00, sizeof(szOrigin));
	httpParse.GetFieldValue("Origin", szOrigin);
	//strcpy(szOrigin, "http://cloud.liveqing.com:10080");
	WriteLog(Log_Debug, "CNetServerHLS=%X, ��ȡ�� Origin = %s , nClient = %llu ", this, szOrigin, nClient);

	//�������������ҵ�
	boost::shared_ptr<CMediaStreamSource> pushClient = GetMediaStreamSource(szPushName);
	if (pushClient == NULL)
	{
		WriteLog(Log_Debug, "CNetServerHLS=%X, û����������ĵ�ַ %s nClient = %llu ", this, szPushName, nClient);

		sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return -1;
	}

	nRequestFileCount ++;
	//�ѿͻ��� ����Դ��ý�忽������ ������HLS Э�飬����Ҫ����
	if(nRequestFileCount == 1)
	  pushClient->AddClientToMap(nClient);

	if (strstr(szRequestFileName, ".m3u8") != NULL)
	{//����M3U8�ļ�
		if (strlen(pushClient->szDataM3U8) == 0)
		{//m3u8�ļ���δ���ɣ����ոտ�ʼ��Ƭ�����ǻ�����3���ļ� 
			WriteLog(Log_Debug, "CNetServerHLS=%X, m3u8�ļ���δ���ɣ����ոտ�ʼ��Ƭ�����ǻ�����3��TS�ļ� %s nClient = %llu ", this, szPushName, nClient);

			sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
 		}

		memset(szM3u8Content, 0x00, sizeof(szM3u8Content));
		strcpy(szM3u8Content, pushClient->szDataM3U8);
		if (bRequestHeadFlag == true)
		{//HEAD ����
			sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: close\r\nContent-Length: 0\r\nDate: %s\r\nServer: %s\r\n\r\n", szOrigin, szDateTime1, MediaServerVerson);
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);
			WriteLog(Log_Debug, "CNetServerHLS=%X, �ظ�HEAD���� httpResponseData = %s, nClient = %llu ", this, httpResponseData, nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return 0;
		}
		else
		{
		  sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: %s\r\nContent-Length: %d\r\nContent-Type: application/vnd.apple.mpegurl; charset=utf-8\r\nDate: %s\r\nKeep-Alive: timeout=30, max=100\r\nServer: %s\r\nSet-Cookie: ZL_COOKIE=%s;expires=%s;path=%s/\r\n\r\n", szOrigin, szConnectionType,
			strlen(szM3u8Content), szDateTime1, MediaServerVerson, szCookieNumber,szDateTime2, szPushName);
 		}

		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);
 		nWriteRet2 = XHNetSDK_Write(nClient, (unsigned char*)szM3u8Content, strlen(szM3u8Content), 1);
		if (nWriteRet != 0 || nWriteRet2 != 0)
		{
			WriteLog(Log_Debug, "CNetServerHLS=%X, �ظ�httpʧ�� szRequestFileName = %s, nClient = %llu ", this, szRequestFileName, nClient);
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}

		WriteLog(Log_Debug, "CNetServerHLS=%X, nClient = %llu ����http�ظ���\r\n%s", this, nClient, httpResponseData);
		WriteLog(Log_Debug, "CNetServerHLS=%X, nClient = %llu ����http�ظ���\r\n%s", this, nClient, szM3u8Content);
	}
	else if (strstr(szRequestFileName, ".ts") != NULL)
	{//����TS�ļ� 
		nTsFileNameOrder = GetTsFileNameOrder(szRequestFileName);
		if (nTsFileNameOrder == -1)
		{
			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}

		//TS�ļ����ֽ���
		fFileByteCount = pushClient->GetTsFileSizeByOrder(nTsFileNameOrder);
		if (fFileByteCount <= 0)
		{//TS�ļ��ֽ���������
			WriteLog(Log_Debug, "CNetServerHLS=%X, �ļ���ʧ�� szReadFileName = %s nClient = %llu ", this, szReadFileName, nClient);

			sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
			nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

			pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
			return -1;
		}
		unsigned char*  pTsFileBuffer = new unsigned char[fFileByteCount];

		if (ABL_MediaServerPort.nHLSCutType == 1)
		{//��Ƭ��Ӳ��
			sprintf(szReadFileName, "%s\\%s", ABL_wwwMediaPath, szRequestFileName);
			WriteLog(Log_Debug, "CNetServerHLS=%X, ��ʼ��ȡTS�ļ� szReadFileName = %s nClient = %llu ", this, szReadFileName, nClient);
			FILE* tsFile = fopen(szReadFileName,"rb");
			if (tsFile == NULL)
			{//��TS�ļ�ʧ��
				WriteLog(Log_Debug, "CNetServerHLS=%X, �ļ���ʧ�� szReadFileName = %s nClient = %llu ", this, szReadFileName, nClient);

				sprintf(httpResponseData, "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nDate: Thu, Feb 18 2021 01:57:15 GMT\r\nKeep-Alive: timeout=30, max=100\r\nAccess-Control-Allow-Origin: *\r\nServer: %s\r\n\r\n", MediaServerVerson);
				nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				return -1;
			}
    		fread(pTsFileBuffer,1, fFileByteCount, tsFile);
 			fclose(tsFile);
		}
		else if (ABL_MediaServerPort.nHLSCutType == 2)
		{
  			pushClient->CopyTsFileBuffer(nTsFileNameOrder, pTsFileBuffer);
   		}

		//����httpͷ
		sprintf(httpResponseData, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Allow-Origin: %s\r\nConnection: %s\r\nContent-Length: %d\r\nContent-Type: video/mp2t; charset=utf-8\r\nDate: %s\r\nkeep-Alive: timeout=30, max=100\r\nServer: %s\r\n\r\n", szOrigin,
			szConnectionType,
			fFileByteCount,
			szDateTime1,
			MediaServerVerson);
		nWriteRet = XHNetSDK_Write(nClient, (unsigned char*)httpResponseData, strlen(httpResponseData), 1);

		//����TS���� 
		int             nPos = 0;
 		while (fFileByteCount > 0 && pTsFileBuffer != NULL)
		{
			if (fFileByteCount > Send_TsFile_MaxPacketCount)
			{
				nWriteRet2 = XHNetSDK_Write(nClient, (unsigned char*)pTsFileBuffer + nPos, Send_TsFile_MaxPacketCount, 1);
				fFileByteCount -= Send_TsFile_MaxPacketCount;
				nPos += Send_TsFile_MaxPacketCount;
			}
			else
			{
				nWriteRet2 = XHNetSDK_Write(nClient, (unsigned char*)pTsFileBuffer + nPos, fFileByteCount, 1);
				nPos += fFileByteCount;
				fFileByteCount = 0;
			}

			if (nWriteRet2 != 0)
			{//���ͳ���
				pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
				break;
			}
		}
		delete[] pTsFileBuffer;
		pTsFileBuffer = NULL;
	}
	else
	{
		WriteLog(Log_Debug, "CNetServerHLS=%X, ���� http �ļ��������� szRequestFileName = %s, nClient = %llu ", this, szRequestFileName, nClient);
		pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
		return -1;
	}

	//�������,����Ƕ����ӣ�����ɾ��
	if(strcmp(szConnectionType,"Close") == 0 || strcmp(szConnectionType, "close") || bRequestHeadFlag == true)
	  pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));

	netDataCacheLength = 0;

	return 0;
}

//����TS�ļ����� ����ȡ�ļ���� 
int64_t  CNetServerHLS::GetTsFileNameOrder(char* szTsFileName)
{
	string strTsFileName = szTsFileName;
	int    nPos1, nPos2;
	char   szTemp[128] = { 0 };

	nPos1 = strTsFileName.rfind("/", strlen(szTsFileName));
	nPos2 = strTsFileName.find(".ts", 0);
	if (nPos1 > 0 && nPos2 > 0)
	{
		memcpy(szTemp, szTsFileName + nPos1 + 1, nPos2 - nPos1 - 1);
		return atoi(szTemp);
	}

	return  -1 ;
}

