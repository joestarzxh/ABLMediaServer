#include <stdio.h>
#include <string.h>
#include "AsyncBuffer.h"
#include <malloc.h>

CAsyncBuffer::CAsyncBuffer()
{
	 nStart = nEnd = 0;
	 pAsyncBuffer = NULL ;
}

CAsyncBuffer::~CAsyncBuffer()
{
	uninit();
#ifndef _WIN32
	malloc_trim(0);
#endif
}

//��ʼ��
bool CAsyncBuffer::init(int nSize)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif
	if (pAsyncBuffer)
		return false;
	while (pAsyncBuffer == NULL)
		pAsyncBuffer = new unsigned char[nSize];
	nStart = nEnd = 0; 
	nBufferSize = nSize;

	return true;
}

//�����ڴ�
bool  CAsyncBuffer::uninit()
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif
	if (pAsyncBuffer)
	{
		delete [] pAsyncBuffer;
		pAsyncBuffer = NULL;

	    nStart = nEnd = 0 ; 
	    return true;
	}
	else
		return false;
}

//���������buffer
bool CAsyncBuffer::push(unsigned char* pData, int nLength)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	if (nLength > nBufferSize || pAsyncBuffer == NULL || nLength > (nBufferSize - (nEnd - nStart)) )
		return false;

	if (nBufferSize - nEnd < nLength  )
	{//β���ռ䲻���洢����ʣ��������ǰ�ƶ� 
		memmove(pAsyncBuffer, pAsyncBuffer + nStart, nEnd - nStart);
		nStart = 0;
		nEnd = nEnd - nStart;
	}
 
	memcpy(pAsyncBuffer + nEnd, pData, nLength);
	nEnd += nLength;

	return true;
}

//ȡ��buffer���з���
unsigned char*  CAsyncBuffer::pop(int& nLength)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif
	if (nEnd - nStart <= 0)
	{
		nLength = 0;
		return NULL;
	}

	nLength = nEnd - nStart;
	return  pAsyncBuffer + nStart ;
}

//�����Ƴ��Ѿ����͵�����
bool  CAsyncBuffer::front_pop(int nLength)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	if (nLength > nBufferSize)
		return false;

	nStart += nLength;

	if (nStart == nEnd)
		nStart = nEnd = 0;

	return true;
}

//�������
bool   CAsyncBuffer::reset()
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	nStart = nEnd = 0; 

	return true;
}
