#pragma once
#include <functional>
#include <map>
#include <string>
#include <mutex>
#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#if (defined _WIN32 || defined _WIN64)

#ifdef   WEBRTCSDK_EXPORTS
#define WEBRTCSDK_EXPORTSIMPL __declspec(dllexport)
#else
#define WEBRTCSDK_EXPORTSIMPL __declspec(dllimport)
#endif
#else

#define WEBRTCSDK_EXPORTSIMPL __attribute__((visibility("default")))
#endif



typedef  std::function<void(uint8_t* pcm, int datalen, int nSampleRate, int nChannel, int64_t nTimeStamp)> PcmCallBack;
typedef  std::function<void(uint8_t* aac_raw, int file_size, int64_t nTimeStamp)> AacCallBack;
class  WEBRTCSDK_EXPORTSIMPL  AudioCapture
{
public:
	AudioCapture() {};

	virtual ~AudioCapture() {};

	static AudioCapture* CreateAudioCapture(std::string audiourlconst, std::map<std::string, std::string> opts = {});

	virtual int Init(const char* audiocodecname, int nSampleRate, int nChannel, int nBitsPerSample) = 0;

	virtual int Init(const std::map<std::string, std::string>& opts) =0;

	virtual int Start( ) = 0;

	virtual void Stop() = 0;

	virtual void Destroy() =0;

	virtual void RegisterPcmCallback(std::function<void(uint8_t* pcm, int datalen,int nSampleRate,int nChannel,int64_t nTimeStamp)> PcmCallback) = 0;		

	virtual void RegisterAacCallback(std::function<void(uint8_t* aac_raw, int file_size, int64_t nTimeStamp)> aacCallBack) = 0;

	virtual bool onData(const char* id, unsigned char* buffer, ssize_t size, int64_t sourcets)=0;

	virtual void HangUp()=0 ;

	virtual void WakeUp()=0 ;

};



class WEBRTCSDK_EXPORTSIMPL  AudioCaptureManager
{
public:
	// ���������
	void AddInput(const std::string& videoUrl);

	// �Ƴ�������
	void RemoveInput(const std::string& videoUrl);

	// ��ȡ����������
	AudioCapture* GetInput(const std::string& videoUrl);
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
	static AudioCaptureManager& getInstance();
private:
	AudioCaptureManager() = default;
	~AudioCaptureManager() {};
	AudioCaptureManager(const AudioCaptureManager&) = delete;
	AudioCaptureManager& operator=(const AudioCaptureManager&) = delete;

private:
	std::mutex m_mutex;
	std::map<std::string, AudioCapture*> m_inputMap;
public:
};

