/*
���ܣ�
    ������ GB28181 Rtp ����������UDP��TCPģʽ  
 	���� �������  �������귢�͵�ͬʱҲ֧�ֹ�����գ�    2023-05-19
����    2021-08-15
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetGB28181RtpClient.h"
#ifdef USE_BOOST
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);


extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern boost::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern MediaServerPort                       ABL_MediaServerPort;
extern int                                   SampleRateArray[];


#else
extern bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
extern std::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
extern std::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);
extern bool                                  DeleteMediaStreamSource(char* szURL);
extern bool                                  DeleteClientMediaStreamSource(uint64_t nClient);


extern CMediaFifo                            pDisconnectBaseNetFifo; //������ѵ����� 
extern char                                  ABL_MediaSeverRunPath[256]; //��ǰ·��
extern std::shared_ptr<CNetRevcBase>       CreateNetRevcBaseClient(int netClientType, NETHANDLE serverHandle, NETHANDLE CltHandle, char* szIP, unsigned short nPort, char* szShareMediaURL);
extern MediaServerPort                       ABL_MediaServerPort;
extern int                                   SampleRateArray[];
#endif
void PS_MUX_CALL_METHOD GB28181_Send_mux_callback(_ps_mux_cb* cb)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)cb->userdata;
	if (pThis == NULL || !pThis->bRunFlag)
		return;
 
	pThis->GB28181PsToRtPacket(cb->data, cb->datasize);

#ifdef  WriteGB28181PSFileFlag
	fwrite(cb->data,1,cb->datasize,pThis->writePsFile);
	fflush(pThis->writePsFile);
#endif
}

static void* ps_alloc(void* param, size_t bytes)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)param;
	 
	return pThis->s_buffer;
}

static void ps_free(void* param, void* /*packet*/)
{
	return;
}

static int ps_write(void* param, int stream, void* packet, size_t bytes)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)param;

	if(pThis->bRunFlag)
	  pThis->GB28181PsToRtPacket((unsigned char*)packet, bytes);

	return true;
}

//rtp����ص���Ƶ
void GB28181_rtp_packet_callback_func_send(_rtp_packet_cb* cb)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)cb->userdata;
	if (pThis == NULL || !pThis->bRunFlag)
		return;

	if (pThis->netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
	{//udp ֱ�ӷ��� 
		pThis->SendGBRtpPacketUDP(cb->data, cb->datasize);
	}
	else if (pThis->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect || pThis->netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
	{//TCP ��Ҫƴ�� ��ͷ
		pThis->GB28181SentRtpVideoData(cb->data, cb->datasize);
	}
}

//PS ���ݴ����rtp 
void  CNetGB28181RtpClient::GB28181PsToRtPacket(unsigned char* pPsData, int nLength)
{
	if(hRtpPS > 0 && bRunFlag)
	{
		inputPS.data = pPsData;
		inputPS.datasize = nLength;
		rtp_packet_input(&inputPS);
	}
}

//����28181PS����TCP��ʽ���� 
void  CNetGB28181RtpClient::GB28181SentRtpVideoData(unsigned char* pRtpVideo, int nDataLength)
{
	if (bRunFlag == false)
		return;
	
	if ((nMaxRtpSendVideoMediaBufferLength - nSendRtpVideoMediaBufferLength < nDataLength + 4) && nSendRtpVideoMediaBufferLength > 0)
	{//ʣ��ռ䲻���洢 ,��ֹ���� 
 		nSendRet = XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nSendRtpVideoMediaBufferLength, 1);
		if (nSendRet != 0)
		{
			bRunFlag = false;

			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, ���͹���RTP�������� ��Length = %d ,nSendRet = %d", this, nSendRtpVideoMediaBufferLength, nSendRet);
			DeleteNetRevcBaseClient(nClient);
			return;
		}

		nSendRtpVideoMediaBufferLength = 0;
	}

	memcpy((char*)&nCurrentVideoTimestamp, pRtpVideo + 4, sizeof(uint32_t));
	if (nStartVideoTimestamp != GB28181VideoStartTimestampFlag &&  nStartVideoTimestamp != nCurrentVideoTimestamp && nSendRtpVideoMediaBufferLength > 0)
	{//����һ֡�µ���Ƶ 
		nSendRet = XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nSendRtpVideoMediaBufferLength, 1);
		if (nSendRet != 0)
		{
			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, ���͹���RTP�������� ��Length = %d ,nSendRet = %d", this, nSendRtpVideoMediaBufferLength, nSendRet);
			DeleteNetRevcBaseClient(nClient);
			return;
		}

		nSendRtpVideoMediaBufferLength = 0;
	}

	if (ABL_MediaServerPort.nGBRtpTCPHeadType == 1)
	{//���� TCP���� 4���ֽڷ�ʽ
		szSendRtpVideoMediaBuffer[nSendRtpVideoMediaBufferLength + 0] = '$';
		szSendRtpVideoMediaBuffer[nSendRtpVideoMediaBufferLength + 1] = 0;
		nVideoRtpLen = htons(nDataLength);
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 2), (unsigned char*)&nVideoRtpLen, sizeof(nVideoRtpLen));
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 4), pRtpVideo, nDataLength);

		nStartVideoTimestamp = nCurrentVideoTimestamp;
 		nSendRtpVideoMediaBufferLength += nDataLength + 4;
	}
	else if (ABL_MediaServerPort.nGBRtpTCPHeadType == 2)
	{//���� TCP���� 2 ���ֽڷ�ʽ
		nVideoRtpLen = htons(nDataLength);
		memcpy(szSendRtpVideoMediaBuffer + nSendRtpVideoMediaBufferLength, (unsigned char*)&nVideoRtpLen, sizeof(nVideoRtpLen));
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 2), pRtpVideo, nDataLength);

		nStartVideoTimestamp = nCurrentVideoTimestamp;
 		nSendRtpVideoMediaBufferLength += nDataLength + 2;
	}
	else
	{
		bRunFlag = false;
		WriteLog(Log_Debug, "CNetGB28181RtpClient = %X, �Ƿ��Ĺ���TCP��ͷ���ͷ�ʽ(����Ϊ 1��2 )nGBRtpTCPHeadType = %d ", this, ABL_MediaServerPort.nGBRtpTCPHeadType);
		DeleteNetRevcBaseClient(nClient);
	}
}

CNetGB28181RtpClient::CNetGB28181RtpClient(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort,char* szShareMediaURL)
{
	hRtpHandle = 0;
	nRecvSampleRate =  nRecvChannels = 0;
	nPFrameCount = 0;
	memset((char*)&gbDstAddr, 0x00, sizeof(gbDstAddr));
	memset((char*)&jt1078VideoHead, 0x00, sizeof(jt1078VideoHead));
	memset((char*)&jt1078AudioHead, 0x00, sizeof(jt1078AudioHead));
	memset((char*)&jt1078OtherHead, 0x00, sizeof(jt1078OtherHead));
	memset((char*)&jt1078VideoHead2019, 0x00, sizeof(jt1078VideoHead2019));
	memset((char*)&jt1078AudioHead2019, 0x00, sizeof(jt1078AudioHead2019));
	memset((char*)&jt1078OtherHead2019, 0x00, sizeof(jt1078OtherHead2019));
	jt1078VideoHead.head[0] = 0x30;
	jt1078VideoHead.head[1] = 0x31;
	jt1078VideoHead.head[2] = 0x63;
	jt1078VideoHead.head[3] = 0x64;
	memcpy(jt1078AudioHead.head, jt1078VideoHead.head,4);
	memcpy(jt1078OtherHead.head, jt1078VideoHead.head, 4);

 	memcpy(jt1078VideoHead2019.head, jt1078VideoHead.head, 4);
	memcpy(jt1078AudioHead2019.head, jt1078VideoHead.head, 4);
	memcpy(jt1078OtherHead2019.head, jt1078VideoHead.head, 4);

	jt1078VideoHead.v = jt1078AudioHead.v = jt1078OtherHead.v = jt1078VideoHead2019.v = jt1078AudioHead2019.v = jt1078OtherHead2019.v = 2;
	jt1078VideoHead.cc = jt1078AudioHead.cc = jt1078OtherHead.cc = jt1078VideoHead2019.cc = jt1078AudioHead2019.cc = jt1078OtherHead2019.cc = 1;
	jt1078VideoHead.m = jt1078AudioHead.m = jt1078OtherHead.m = jt1078VideoHead2019.m = jt1078AudioHead2019.m = jt1078OtherHead2019.m = 1;
	jt1078VideoHead.ch = jt1078AudioHead.ch = jt1078OtherHead.ch = jt1078VideoHead2019.ch = jt1078AudioHead2019.ch = jt1078OtherHead2019.ch = 1;

	nSendRtcpTime = GetTickCount64();
	memset(m_recvMediaSource, 0x00, sizeof(m_recvMediaSource));
	pRecvMediaSource = NULL;
	psBeiJingLaoChenDemuxer = NULL;
	netDataCache = NULL; //�������ݻ���
	netDataCacheLength = 0;//�������ݻ����С
	nNetStart = nNetEnd = 0; //����������ʼλ��\����λ��
	MaxNetDataCacheCount = 1024*1024*2;
	nRtpRtcpPacketType = 0;

	nMaxRtpSendVideoMediaBufferLength = 640;//Ĭ���ۼ�640
	strcpy(m_szShareMediaURL,szShareMediaURL);
	nClient = hClient;
	nServer = hServer;
	psMuxHandle = 0;

	nVideoStreamID = nAudioStreamID = -1;
	handler.alloc = ps_alloc;
	handler.write = ps_write;
	handler.free = ps_free;
    videoPTS = audioPTS = 0;
	s_buffer = NULL;
	psBeiJingLaoChen = NULL;
	if (ABL_MediaServerPort.gb28181LibraryUse == 2)
	{
		s_buffer = new  char[IDRFrameMaxBufferLength];
		psBeiJingLaoChen = ps_muxer_create(&handler, this);
	}
	hRtpPS = 0;
	bRunFlag = true;

	nSendRtpVideoMediaBufferLength = 0; //�Ѿ����۵ĳ���  ��Ƶ
	nStartVideoTimestamp           = GB28181VideoStartTimestampFlag ; //��һ֡��Ƶ��ʼʱ��� ��
	nCurrentVideoTimestamp         = 0;// ��ǰ֡ʱ���

	m_videoFifo.InitFifo(MaxLiveingVideoFifoBufferLength);
	m_audioFifo.InitFifo(MaxLiveingAudioFifoBufferLength);

#ifdef  WriteGB28181PSFileFlag
	char    szFileName[256] = { 0 };
	sprintf(szFileName, "%s%X.ps", ABL_MediaSeverRunPath,this);
	writePsFile = fopen(szFileName,"wb");
#endif
#ifdef WriteRecvPSDataFlag
	fWritePSDataFile = fopen("E:\\recv_app_recv.ps","wb");
#endif
#ifdef WriteJtt1078SrcVideoFlag
	char    szFileName[256] = { 0 };
	sprintf(szFileName, "%s%1078_X.264", ABL_MediaSeverRunPath, this);
	fWrite1078SrcFile = fopen(szFileName, "wb");
#endif

 	WriteLog(Log_Debug, "CNetGB28181RtpClient ���� = %X  nClient = %llu ", this, nClient);
}

CNetGB28181RtpClient::~CNetGB28181RtpClient()
{
	bRunFlag = false;
	std::lock_guard<std::mutex> lock(businessProcMutex);

	if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
	{
		XHNetSDK_DestoryUdp(nClient);
		XHNetSDK_DestoryUdp(nClientRtcp);
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect )
	{
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
	{
		pDisconnectBaseNetFifo.push((unsigned char*)&hParent, sizeof(hParent));
	}
	m_videoFifo.FreeFifo();
	m_audioFifo.FreeFifo();
	ps_mux_stop(psDeMuxHandle);
	rtp_packet_stop(hRtpPS);
	if(psBeiJingLaoChen != NULL )
	  ps_muxer_destroy(psBeiJingLaoChen);
	if (psBeiJingLaoChenDemuxer != NULL)
		ps_demuxer_destroy(psBeiJingLaoChenDemuxer);
	SAFE_ARRAY_DELETE(s_buffer);
	SAFE_ARRAY_DELETE(netDataCache);
	//����ɾ��ý��Դ
	if (strlen(m_recvMediaSource) > 0 && pRecvMediaSource != NULL)
		DeleteMediaStreamSource(m_recvMediaSource);

#ifdef  WriteGB28181PSFileFlag
	fclose(writePsFile);
#endif
#ifdef WriteRecvPSDataFlag
	if(fWritePSDataFile != NULL)
	  fclose(fWritePSDataFile);
#endif
#ifdef WriteJtt1078SrcVideoFlag
	fclose(fWrite1078SrcFile);
#endif

	WriteLog(Log_Debug, "CNetGB28181RtpClient ���� = %X  nClient = %llu ,nMediaClient = %llu\r\n", this, nClient, nMediaClient);
	malloc_trim(0);
}

int CNetGB28181RtpClient::PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec)
{
	nRecvDataTimerBySecond = 0 ;
	if (!bRunFlag || m_startSendRtpStruct.disableVideo[0] == 0x31)
		return -1;
	std::lock_guard<std::mutex> lock(businessProcMutex);

	if (strlen(mediaCodecInfo.szVideoName) == 0)
		strcpy(mediaCodecInfo.szVideoName, szVideoCodec);

	m_videoFifo.push(pVideoData, nDataLength);
	return 0;
}

int CNetGB28181RtpClient::PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate)
{
	nRecvDataTimerBySecond = 0;

	//��������Ƶʱ����������Ƶ���
	if (!bRunFlag || m_startSendRtpStruct.disableAudio[0] == 0x31)
		return -1;

	std::lock_guard<std::mutex> lock(businessProcMutex);

	if (strlen(mediaCodecInfo.szAudioName) == 0)
	{
		strcpy(mediaCodecInfo.szAudioName, szAudioCodec);
		mediaCodecInfo.nChannels = nChannels;
		mediaCodecInfo.nSampleRate = SampleRate;
	}

	if (ABL_MediaServerPort.nEnableAudio == 0)
		return -1;

	m_audioFifo.push(pVideoData, nDataLength);
	return 0;
}

void  CNetGB28181RtpClient::CreateRtpHandle()
{
	if (hRtpPS == 0)
	{
		if (strcmp(m_startSendRtpStruct.disableVideo, "1") == 0)
			nMaxRtpSendVideoMediaBufferLength = 640;
		else
			nMaxRtpSendVideoMediaBufferLength = MaxRtpSendVideoMediaBufferLength ;

		int nRet = rtp_packet_start(GB28181_rtp_packet_callback_func_send, (void*)this, &hRtpPS);
		if (nRet != e_rtppkt_err_noerror)
		{
			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X ��������Ƶrtp���ʧ��,nClient = %llu,  nRet = %d", this, nClient, nRet);
			return ;
		}
		optionPS.handle = hRtpPS;
		if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x31)
		{//����PS���
			optionPS.mediatype = e_rtppkt_mt_video;
			optionPS.payload = atoi(m_startSendRtpStruct.payload);
			optionPS.streamtype = e_rtppkt_st_gb28181;
			optionPS.ssrc = atoi(m_startSendRtpStruct.ssrc);
			optionPS.ttincre = (90000 / mediaCodecInfo.nVideoFrameRate);
			rtp_packet_setsessionopt(&optionPS);
		}
		else if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x32 || m_startSendRtpStruct.RtpPayloadDataType[0] == 0x33)
		{//ES \ XHB ���
			if (atoi(m_startSendRtpStruct.disableAudio) == 1)
			{//ֻ����Ƶ
				optionPS.mediatype = e_rtppkt_mt_video;
				if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
				{
					strcpy(m_startSendRtpStruct.payload, "98");
					optionPS.payload = 98;
					optionPS.streamtype = e_rtppkt_st_h264;
				}
				else  if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
				{
					strcpy(m_startSendRtpStruct.payload, "99");
					optionPS.payload = 99;
					optionPS.streamtype = e_rtppkt_st_h265;
				}
				optionPS.ssrc = atoi(m_startSendRtpStruct.ssrc);
				optionPS.ttincre = (90000 / mediaCodecInfo.nVideoFrameRate);
 			}
			else if (atoi(m_startSendRtpStruct.disableVideo) == 1)
			{//ֻ����Ƶ 
				optionPS.mediatype = e_rtppkt_mt_audio;
				optionPS.ssrc = atoi(m_startSendRtpStruct.ssrc);
				if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
				{
					optionPS.ttincre = 320; 
					optionPS.streamtype = e_rtppkt_st_g711a;
					optionPS.payload = 8;
				}else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
				{
					optionPS.ttincre = 320;  
					optionPS.streamtype = e_rtppkt_st_g711u;
					optionPS.payload = 0;
				}
				else if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0) 
				{
					optionPS.ttincre = 1024;  
					optionPS.streamtype = e_rtppkt_st_aac_have_adts;
					optionPS.payload = 97;
				}
 			}
			rtp_packet_setsessionopt(&optionPS);
		}

		inputPS.handle = hRtpPS;
		inputPS.ssrc = optionPS.ssrc;

		memset((char*)&gbDstAddr, 0x00, sizeof(gbDstAddr));
		gbDstAddr.sin_family = AF_INET;
		gbDstAddr.sin_addr.s_addr = inet_addr(m_startSendRtpStruct.dst_url);
		gbDstAddr.sin_port = htons(atoi(m_startSendRtpStruct.dst_port));
		memset((char*)&gbDstAddrRTCP, 0x00, sizeof(gbDstAddrRTCP));
		gbDstAddrRTCP.sin_family = AF_INET;
		gbDstAddrRTCP.sin_addr.s_addr = inet_addr(m_startSendRtpStruct.dst_url);
		gbDstAddrRTCP.sin_port = htons(atoi(m_startSendRtpStruct.dst_port)+1);//rtcp�˿�

		//����ý��Դ
		SplitterAppStream(m_szShareMediaURL);
		sprintf(m_addStreamProxyStruct.url, "rtp://%s:%s/%s/%s", m_startSendRtpStruct.dst_url, m_startSendRtpStruct.dst_port,m_addStreamProxyStruct.app, m_addStreamProxyStruct.stream);
		strcpy(szClientIP, m_startSendRtpStruct.dst_url);
	}
}

int CNetGB28181RtpClient::SendVideo()
{
	if (!bRunFlag )
		return -1;

	unsigned char* pData = NULL;
	int            nLength = 0;

	//�������ɹ������ӳɹ�
	if (!bUpdateVideoFrameSpeedFlag)
		bUpdateVideoFrameSpeedFlag = true;

	//jtt1078 
	if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x34)
	{
		if(strcmp(m_startSendRtpStruct.jtt1078_version,"2013") == 0 || strcmp(m_startSendRtpStruct.jtt1078_version, "2016") == 0)
		   SendJtt1078VideoPacket();
		else if(strcmp(m_startSendRtpStruct.jtt1078_version, "2019") == 0)
		   SendJtt1078VideoPacket2019();
		return 0;//���ʽֱ�ӷ���
	}

	if (ABL_MediaServerPort.gb28181LibraryUse == 1)
	{//����
		if (psMuxHandle == 0)
		{
			memset(&init, 0, sizeof(init));
			init.cb = (void*)GB28181_Send_mux_callback;
			init.userdata = this;
			init.alignmode = e_psmux_am_4octet;
			init.ttmode = 0;
			init.ttincre = (90000 / mediaCodecInfo.nVideoFrameRate);
			init.h = &psMuxHandle;
			int32_t ret = ps_mux_start(&init);

			input.handle = psMuxHandle;

			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X ������ ps ����ɹ�  ,nClient = %llu,  nRet = %d", this, nClient, ret);
		}
	}
	else
	{//�����ϳ�
		if (nVideoStreamID == -1 && psBeiJingLaoChen != NULL )
		{
			if (strcmp(mediaCodecInfo.szVideoName,"H264") == 0 )
			  nVideoStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChen, PSI_STREAM_H264, NULL, 0);
			else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			  nVideoStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChen, PSI_STREAM_H265, NULL, 0);
		}
	}

	//����rtp���
	CreateRtpHandle();

	if ((pData = m_videoFifo.pop(&nLength)) != NULL)
	{
		input.mediatype = e_psmux_mt_video;
		if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			input.streamtype = e_psmux_st_h264;
		else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			input.streamtype = e_psmux_st_h265;
		else
		{
			m_videoFifo.pop_front();
			return 0;
		}

		if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x31)
		{//PS ���
			//����PS���
			if (ABL_MediaServerPort.gb28181LibraryUse == 1)
			{
	  		  input.data = pData;
			  input.datasize = nLength;
			  ps_mux_input(&input);
			}
			else
			{//�����ϳ�PS���
				if (nVideoStreamID != -1 && psBeiJingLaoChen != NULL && strlen(mediaCodecInfo.szVideoName) > 0 )
				{
					if(strcmp(mediaCodecInfo.szVideoName,"H264") == 0)
					  nflags = CheckVideoIsIFrame("H264", pData, nLength);
					else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
					  nflags = CheckVideoIsIFrame("H265", pData, nLength);

					ps_muxer_input((ps_muxer_t*)psBeiJingLaoChen, nVideoStreamID, nflags, videoPTS, videoPTS, pData, nLength);
					videoPTS += (90000 / mediaCodecInfo.nVideoFrameRate);
				}
			}
		}
		else if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x32 || m_startSendRtpStruct.RtpPayloadDataType[0] == 0x33)
		{//ES ��� \ XHB ���
			if (hRtpPS > 0 && bRunFlag)
			{
				inputPS.data = pData;
				inputPS.datasize = nLength;
				rtp_packet_input(&inputPS);
			}
		}
 
		m_videoFifo.pop_front();
	}
	return 0;
}

int CNetGB28181RtpClient::SendAudio()
{
	if ( ABL_MediaServerPort.nEnableAudio == 0 || !bRunFlag || m_startSendRtpStruct.disableAudio[0] == 0x31 )
		return 0;

	//jtt1078 
	if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x34)
	{
		if (strcmp(m_startSendRtpStruct.jtt1078_version, "2013") == 0 || strcmp(m_startSendRtpStruct.jtt1078_version, "2016") == 0)
			SendJtt1078AduioPacket();
		else if (strcmp(m_startSendRtpStruct.jtt1078_version, "2019") == 0)
			SendJtt1078AduioPacket2019();
		return 0;
	}

	unsigned char* pData = NULL;
	int            nLength = 0;
	if ((pData = m_audioFifo.pop(&nLength)) != NULL)
	{
		input.mediatype = e_psmux_mt_audio;
		if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
			input.streamtype = e_psmux_st_aac;
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
			input.streamtype = e_psmux_st_g711a;
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
			input.streamtype = e_psmux_st_g711u;
		else
		{
			m_audioFifo.pop_front();
			return 0;
		}

		//����rtp���
		CreateRtpHandle();

		if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x31)
		{//����PS���ʱ,���԰���Ƶ����Ƶ�����һ��,����ES���,��Ƶ\��Ƶ ���ܴ����һ��
			//����PS���
			if (ABL_MediaServerPort.gb28181LibraryUse == 1)
			{
			   input.data = pData;
			   input.datasize = nLength;
 			   ps_mux_input(&input);
			}
			else
			{//�����ϳ�PS���
				if (nAudioStreamID == -1 && psBeiJingLaoChen != NULL && strlen(mediaCodecInfo.szAudioName) > 0 )
				{
					if ( strcmp(mediaCodecInfo.szAudioName,"AAC") == 0 )
						nAudioStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChen, PSI_STREAM_AAC, NULL, 0);
					else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
						nAudioStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChen, PSI_STREAM_AUDIO_G711A, NULL, 0);
					else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
						nAudioStreamID = ps_muxer_add_stream((ps_muxer_t*)psBeiJingLaoChen, PSI_STREAM_AUDIO_G711U, NULL, 0);
				}

				if (nAudioStreamID != -1 && psBeiJingLaoChen != NULL && strlen(mediaCodecInfo.szAudioName) > 0 )
				{
					ps_muxer_input((ps_muxer_t*)psBeiJingLaoChen, nAudioStreamID, 0, audioPTS, audioPTS, pData, nLength);

					if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
						audioPTS += mediaCodecInfo.nBaseAddAudioTimeStamp;
					else if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0 || strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
						audioPTS += nLength / 8;
				}
			}
		}
		else if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x32 || m_startSendRtpStruct.RtpPayloadDataType[0] == 0x33)
		{//ES\XHB ���
 			inputPS.data = pData;
			inputPS.datasize = nLength;
 		    rtp_packet_input(&inputPS);
 		}
 
		m_audioFifo.pop_front();
	}
	return 0;
}

//udp��ʽ����rtp��
void  CNetGB28181RtpClient::SendGBRtpPacketUDP(unsigned char* pRtpData, int nLength)
{
	XHNetSDK_Sendto(nClient, pRtpData, nLength, (void*)&gbDstAddr);

 	if (GetTickCount64() - nSendRtcpTime >= 5 * 1000 ) 
	{//��������rtcp��
		nSendRtcpTime = GetTickCount64();

		memset(szRtcpSRBuffer, 0x00, sizeof(szRtcpSRBuffer));
		rtcpSRBufferLength = sizeof(szRtcpSRBuffer);
		rtcpSR.BuildRtcpPacket(szRtcpSRBuffer, rtcpSRBufferLength, nSSRC);

		XHNetSDK_Sendto(nClientRtcp, szRtcpSRBuffer, rtcpSRBufferLength, (void*)&gbDstAddrRTCP);
	}
}

int CNetGB28181RtpClient::InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address)
{
 	std::lock_guard<std::mutex> lock(businessProcMutex);
	if (!bRunFlag || nDataLength <= 0 )
		return -1;
	if (!(strlen(m_startSendRtpStruct.recv_app) > 0 && strlen(m_startSendRtpStruct.recv_stream) > 0))
		return -1;

	if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
	{//UDP
	   //��ȡrtp���İ�ͷ
		rtpHeadPtr = (_rtp_header*)(pData);

		//���ȺϷ� ������rtp�� ��rtpHeadPtr->v == 2) ,��ֹrtcp����ִ��rtp���
		if (nDataLength > 0 && nDataLength < 1500  && rtpHeadPtr->v == 2)
 		  RtpDepacket(pData, nDataLength);
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect  || netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
	{//TCP 
		if (netDataCache == NULL)
			netDataCache = new unsigned char[MaxNetDataCacheCount];

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
					WriteLog(Log_Debug, "CNetGB28181RtpClient = %X nClient = %llu �����쳣 , ִ��ɾ��", this, nClient);
					pDisconnectBaseNetFifo.push((unsigned char*)&nClient, sizeof(nClient));
					return 0;
				}
			}
			else
			{//û��ʣ�࣬��ô �ף�βָ�붼Ҫ��λ 
				nNetStart = nNetEnd = netDataCacheLength = 0;
			}
			memcpy(netDataCache + nNetEnd, pData, nDataLength);
			netDataCacheLength += nDataLength;
			nNetEnd += nDataLength;
		}
	}

    return 0;
}

int CNetGB28181RtpClient::ProcessNetData()
{
 	std::lock_guard<std::mutex> lock(businessProcMutex);
	if (!bRunFlag)
		return -1;
	if (!(strlen(m_startSendRtpStruct.recv_app) > 0 && strlen(m_startSendRtpStruct.recv_stream) > 0))
		return -1;

	unsigned char* pData = NULL;
	int            nLength;

	if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
	{//UDP
 
	}
	else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect  || netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
	{//TCP ��ʽ��rtp����ȡ 
		while (netDataCacheLength > 2048)
		{//���ܻ���̫��buffer,������ɽ��չ�������ֻ����Ƶ��ʱ���������ʱ�ܴ� 2048 �ȽϺ��� 
			memcpy(rtpHeadOfTCP, netDataCache + nNetStart, 2);
			if ((rtpHeadOfTCP[0] == 0x24 && rtpHeadOfTCP[1] == 0x00) || (rtpHeadOfTCP[0] == 0x24 && rtpHeadOfTCP[1] == 0x01) || (rtpHeadOfTCP[0] == 0x24))
			{
				nNetStart += 2;
				memcpy((char*)&nRtpLength, netDataCache + nNetStart, 2);
				nNetStart += 2;
				netDataCacheLength -= 4;

				if (nRtpRtcpPacketType == 0)
					nRtpRtcpPacketType = 2;//4���ֽڷ�ʽ
			}
			else
			{
				memcpy((char*)&nRtpLength, netDataCache + nNetStart, 2);
				nNetStart += 2;
				netDataCacheLength -= 2;

				if (nRtpRtcpPacketType == 0)
					nRtpRtcpPacketType = 1;//2���ֽڷ�ʽ��
			}

			//��ȡrtp���İ�ͷ
			rtpHeadPtr = (_rtp_header*)(netDataCache + nNetStart);

			nRtpLength = ntohs(nRtpLength);
			if (nRtpLength > 65535)
			{
				WriteLog(Log_Debug, "CNetGB28181RtpServer = %X rtp��ͷ��������  nClient = %llu ,nRtpLength = %llu", this, nClient, nRtpLength);
				DeleteNetRevcBaseClient(nClient);
				return -1;
			}

			//���ȺϷ� ������rtp�� ��rtpHeadPtr->v == 2) ,��ֹrtcp����ִ��rtp���
			if (nRtpLength > 0 && rtpHeadPtr->v == 2)
			{
				//����rtpͷ�����payload���н��,��Ч��ֹ�û���д��
				if (hRtpHandle == 0)
					m_gbPayload = rtpHeadPtr->payload;

				RtpDepacket(netDataCache + nNetStart, nRtpLength);
			}

			nNetStart += nRtpLength;
			netDataCacheLength -= nRtpLength;
		}
	}

 	return 0;
}

//���͵�һ������
int CNetGB28181RtpClient::SendFirstRequst()
{//�� gb28181 Ϊtcpʱ�������ú��� 

	if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect)
	{//�ظ�http�������ӳɹ���
		sprintf(szResponseBody, "{\"code\":0,\"port\":%d,\"memo\":\"success\",\"key\":%llu}", nReturnPort, nClient);
		ResponseHttp(nClient_http, szResponseBody, false);
	}

	auto pMediaSource = GetMediaStreamSource(m_szShareMediaURL);
	if (pMediaSource != NULL)
	{
		memcpy((char*)&mediaCodecInfo, (char*)&pMediaSource->m_mediaCodecInfo, sizeof(MediaCodecInfo));
		pMediaSource->AddClientToMap(nClient);
	}


	return 0;
}

//rtp����ص�
void RTP_DEPACKET_CALL_METHOD NetGB28181RtpClient_rtppacket_callback_recv(_rtp_depacket_cb* cb)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)cb->userdata;
	if (!pThis->bRunFlag)
		return;

	if (pThis->m_startSendRtpStruct.RtpPayloadDataType[0] == 0x31)
	{//����PS���
		if (pThis->psBeiJingLaoChenDemuxer)
			ps_demuxer_input(pThis->psBeiJingLaoChenDemuxer, cb->data, cb->datasize);
	}
	else if (pThis->m_startSendRtpStruct.RtpPayloadDataType[0] == 0x32)
	{//RTP ���
		if (pThis->pRecvMediaSource == NULL)
		{//rtp��� 
			pThis->pRecvMediaSource = CreateMediaStreamSource(pThis->m_recvMediaSource, pThis->nClient, MediaSourceType_LiveMedia, 0, pThis->m_h265ConvertH264Struct);
			if (pThis->pRecvMediaSource != NULL)
			{
				pThis->pRecvMediaSource->netBaseNetType = pThis->netBaseNetType;
				WriteLog(Log_Debug, "NetGB28181RtpClient_rtppacket_callback_recv ����ý��Դ %s �ɹ�  ", pThis->m_recvMediaSource);
			}
		}

		if (pThis->pRecvMediaSource != NULL)
		{
			if (cb->payload == 98)
				pThis->pRecvMediaSource->PushVideo(cb->data, cb->datasize, "H264");
			else if (cb->payload == 99)
				pThis->pRecvMediaSource->PushVideo(cb->data, cb->datasize, "H265");
			else if (cb->payload == 0)//g711u 
				pThis->pRecvMediaSource->PushAudio(cb->data, cb->datasize, "G711_U", 1, 8000);
			else if (cb->payload == 8)//g711a
				pThis->pRecvMediaSource->PushAudio(cb->data, cb->datasize, "G711_A", 1, 8000);
			else if (cb->payload == 97)//aac
			{
				//��ȡAACý����Ϣ
				if(pThis->nRecvSampleRate == 0 && pThis->nRecvChannels == 0)
				   pThis->GetAACAudioInfo2(cb->data, cb->datasize, &pThis->nRecvSampleRate , &pThis->nRecvChannels);
				if (cb->datasize > 0 && cb->datasize < 2048)
					pThis->pRecvMediaSource->PushAudio((unsigned char*)cb->data, cb->datasize, "AAC", pThis->nRecvChannels, pThis->nRecvSampleRate);
			}
		}
	}
}

static void mpeg_ps_dec_NetGB28181RtpClient(void* param, int stream, int codecid, const void* extra, int bytes, int finish)
{
	printf("stream %d, codecid: %d, finish: %s\n", stream, codecid, finish ? "true" : "false");
}
//rtp ���
struct ps_demuxer_notify_t notify_CNetGB28181RtpClient = { mpeg_ps_dec_NetGB28181RtpClient, };

//����PS����ص�
static int NetGB28181RtpClient_on_gb28181_unpacket(void* param, int stream, int avtype, int flags, int64_t pts, int64_t dts, const void* data, size_t bytes)
{
	CNetGB28181RtpClient* pThis = (CNetGB28181RtpClient*)param;
	if (!pThis->bRunFlag)
		return -1;

	if (pThis->pRecvMediaSource == NULL)
	{//���ȴ���ý��Դ 
		pThis->pRecvMediaSource = CreateMediaStreamSource(pThis->m_recvMediaSource, pThis->nClient, MediaSourceType_LiveMedia, 0, pThis->m_h265ConvertH264Struct);
		if (pThis->pRecvMediaSource != NULL)
		{
 			pThis->pRecvMediaSource->netBaseNetType = pThis->netBaseNetType;
			WriteLog(Log_Debug, "NetGB28181RtpClient_on_gb28181_unpacket ����ý��Դ %s �ɹ�  ", pThis->m_recvMediaSource);
		}
	}

	if (pThis->pRecvMediaSource == NULL)
		return -1;

	if (!pThis->pRecvMediaSource->bUpdateVideoSpeed)
	{//��Ҫ����ý��Դ��֡�ٶ�
		pThis->pRecvMediaSource->UpdateVideoFrameSpeed(25, pThis->netBaseNetType);
		pThis->pRecvMediaSource->bUpdateVideoSpeed = true;
	}

	if (pThis->m_startSendRtpStruct.recv_disableAudio[0] == 0x30  && (PSI_STREAM_AAC == avtype || PSI_STREAM_AUDIO_G711A == avtype || PSI_STREAM_AUDIO_G711U == avtype))
	{
		if (PSI_STREAM_AAC == avtype)
		{//aac
		    //��ȡAACý����Ϣ
			if (pThis->nRecvSampleRate == 0 && pThis->nRecvChannels == 0)
  				pThis->GetAACAudioInfo2((unsigned char*)data, bytes, &pThis->nRecvSampleRate, &pThis->nRecvChannels);

 			pThis->pRecvMediaSource->PushAudio((unsigned char*)data, bytes, "AAC", pThis->nRecvChannels, pThis->nRecvSampleRate);
		}
		else if (PSI_STREAM_AUDIO_G711A == avtype)
		{// G711A  
			pThis->pRecvMediaSource->PushAudio((unsigned char*)data, bytes, "G711_A", 1, 8000);
		}
		else if (PSI_STREAM_AUDIO_G711U == avtype)
		{// G711U  
			pThis->pRecvMediaSource->PushAudio((unsigned char*)data, bytes, "G711_U", 1, 8000);
		}
	}
	else if (pThis->m_startSendRtpStruct.recv_disableVideo[0] == 0x30 && (PSI_STREAM_H264 == avtype || PSI_STREAM_H265 == avtype || PSI_STREAM_VIDEO_SVAC == avtype))
	{
#ifdef WriteRecvPSDataFlag
		if (pThis->fWritePSDataFile != NULL )
		{
			fwrite(data, 1, bytes, pThis->fWritePSDataFile);
			fflush(pThis->fWritePSDataFile);
		}
#endif	
 		if (PSI_STREAM_H264 == avtype)
			pThis->pRecvMediaSource->PushVideo((unsigned char*)data, bytes, "H264");
		else if (PSI_STREAM_H265 == avtype)
			pThis->pRecvMediaSource->PushVideo((unsigned char*)data, bytes, "H265");
 	}
}

//rtp��� 
bool  CNetGB28181RtpClient::RtpDepacket(unsigned char* pData, int nDataLength)
{
	if (pData == NULL || nDataLength > 65536 || !bRunFlag || nDataLength < 12)
		return false;

	//����rtp���
	if (hRtpHandle == 0)
	{
		rtp_depacket_start(NetGB28181RtpClient_rtppacket_callback_recv, (void*)this, (uint32_t*)&hRtpHandle);
 
		if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x31)
		{//rtp + PS
			rtp_depacket_setpayload(hRtpHandle, 96, e_rtpdepkt_st_gbps);
  		}
		else if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x32 && rtpHeadPtr != NULL)
		{//rtp + ES
			if (rtpHeadPtr->payload == 98)
				rtp_depacket_setpayload(hRtpHandle, rtpHeadPtr->payload, e_rtpdepkt_st_h264);
			else if (rtpHeadPtr->payload == 99)
				rtp_depacket_setpayload(hRtpHandle, rtpHeadPtr->payload, e_rtpdepkt_st_h265);
		}
		else if (m_startSendRtpStruct.RtpPayloadDataType[0] == 0x33)
		{//rtp + xhb
			rtp_depacket_setpayload(hRtpHandle, m_gbPayload, e_rtpdepkt_st_xhb);
		}
		strcpy(m_addStreamProxyStruct.app, m_startSendRtpStruct.recv_app);
		strcpy(m_addStreamProxyStruct.stream, m_startSendRtpStruct.recv_stream);
		sprintf(m_recvMediaSource, "/%s/%s", m_startSendRtpStruct.recv_app, m_startSendRtpStruct.recv_stream);
		WriteLog(Log_Debug, "CNetGB28181RtpClient = %X ,����rtp����ɹ� nClient = %llu ,hRtpHandle = %d", this, nClient, hRtpHandle);
	}

	if (psBeiJingLaoChenDemuxer == NULL)
	{
		psBeiJingLaoChenDemuxer = ps_demuxer_create(NetGB28181RtpClient_on_gb28181_unpacket, this);
		if (psBeiJingLaoChenDemuxer != NULL)
		{
			ps_demuxer_set_notify(psBeiJingLaoChenDemuxer, &notify_CNetGB28181RtpClient, this);
			WriteLog(Log_Debug, "CNetGB28181RtpClient = %X ,��������PS����ɹ� nClent = %llu ,psBeiJingLaoChenDemuxer = %X", this, nClient, psBeiJingLaoChenDemuxer);
		}
	}

	if (hRtpHandle > 0)
	{//rtp���
		rtp_depacket_input(hRtpHandle, pData, nDataLength);
	}

	return true;
}

//����m3u8�ļ�
bool  CNetGB28181RtpClient::RequestM3u8File()
{
	return true;
}

//����1078��Ƶ���� 
void  CNetGB28181RtpClient::SendJtt1078VideoPacket()
{
	unsigned char* pData = NULL;
	int            nLength = 0;

	if(gbDstAddr.sin_port == 0)
	{ 
	  gbDstAddr.sin_family = AF_INET;
	  gbDstAddr.sin_addr.s_addr = inet_addr(m_startSendRtpStruct.dst_url);
	  gbDstAddr.sin_port = htons(atoi(m_startSendRtpStruct.dst_port));
	}
 
	if ((pData = m_videoFifo.pop(&nLength)) != NULL)
	{
		if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			jt1078VideoHead.pt = 98;
		else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			jt1078VideoHead.pt = 99;
		else
		{
			m_videoFifo.pop_front();
			return ;
		}

		//ȷ��֡���� 
		if (CheckVideoIsIFrame(mediaCodecInfo.szVideoName, pData, nLength))
		{
			jt1078VideoHead.frame_type = 0x00;
			nPFrameCount = 1;
		}
		else
		{
			jt1078VideoHead.frame_type = 0x01;
			nPFrameCount ++;
		}

		//�����ܰ����� 
		pPacketCount = nLength / JTT1078_MaxPacketLength;
		if (nLength % JTT1078_MaxPacketLength != 0)
			pPacketCount += 1;

		jt1078SendPacketLenth = 0;
		nPacketOrder = 0;
		nSrcVideoPos = 0;
		while (nLength > 0)
		{
			nPacketOrder ++;

			//�ܳ���С�ڵ���950 
			if (pPacketCount == 1)
				jt1078VideoHead.packet_type = 0;
			else
			{//��� 
				if (nPacketOrder == 1)
					jt1078VideoHead.packet_type = 1;//��һ��
				else if (nPacketOrder > 1 && nPacketOrder < pPacketCount)
					jt1078VideoHead.packet_type = 3;//�м��
				else
					jt1078VideoHead.packet_type = 2;//����
			}

			//����ʱ���� 
			jt1078VideoHead.frame_interval = htons(1000 / mediaCodecInfo.nVideoFrameRate);
			jt1078VideoHead.i_frame_interval = htons(nPFrameCount * (1000 / mediaCodecInfo.nVideoFrameRate));
			//WriteLog(Log_Debug, "i_frame_interval = %d , frame_interval = %d", ntohs(jt1078VideoHead.i_frame_interval), ntohs(jt1078VideoHead.frame_interval));

			if (nLength >= JTT1078_MaxPacketLength)
			{
				jt1078VideoHead.payload_size = htons(JTT1078_MaxPacketLength);

				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, (char*)&jt1078VideoHead, sizeof(jt1078VideoHead));
				jt1078SendPacketLenth += sizeof(jt1078VideoHead);
				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, pData + nSrcVideoPos, JTT1078_MaxPacketLength);
#ifdef WriteJtt1078SrcVideoFlag
				fwrite(pData + nSrcVideoPos, 1, JTT1078_MaxPacketLength, fWrite1078SrcFile);
				fflush(fWrite1078SrcFile);
#endif
				jt1078SendPacketLenth += JTT1078_MaxPacketLength;
				nSrcVideoPos += JTT1078_MaxPacketLength;
 
				nLength -= JTT1078_MaxPacketLength;
			}
			else
			{
				jt1078VideoHead.payload_size = htons(nLength);

				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, (char*)&jt1078VideoHead, sizeof(jt1078VideoHead));
				jt1078SendPacketLenth += sizeof(jt1078VideoHead);
				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, pData + nSrcVideoPos, nLength);
#ifdef WriteJtt1078SrcVideoFlag
				fwrite(pData + nSrcVideoPos, 1, nLength, fWrite1078SrcFile);
				fflush(fWrite1078SrcFile);
#endif
				jt1078SendPacketLenth += nLength;
				nSrcVideoPos += nLength;
 
				nLength = 0;
			}

			if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
			{//udp 
				XHNetSDK_Sendto(nClient, szSendRtpVideoMediaBuffer, jt1078SendPacketLenth, (void*)&gbDstAddr);
				jt1078SendPacketLenth = 0;
			}
			else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect || netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
			{//tcp 
				if ((jt1078SendPacketLenth > (MaxGB28181RtpSendVideoMediaBufferLength - 4096)) || nLength == 0 )
				{
					XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, jt1078SendPacketLenth, 1);

					jt1078SendPacketLenth = 0;
				}
			}

			jt1078VideoHead.seq++;
			if (jt1078VideoHead.seq >= 0xFFFF)
				jt1078VideoHead.seq = 0;
 		}

		m_videoFifo.pop_front();
	}
}
//����1078��Ƶ���� 
void  CNetGB28181RtpClient::SendJtt1078VideoPacket2019()
{
	unsigned char* pData = NULL;
	int            nLength = 0;

	if (gbDstAddr.sin_port == 0)
	{
		gbDstAddr.sin_family = AF_INET;
		gbDstAddr.sin_addr.s_addr = inet_addr(m_startSendRtpStruct.dst_url);
		gbDstAddr.sin_port = htons(atoi(m_startSendRtpStruct.dst_port));
	}

	if ((pData = m_videoFifo.pop(&nLength)) != NULL)
	{
		if (strcmp(mediaCodecInfo.szVideoName, "H264") == 0)
			jt1078VideoHead2019.pt = 98;
		else if (strcmp(mediaCodecInfo.szVideoName, "H265") == 0)
			jt1078VideoHead2019.pt = 99;
		else
		{
			m_videoFifo.pop_front();
			return;
		}

		//ȷ��֡���� 
		if (CheckVideoIsIFrame(mediaCodecInfo.szVideoName, pData, nLength))
		{
			jt1078VideoHead2019.frame_type = 0x00;
			nPFrameCount = 1;
		}
		else
		{
			jt1078VideoHead2019.frame_type = 0x01;
			nPFrameCount++;
		}

		//�����ܰ����� 
		pPacketCount = nLength / JTT1078_MaxPacketLength;
		if (nLength % JTT1078_MaxPacketLength != 0)
			pPacketCount += 1;

		jt1078SendPacketLenth = 0;
		nPacketOrder = 0;
		nSrcVideoPos = 0;
		while (nLength > 0)
		{
			nPacketOrder++;

			//�ܳ���С�ڵ���950 
			if (pPacketCount == 1)
				jt1078VideoHead2019.packet_type = 0;
			else
			{//��� 
				if (nPacketOrder == 1)
					jt1078VideoHead2019.packet_type = 1;//��һ��
				else if (nPacketOrder > 1 && nPacketOrder < pPacketCount)
					jt1078VideoHead2019.packet_type = 3;//�м��
				else
					jt1078VideoHead2019.packet_type = 2;//����
			}

			//����ʱ���� 
			jt1078VideoHead2019.frame_interval = htons(1000 / mediaCodecInfo.nVideoFrameRate);
			jt1078VideoHead2019.i_frame_interval = htons(nPFrameCount * (1000 / mediaCodecInfo.nVideoFrameRate));
			//WriteLog(Log_Debug, "i_frame_interval = %d , frame_interval = %d", ntohs(jt1078VideoHead2019.i_frame_interval), ntohs(jt1078VideoHead2019.frame_interval));

			if (nLength >= JTT1078_MaxPacketLength)
			{
				jt1078VideoHead2019.payload_size = htons(JTT1078_MaxPacketLength);

				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, (char*)&jt1078VideoHead2019, sizeof(jt1078VideoHead2019));
				jt1078SendPacketLenth += sizeof(jt1078VideoHead2019);
				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, pData + nSrcVideoPos, JTT1078_MaxPacketLength);
#ifdef WriteJtt1078SrcVideoFlag
				fwrite(pData + nSrcVideoPos, 1, JTT1078_MaxPacketLength, fWrite1078SrcFile);
				fflush(fWrite1078SrcFile);
#endif
				jt1078SendPacketLenth += JTT1078_MaxPacketLength;
				nSrcVideoPos += JTT1078_MaxPacketLength;

				nLength -= JTT1078_MaxPacketLength;
			}
			else
			{
				jt1078VideoHead2019.payload_size = htons(nLength);

				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, (char*)&jt1078VideoHead2019, sizeof(jt1078VideoHead2019));
				jt1078SendPacketLenth += sizeof(jt1078VideoHead2019);
				memcpy(szSendRtpVideoMediaBuffer + jt1078SendPacketLenth, pData + nSrcVideoPos, nLength);
#ifdef WriteJtt1078SrcVideoFlag
				fwrite(pData + nSrcVideoPos, 1, nLength, fWrite1078SrcFile);
				fflush(fWrite1078SrcFile);
#endif
				jt1078SendPacketLenth += nLength;
				nSrcVideoPos += nLength;

				nLength = 0;
			}

			if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
			{//udp 
				XHNetSDK_Sendto(nClient, szSendRtpVideoMediaBuffer, jt1078SendPacketLenth, (void*)&gbDstAddr);
				jt1078SendPacketLenth = 0;
			}
			else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect || netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
			{//tcp 
				if ((jt1078SendPacketLenth > (MaxGB28181RtpSendVideoMediaBufferLength - 4096)) || nLength == 0)
				{
					XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, jt1078SendPacketLenth, 1);

					jt1078SendPacketLenth = 0;
				}
			}

			jt1078VideoHead2019.seq++;
			if (jt1078VideoHead2019.seq >= 0xFFFF)
				jt1078VideoHead2019.seq = 0;
		}

		m_videoFifo.pop_front();
	}
}

//����1078��Ƶ���� 
void  CNetGB28181RtpClient::SendJtt1078AduioPacket()
{
	unsigned char* pData = NULL;
	int            nLength = 0;

	if (gbDstAddr.sin_port == 0)
	{
		gbDstAddr.sin_family = AF_INET;
		gbDstAddr.sin_addr.s_addr = inet_addr(m_startSendRtpStruct.dst_url);
		gbDstAddr.sin_port = htons(atoi(m_startSendRtpStruct.dst_port));
	}

	if ((pData = m_audioFifo.pop(&nLength)) != NULL)
	{
		if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
			jt1078AudioHead.pt = 6;
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
			jt1078AudioHead.pt = 7;
		else if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
			jt1078AudioHead.pt = 19;
		else
		{
			m_audioFifo.pop_front();
			return;
		}

		jt1078AudioHead.frame_type = 0x03;
		jt1078AudioHead.packet_type = 0x00;
  
		jt1078AudioHead.payload_size = htons(nLength);

		memcpy(szSendRtpVideoMediaBuffer, (char*)&jt1078AudioHead, sizeof(jt1078AudioHead));
 		memcpy(szSendRtpVideoMediaBuffer + sizeof(jt1078AudioHead), pData, nLength);
    
		if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
		{//udp 
			XHNetSDK_Sendto(nClient, szSendRtpVideoMediaBuffer, nLength + sizeof(jt1078AudioHead), (void*)&gbDstAddr);
 		}
		else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect || netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
		{//tcp 
 			XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nLength + sizeof(jt1078AudioHead), 1);
  		}

		jt1078AudioHead.seq++;
		if (jt1078AudioHead.seq >= 0xFFFF)
			jt1078AudioHead.seq = 0;
 
		m_audioFifo.pop_front();
	}
}
//����1078��Ƶ���� 
void  CNetGB28181RtpClient::SendJtt1078AduioPacket2019()
{
	unsigned char* pData = NULL;
	int            nLength = 0;

	if (gbDstAddr.sin_port == 0)
	{
		gbDstAddr.sin_family = AF_INET;
		gbDstAddr.sin_addr.s_addr = inet_addr(m_startSendRtpStruct.dst_url);
		gbDstAddr.sin_port = htons(atoi(m_startSendRtpStruct.dst_port));
	}

	if ((pData = m_audioFifo.pop(&nLength)) != NULL)
	{
		if (strcmp(mediaCodecInfo.szAudioName, "G711_A") == 0)
			jt1078AudioHead2019.pt = 6;
		else if (strcmp(mediaCodecInfo.szAudioName, "G711_U") == 0)
			jt1078AudioHead2019.pt = 7;
		else if (strcmp(mediaCodecInfo.szAudioName, "AAC") == 0)
			jt1078AudioHead2019.pt = 19;
		else
		{
			m_audioFifo.pop_front();
			return;
		}

		jt1078AudioHead2019.frame_type = 0x03;
		jt1078AudioHead2019.packet_type = 0x00;

		jt1078AudioHead2019.payload_size = htons(nLength);

		memcpy(szSendRtpVideoMediaBuffer, (char*)&jt1078AudioHead2019, sizeof(jt1078AudioHead2019));
		memcpy(szSendRtpVideoMediaBuffer + sizeof(jt1078AudioHead2019), pData, nLength);

		if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpUDP)
		{//udp 
			XHNetSDK_Sendto(nClient, szSendRtpVideoMediaBuffer, nLength + sizeof(jt1078AudioHead2019), (void*)&gbDstAddr);
		}
		else if (netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Connect || netBaseNetType == NetBaseNetType_NetGB28181SendRtpTCP_Passive)
		{//tcp 
			XHNetSDK_Write(nClient, szSendRtpVideoMediaBuffer, nLength + sizeof(jt1078AudioHead2019), 1);
		}

		jt1078AudioHead2019.seq++;
		if (jt1078AudioHead2019.seq >= 0xFFFF)
			jt1078AudioHead2019.seq = 0;

		m_audioFifo.pop_front();
	}
}
