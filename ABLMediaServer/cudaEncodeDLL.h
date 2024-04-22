// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CUDAENCODEDLL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CUDAENCODEDLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#pragma once
#ifdef _WIN32
#define WINAPI      __stdcall
#else
#define WINAPI      
#endif


//��Ƶ��ʽ 
enum cudaEncodeVideo_enum {
	cudaEncodeVideo_MPEG1 = 0,                                         /**<  MPEG1             */
	cudaEncodeVideo_MPEG2,                                           /**<  MPEG2             */
	cudaEncodeVideo_MPEG4,                                           /**<  MPEG4             */
	cudaEncodeVideo_VC1,                                             /**<  VC1               */
	cudaEncodeVideo_H264,                                            /**<  H264              */
	cudaEncodeVideo_JPEG,                                            /**<  JPEG              */
	cudaEncodeVideo_H264_SVC,                                        /**<  H264-SVC          */
	cudaEncodeVideo_H264_MVC,                                        /**<  H264-MVC          */
	cudaEncodeVideo_HEVC,                                            /**<  HEVC              */
	cudaEncodeVideo_VP8,                                             /**<  VP8               */
	cudaEncodeVideo_VP9,                                             /**<  VP9               */
	cudaEncodeVideo_NumCodecs,                                       /**<  Max codecs        */
	// Uncompressed YUV
	cudaEncodeVideo_YUV420 = (('I' << 24) | ('Y' << 16) | ('U' << 8) | ('V')),   /**< Y,U,V (4:2:0)      */
	cudaEncodeVideo_YV12 = (('Y' << 24) | ('V' << 16) | ('1' << 8) | ('2')),   /**< Y,V,U (4:2:0)      */
	cudaEncodeVideo_NV12 = (('N' << 24) | ('V' << 16) | ('1' << 8) | ('2')),   /**< Y,UV  (4:2:0)      */
	cudaEncodeVideo_YUYV = (('Y' << 24) | ('U' << 16) | ('Y' << 8) | ('V')),   /**< YUYV/YUY2 (4:2:2)  */
	cudaEncodeVideo_UYVY = (('U' << 24) | ('Y' << 16) | ('V' << 8) | ('Y'))    /**< UYVY (4:2:2)       */
} ;

/*
���ܣ�
��ʼ��
������
��
*/
typedef bool (WINAPI* ABL_cudaEncode_Init) ();

/*
���ܣ�
��ȡӢΰ���Կ�����
����
��
����ֵ��
0   û��Ӣΰ���Կ�
N   ��N��Ӣΰ���Կ�
*/
typedef int (WINAPI* ABL_cudaEncode_GetDeviceGetCount) ();

/*
����:
��ȡӢΰ���Կ�����
������
int    nOrder   �Կ����
char*  szName   �Կ�����
����ֵ
true            ��ȡ�ɹ�
false           ��ȡʧ��
*/
typedef bool (WINAPI* ABL_cudaEncode_GetDeviceName) (int nOrder, char* szName);
 
//���������� 
/*
���ܣ����������� 
������
 cudaEncodeVideo_enum videoCodec,  ��Ҫ��ʲô������ȥ����YUV���ݣ�����֧��H264 ��cudaEncodeVideo_H264����H265 ��cudaEncodeVideo_H265�� 
 cudaEncodeVideo_enum yuvType,     ����YUV����������   cudaEncodeVideo_YUV420 �� cudaEncodeVideo_YV12 ��cudaEncodeVideo_NV12 
 int nWidth,                       YUV ���ݵĿ� 
 int nHeight,                      YUV ���ݵĸ�
 uint64_t& nCudaChan               �����������ɹ��� ���صľ�� 
*/
typedef bool (WINAPI* ABL_cudaEncode_CreateVideoEncode) (cudaEncodeVideo_enum videoCodec, cudaEncodeVideo_enum yuvType, int nWidth, int nHeight, uint64_t& nCudaChan);

//ɾ��������
typedef bool (WINAPI* ABL_cudaEncode_DeleteVideoEncode) (uint64_t nCudaChan);

typedef int  (WINAPI* ABL_cudaEncode_CudaVideoEncode) (uint64_t nCudaChan, unsigned char* pYUVData, int nYUVLength, char* pOutEncodeData);

typedef bool(WINAPI* ABL_cudaEncode_UnInit) ();

