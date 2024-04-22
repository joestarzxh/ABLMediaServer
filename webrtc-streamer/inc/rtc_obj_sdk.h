/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#pragma once
#define WIN32_LEAN_AND_MEAN	
 // ���Ҫ�ڴ˴�Ԥ����ı�ͷ

#include <map>
#include <list>
#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include "../capture/VideoCapture.h"
#include "../capture/AudioCapture.h"

typedef std::function<void(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight)> LocalFrameCallBackFunc;

typedef std::function<void(std::string userID, uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight)> RemoteFrameCallBackFunc;





class WEBRTCSDK_EXPORTSIMPL MainWndCallback {
public:
	MainWndCallback()
	{

	}
	virtual ~MainWndCallback() {}

	

	//����ͷyuv���ݻص�
	virtual void onLocalFrame(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight) = 0;
	
	//Զ����Ƶyuv���ݻص�
	virtual void onRemoteFrame(std::string userID, uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight) = 0;

	//������˷����ݻص�
	virtual void onLocalAudio(const void* audio_data ,int sample_rate_hz, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) = 0;

	//�Է���˷����ݻص�
	virtual void onRemoteAudio(std::string userID, const void* audio_data, int sample_rate_hz, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) = 0;

	//Զ�̷���ֹͣ������Ƶ��
	virtual  void onRemoveTrack(std::string userID) = 0;

	//websocket����Ϣ h�ص�
	virtual  void onWsMessage(std::string strMessage) = 0;

	//log��Ϣ�ص�
	virtual  void onLogMessage(std::string strLog) = 0;

	//������Ϣ�ص�
	virtual void onError(int nErrorCode, std::string msg) = 0;


	virtual void OnDataChannel(std::string userID, const char* data, uint32_t len, bool binary) = 0;
};

class PeerConnectionManager;
class HttpServerRequestHandler;

class WEBRTCSDK_EXPORTSIMPL WebRtcEndpoint {
public:
	//��ʼ��
	void init(const char* webrtcConfig, std::function<void(const char* callbackJson, void* pUserHandle)> callback);
	
	//�ͷ�
	void Uninit();
	
	//�����ر�ĳһ·����
	bool stopWebRtcPlay(const char* peerid);
	
	// �����ر�ĳһ��ý��Դ 
	bool  deleteWebRtcSource(const char* szMediaSource);

	void createIceServers(std::string username, std::string realm,
		std::string externalIp, std::string listeningIp,
		int listeningPort, int minPort, int maxPort);

	void createIceServers(const char* callbackJson);

public:
	static WebRtcEndpoint& getInstance();
private:
	WebRtcEndpoint(); 

	~WebRtcEndpoint() = default;

	WebRtcEndpoint(const WebRtcEndpoint&) = delete;

	WebRtcEndpoint& operator=(const WebRtcEndpoint&) = delete;


	PeerConnectionManager* webRtcServer;

	HttpServerRequestHandler* httpServer;

	std::function<void(const char* callbackJson, void* pUserHandle)>  m_callback;

	std::atomic<bool> bInit;

	std::thread* m_turnThread;

};

