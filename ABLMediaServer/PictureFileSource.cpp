/*
���ܣ�
    1��װ���ϵ���ʷͼƬ�ļ�����
    2����ͼƬ�ļ����ֽ�����������
	3��������ͼƬ�ļ��ļ���׷�ӵ�list��β��
	4��ɾ�����ڵ�ͼƬ�ļ�
	5������ app\ stream ��ʱ��� ���ҳ���������������ͼƬ�ļ����� 
	6������ app\ stream \ һ��ͼƬ���� ���жϸ��ļ��Ƿ����  
	 
����    2022-03-18
����    �޼��ֵ� 
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "PictureFileSource.h"

extern MediaServerPort                       ABL_MediaServerPort;

CPictureFileSource::CPictureFileSource(char* app, char* stream)
{
	memset(m_app,0x00,sizeof(m_app));
	memset(m_stream, 0x00, sizeof(m_stream));
	memset(m_szShareURL, 0x00, sizeof(m_szShareURL));
 
	strcpy(m_app, app);
	strcpy(m_stream, stream);
	sprintf(m_szShareURL, "/%s/%s", app, stream);
}

CPictureFileSource::~CPictureFileSource()
{
	malloc_trim(0);
}

bool CPictureFileSource::AddPictureFile(char* szFileName)
{
	std::lock_guard<std::mutex> lock(PictureFileLock);

	memset(szBuffer, 0x00, sizeof(szBuffer));
	memcpy(szBuffer, szFileName, strlen(szFileName) - 4);

	fileList.push_back(atoll(szBuffer));
 
	return true;
}

void CPictureFileSource::Sort()
{
	std::lock_guard<std::mutex> lock(PictureFileLock);
 	fileList.sort();
}

//�޸Ĺ���¼���ļ�
bool  CPictureFileSource::UpdateExpirePictureFile(char* szNewFileName)
{
	std::lock_guard<std::mutex> lock(PictureFileLock);
	uint64_t nGetFile;
	uint64_t nSecond = 0; 
	char    szDateTime[128] = { 0 };
	bool    bUpdateFlag = false;

	if (fileList.size() <= 0 )
	{
		WriteLog(Log_Debug, "UpdateExpirePictureFile %s ��δ��ͼƬ�ļ� ,������Ϊ %s ", m_szShareURL, szNewFileName);
		return true  ; 
	}

	while (fileList.size() > ABL_MediaServerPort.pictureMaxCount - 1)
	{
		nGetFile = fileList.front();
		sprintf(szDateTime, "%llu", nGetFile);
		 
		if (true)
		{
			fileList.pop_front();
#ifdef OS_System_Windows
			sprintf(szDeleteFile, "%s%s\\%s\\%s.jpg", ABL_MediaServerPort.picturePath, m_app, m_stream, szDateTime);
#else 
			sprintf(szDeleteFile, "%s%s/%s/%s.jpg", ABL_MediaServerPort.picturePath, m_app, m_stream, szDateTime);
#endif
			//����޸�ʧ�ܣ������Ժ��ٴ��޸�
			if (rename(szDeleteFile,szNewFileName) != 0 )
			{
				fileList.push_back(nGetFile); 
				WriteLog(Log_Debug, "UpdateExpirePictureFile %s �޸��ļ� %llu.jpg ʧ�ܣ������Ժ����޸� ", m_szShareURL, nGetFile);
				break;
 			}
			else
			{
			    bUpdateFlag = true;
			    break;
			}
   		}
		else
			break;
	}
   
	return bUpdateFlag ;
}

//��ѯ¼���ļ��Ƿ���� 
bool  CPictureFileSource::queryPictureFile(char* szPictureFileName)
{
	std::lock_guard<std::mutex> lock(PictureFileLock);

	bool bRet = false;
	//�ļ����ֳ�������
	if (strlen(szPictureFileName) != 16) 
		return false;

	//ȥ����չ�� .jpg , .bmp , .png 
	if (strstr(szPictureFileName, ".jpg") != NULL || strstr(szPictureFileName, ".bmp") != NULL)
		szPictureFileName[strlen(szPictureFileName) - 4] = 0x00;
	if (strstr(szPictureFileName, ".png") != NULL )
		szPictureFileName[strlen(szPictureFileName) - 5] = 0x00;

#ifdef USE_BOOST
	//�ж��Ƿ�Ϊ����
	if (!boost::all(szPictureFileName, boost::is_digit()))
		return false;
#else
	//�ж��Ƿ�Ϊ����
	if (!ABL::is_digits(szPictureFileName))
		return false;
#endif

 
	list<uint64_t>::iterator it2;
	for (it2 = fileList.begin(); it2 != fileList.end(); it2++)
	{
		if (*it2 == atoll(szPictureFileName))
		{
			bRet = true;
			break;
		}
	}
	return bRet;
}
