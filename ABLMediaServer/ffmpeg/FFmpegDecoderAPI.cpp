
/*

 * CarEyeDecoderAPI.cpp
 *
 * Author:
 * Date: 2018-05-16 22:52
 * Copyright 2018
 *
 * ����FFMPEG������Ƶ����ӿ�ʵ��
 */

#include <stdio.h>
#include "FFmpegDecoderAPI.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
};

#ifndef _WIN32
#include "ffmpegTypes.h"
#endif

#include "AudioDecoder.h"


FFmpegAudioDecoderAPI* FFmpegAudioDecoderAPI::CreateDecoder(std::map<std::string, std::string> opts)
{
	return new AudioDecoder(opts);
}
