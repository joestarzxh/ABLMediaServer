#pragma once

#include <functional>
#include <map>
#include <string>
#include <mutex>

#if (defined _WIN32 || defined _WIN64)

#ifdef   WEBRTCSDK_EXPORTS
#define WEBRTCSDK_EXPORTSIMPL __declspec(dllexport)
#else
#define WEBRTCSDK_EXPORTSIMPL __declspec(dllimport)
#endif
#else

#define WEBRTCSDK_EXPORTSIMPL __attribute__((visibility("default")))
#endif


typedef std::function<void(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp)> VideoYuvCallBack; 

typedef std::function<void(char* h264_raw, int file_size, bool bKey, int nWidth, int nHeight,int fps, int64_t nTimeStamp)> H264CallBack;


enum PlayEvenID {
	MEDIA_NONE = 0, //�޲���
	MEDIA_START = 1, //��ʼ 
	MEDIA_PLAY = 2, //������ 
	MEDIA_PAUSE = 3,//��ͣ
	MEDIA_STOP = 4, //����ֹͣ
	MEDIA_END = 5, //���Ž���
	MEDIA_ERROR = 6 , //�����쳣
	MEDIA_REMOVE = 7  //����
};




/*
	std::map<std::string, std::string> opts;
	opts["width"] = to_string(m_StreamConfig.nWidth);
	opts["height"] = to_string(m_StreamConfig.nHeight);
	opts["fps"] = to_string(m_StreamConfig.fps);
	opts["bitrate"] = to_string(m_StreamConfig.startVideoBitrate);
	opts["timeout"] = to_string(5);
if (opts.find("width") != opts.end()) {
	width = std::stoi(opts.at("width"));
}
if (opts.find("height") != opts.end()) {
	height = std::stoi(opts.at("height"));
}
if (opts.find("fps") != opts.end()) {
	fps = std::stoi(opts.at("fps"));
}

*/

class WEBRTCSDK_EXPORTSIMPL VideoCapture
{
public:
	static VideoCapture* CreateVideoCapture(std::string videourl="");

	VideoCapture()
	{
	}
	virtual  ~VideoCapture()
	{
	}
	virtual bool Start() = 0;

	virtual void Init(const char* devicename, int nWidth , int nHeight , int nFrameRate = 30) = 0;
	
	virtual void Init(const char* strJson) = 0;

	virtual void Stop(VideoYuvCallBack yuvCallback) = 0;

	virtual void Destroy()=0;

	virtual void RegisterCallback(VideoYuvCallBack yuvCallback) = 0;

	virtual void RegisterH264Callback(H264CallBack yuvCallback) = 0;

	virtual bool onData(const char* id, unsigned char* buffer, int size, int64_t ts)=0;

	virtual bool onData(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp) = 0;

};




class WEBRTCSDK_EXPORTSIMPL VideoCaptureManager
{
public:
	// ���������
	VideoCapture* AddInput(const std::string& videoUrl);

	// �Ƴ�������
	void RemoveInput(const std::string& videoUrl);

	// ��ȡ����������
	VideoCapture* GetInput(const std::string& videoUrl);


	std::string  getStream(const std::string& videoUrl);

	bool isURLWithProtocol(const std::string& str) {
		// �ж��ַ����Ƿ���Э�鿪ͷ������ "rtsp://"
		return (str.substr(0, 7) == "rtsp://" || str.substr(0, 7) == "http://" || str.substr(0, 7) == "rtmp://");
	}

	std::string extractPathFromURL(const std::string& url) {
		size_t pos = url.find("://");
		if (pos != std::string::npos) {
			// ����ַ�������Э�飬��ȡЭ����·������
			return url.substr(pos + 3);
		}
		else {
			// ���û��Э�飬ֱ�ӷ���ԭʼ�ַ���
			return url;
		}
	}

	std::string getPortionAfterPort(const std::string& str) {
		size_t startPos = str.find(':', 6); // �ӵ�6���ַ���ʼ����ð�ţ�����Э�鲿��
		if (startPos == std::string::npos) {
			return ""; // �Ҳ���ð�ţ����ؿ��ַ���
		}

		size_t endPos = str.find('/', startPos); // ��ð�ź�����ҵ�һ��б��
		if (endPos == std::string::npos) {
			return ""; // �Ҳ���б�ܣ����ؿ��ַ���
		}

		return str.substr(endPos); // ��ȡб�ܺ���Ĳ���
	}
public:
	static VideoCaptureManager& getInstance();
private:
	VideoCaptureManager() = default;
	~VideoCaptureManager() {};
	VideoCaptureManager(const VideoCaptureManager&) = delete;
	VideoCaptureManager& operator=(const VideoCaptureManager&) = delete;

private:
	std::mutex m_mutex;
	std::map<std::string, VideoCapture*> m_inputMap;
public:

	static VideoCaptureManager* s_pVideoTrackMgrGet;
};

