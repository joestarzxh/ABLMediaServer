/*
���ܣ� Frameת����

Author �޼��ֵ�
Date   2017-12-16
QQ     79941308
E-mail luoshizhen2003@gmail.com

*/

#include "stdafx.h"
#include "AVFrameSWS.h"

CAVFrameSWS::CAVFrameSWS(void)
{
	m_bCreeateSrcFrame = false;
	bInitFlag = false;
	szDestData = NULL;
	pFrameDest = NULL;
	szSrcData = NULL ;
	pFrameSrc = NULL ;
 	img_convert_ctx = NULL; //ת����
#ifdef WriteAVFrameSWSYUVFlag
	fWriteYUV = NULL ;
	nWriteYUVCount = 0;
#endif
}

CAVFrameSWS::~CAVFrameSWS(void)
{
	DeleteAVFrameSws();
	malloc_trim(0);
}

/*
���ܣ� ����ת����
������
    bool          bCreeateSrcFrame  �Ƿ���Ҫ����ԭʼframe 
    AVPixelFormat nSrcImage,     ԭʼ����
	int           nSrcWidth,     ԭʼ��
	int           nSrcHeight,    ԭʼ��
	AVPixelFormat nDstImage,     Ŀ������
	int           nDstWidth,     Ŀ���
	int           nDestHeight,   Ŀ���
	int           swsType        ת������ �㷨
*/
bool CAVFrameSWS::CreateAVFrameSws(bool bCreeateSrcFrame,AVPixelFormat nSrcImage, int nSrcWidth, int nSrcHeight, AVPixelFormat nDstImage, int nDstWidth, int nDestHeight, int swsType)
{
	std::lock_guard<std::mutex> lock(swsLock);
	if (bInitFlag)
	{
		return true;
	}

	m_nWidth     = nSrcWidth;
	m_nHeight    = nSrcHeight;
	m_nWidthDst  = nDstWidth;
	m_nHeightDst = nDestHeight ;
	m_bCreeateSrcFrame = bCreeateSrcFrame;

	if (m_bCreeateSrcFrame)
	{//cuda��Ҫ���ַ�ʽ 
		numBytes1 = av_image_get_buffer_size((enum AVPixelFormat)nSrcImage, m_nWidth, m_nHeight, 1);
		pFrameSrc = av_frame_alloc();
		szSrcData = new unsigned char[numBytes1];
		av_image_fill_arrays(pFrameSrc->data, pFrameSrc->linesize, szSrcData, nSrcImage, m_nWidth, m_nHeight, 1);
	}

	numBytes2 = av_image_get_buffer_size((enum AVPixelFormat)nDstImage, nDstWidth, nDestHeight,1);
	pFrameDest = av_frame_alloc();
 	szDestData = new unsigned char[numBytes2];
	av_image_fill_arrays(pFrameDest->data, pFrameDest->linesize, szDestData, nDstImage, nDstWidth, nDestHeight, 1);

	img_convert_ctx = sws_getContext(nSrcWidth, nSrcHeight,  //������ԭʼ�ģ�
		nSrcImage,                //ԭ���ظ�ʽ
		nDstWidth, nDestHeight,    //������ (Ŀ���)
		(enum AVPixelFormat)nDstImage, //Ŀ������
		swsType, NULL, NULL, NULL);//SWS_BICUBIC

	pFrameDest->width = nDstWidth;
	pFrameDest->height = nDestHeight;
	pFrameDest->format = nDstImage ;//ָ��Ϊ�����ʽ

	bInitFlag = true;
	return true;
}

bool CAVFrameSWS::AVFrameSWS(AVFrame* pPicture)
{
	std::lock_guard<std::mutex> lock(swsLock);
	bool bRet;
	int  nRet;
	if (bInitFlag && img_convert_ctx != NULL)
	{
		nRet = sws_scale(img_convert_ctx, pPicture->data, pPicture->linesize, 0, m_nHeight, pFrameDest->data, pFrameDest->linesize);
		bRet = true;
	}
	else
		bRet = false;
	return bRet;
}

bool CAVFrameSWS::AVFrameSWSYUV(unsigned char* pYUV, int nLength)
{
	std::lock_guard<std::mutex> lock(swsLock);
	if (!m_bCreeateSrcFrame || pYUV == NULL || nLength <= 0 || nLength > ((m_nWidth * m_nHeight * 3) / 2) )
		return false;

	bool bRet;
	int  nRet;
 
	if (bInitFlag && img_convert_ctx != NULL)
	{
		memcpy(szSrcData, pYUV, nLength);
		nRet = sws_scale(img_convert_ctx, pFrameSrc->data, pFrameSrc->linesize, 0, m_nHeight, pFrameDest->data, pFrameDest->linesize);
		bRet = true;
	}
	else
		bRet = false;
	return bRet;
}

bool CAVFrameSWS::DeleteAVFrameSws()
{
	std::lock_guard<std::mutex> lock(swsLock);
	bool bRet;
	if (bInitFlag)
	{
		if (pFrameDest != NULL)
		{
			av_frame_free(&pFrameDest);//avcodec_alloc_frame avcodec_free_frame
			pFrameDest = NULL;
		}
		if (img_convert_ctx != NULL)
		{
			sws_freeContext(img_convert_ctx);
			img_convert_ctx = NULL;
		}
		SAFE_ARRAY_DELETE(szDestData);

		if (m_bCreeateSrcFrame)
		{//֧��cuda
			if (pFrameSrc != NULL)
			{
				av_frame_free(&pFrameSrc); 
				pFrameSrc = NULL;
			}
			SAFE_ARRAY_DELETE(szSrcData);
		}

		bInitFlag = false;
		bRet = true;
	}
	else
		bRet = false;

#ifdef WriteAVFrameSWSYUVFlag
	if (fWriteYUV != NULL)
	{
		fclose(fWriteYUV);
		fWriteYUV = NULL;
	}
#endif
	return bRet;
}

bool CAVFrameSWS::GetStatus()
{
	std::lock_guard<std::mutex> lock(swsLock);
	return bInitFlag;
}
