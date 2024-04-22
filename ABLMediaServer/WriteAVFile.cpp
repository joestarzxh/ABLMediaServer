// WriteAVFile.cpp: implementation of the CWriteAVFile class.
//
//////////////////////////////////////////////////////////////////////
#ifdef OS_System_Windows
#include "stdafx.h"
#include "WriteAVFile.h"
#include "MyWriteLogFile.h"

extern char                           ABL_szCurrentPath[512]  ;
extern char                           ABL_szLogPath[256] ;
extern char                           ABL_BaseLogFileName[256];
extern unsigned long  GetLogFileByPathName(char* szPath, char* szLogFileName, char* szOutFileName, bool& bFileExist);

CWriteAVFile::CWriteAVFile()
{
	 bOpenFlag = false;        //���ļ��ı�־
	 hWriteHandle = NULL  ;     //�ļ����
	 szCacheAVBuffer = new char[OneWriteDiskMaxLength];
	 InitializeCriticalSection(&file_CriticalSection);
	 memset(m_szFileName, 0x00, sizeof(m_szFileName));
}

CWriteAVFile::~CWriteAVFile()
{
      CloseAVFile() ;
	  DeleteCriticalSection(&file_CriticalSection);
	  delete[] szCacheAVBuffer;
	  malloc_trim(0);
}

//�����ļ�
bool CWriteAVFile::CreateAVFile(char* szFileName, bool bFileExist, unsigned long nFileSize)
{
	unsigned long dwWrite ;

    if(bOpenFlag)
	  return false;

	if (bFileExist == false)
	{//�����ļ�
		hWriteHandle = CreateFile(szFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hWriteHandle == INVALID_HANDLE_VALUE)
			return false;
	}
	else
	{//ԭ�ļ��Ѿ�����
		hWriteHandle = CreateFile(szFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hWriteHandle == INVALID_HANDLE_VALUE)
			return false;
		::SetFilePointer(hWriteHandle, NULL, NULL, FILE_END);
	}

	//ͳ���ֽ����������ܳ���1G
	nWriteByteCount = nFileSize;

    nCacheAVLength = 0 ;
	bOpenFlag = true;
	if (strlen(m_szFileName) == 0)
	 strcpy(m_szFileName, szFileName);

	return true;
}

//д��ý������
bool  CWriteAVFile::WriteAVFile(char* szMediaData, int nLength, bool bFlastWriteFlag)
{
	unsigned long dwWrite ;
	unsigned long dwFileSize = 0;
     if(!bOpenFlag || nLength <= 0 || szMediaData == NULL)
         return false;

	 EnterCriticalSection(&file_CriticalSection);

	 if (bFlastWriteFlag)
	 {//����д��
		 WriteFile(hWriteHandle, szMediaData, nLength, &dwWrite, NULL);
		 nWriteByteCount += nLength;
	 }
	 else
	 {//�Ȼ�����д��
		 if (OneWriteDiskMaxLength - nCacheAVLength < nLength)
		 {//����ʣ��Ŀռ䲻�������֡
			 if (nCacheAVLength > 0)
				 WriteFile(hWriteHandle, szCacheAVBuffer, nCacheAVLength, &dwWrite, NULL);

			 //�ܼ��ֽ��������ܳ���1G
			 nWriteByteCount += nCacheAVLength;

			 nCacheAVLength = 0;
		 }

		 memcpy(szCacheAVBuffer + nCacheAVLength, szMediaData, nLength);
		 nCacheAVLength += nLength;
	 }
	 LeaveCriticalSection(&file_CriticalSection);

	 //�������1G�ֽڣ������йر�
	 if (nWriteByteCount >= MaxCsmFileByteCount)
	 {
		 CloseAVFile();

		 //��ȡ��־����
		 dwFileSize = GetLogFileByPathName(ABL_szLogPath, ABL_BaseLogFileName, m_szFileName, bFileExist);

		 CreateAVFile(m_szFileName, bFileExist, dwFileSize);//�����������ļ�
	 }

	 return true;
}

//�ر��ļ�
void  CWriteAVFile::CloseAVFile() 
{
	unsigned long dwWrite ;

	if(bOpenFlag)
	{
       EnterCriticalSection(&file_CriticalSection);
		bOpenFlag = false;
	
	    if(nCacheAVLength > 0)
  		  WriteFile(hWriteHandle,szCacheAVBuffer,nCacheAVLength,&dwWrite,NULL) ;
	   
		nCacheAVLength = 0;
		CloseHandle(hWriteHandle) ;
 	   LeaveCriticalSection(&file_CriticalSection);

	}
}
#endif