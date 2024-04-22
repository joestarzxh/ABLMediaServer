/*
��Ŀ�� 
Author �޼��ֵ�
Date   2019-08-15
QQ     79941308
E-mail 79941308@qq.com
        
	   20019-08-15 1.0.2 �汾
 
*/
#pragma  once 
#ifdef OS_System_Windows 


//Log ��Ϣ����
enum  LogLevel
{
	Log_Debug = 0,   //���ڵ���
	Log_Title = 1,   //������ʾ
	Log_Error = 2    //��ʶΪ����
};

/*
 �������ܣ�
     ��ʼ��־��
 ����:
    char* szSubPath                ��·�� �����ڵ�ǰ·�������log·������ ������һ����·�� szSubPath  
    char* szBaseLogFile            ��־�ļ����֣�һ��Ҫʹ�������ĸ�ʽ  "rtspURLServer_00*.log"  �������������־�ļ�Ϊ "rtspURLServer_0000000000001.log" ��"rtspURLServer_0000000000002.log����"rtspURLServer_0000000000003.log"
	int   nMaxSaveLogFileCount     ��־�ļ���󱣴���������������������Զ��ع�����ɾ�����ϵ���־�ļ���
˵����
    ��־����·���Զ���������־dll����·����log·������·��szSubPath ���档
*/
bool  StartLogFile(char* szSubPath,char* szBaseLogFile,int nMaxSaveLogFileCount);

/*
�������ܣ�
   д��־
����:
    int   nLevel,      ��־���� 
	                   enum  LogLevel
						{
							Log_Debug = 0,   //���ڵ���
							Log_Title = 1,   //������ʾ
							Log_Error = 2    //��ʶΪ����
						};
	char* szSQL, ...   ��־���ݣ��� sprintf ��ʽһ��
*/
bool  WriteLog(LogLevel nLogLevel, char* szSQL, ...);

/*
�������ܣ�
  �ر���־
����:
  �޲���
*/
bool  StopLogFile();



#endif