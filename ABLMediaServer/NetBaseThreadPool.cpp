/*
���ܣ�
    ���е��̳߳ع��ܣ����Զ���߳�ȥִ��һ��ʵ������ͬ������
    ����̳߳أ����������ݴﵽʱ�����߳�ִ�� 	
����    2021-03-29
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "NetBaseThreadPool.h"
#ifdef USE_BOOST
extern boost::shared_ptr<CNetRevcBase> GetNetRevcBaseClient(NETHANDLE CltHandle);
#else
extern std::shared_ptr<CNetRevcBase> GetNetRevcBaseClient(NETHANDLE CltHandle);
#endif


CNetBaseThreadPool::CNetBaseThreadPool(int nThreadCount)
{
	nThreadProcessCount = 0;
 	nTrueNetThreadPoolCount = nThreadCount;
	if (nThreadCount > MaxNetHandleQueueCount)
		nTrueNetThreadPoolCount = MaxNetHandleQueueCount;

	if (nThreadCount <= 0)
		nTrueNetThreadPoolCount = 64 ;

	unsigned long dwThread;
	bRunFlag = true;
	for (int i = 0; i < nTrueNetThreadPoolCount; i++)
	{
		nGetCurrentThreadOrder = i;
		bCreateThreadFlag = false;
#ifdef OS_System_Windows
		hProcessHandle[i] = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OnProcessThread, (LPVOID)this, 0, &dwThread);
#else
		pthread_create(&hProcessHandle[i], NULL, OnProcessThread, (void*)this);
#endif
		while (bCreateThreadFlag == false)
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			//Sleep(5);
	}
	WriteLog(Log_Debug, "CNetBaseThreadPool ���� = %X, ", this);
}

CNetBaseThreadPool::~CNetBaseThreadPool()
{
	bRunFlag = false;
	int i;
	std::lock_guard<std::mutex> lock(threadLock);
	for (i = 0; i < nTrueNetThreadPoolCount; i++)
	{//֪ͨ�����߳�
 		cv[i].notify_one();
	}

	for ( i = 0; i < nTrueNetThreadPoolCount; i++)
	{
		while (!bExitProcessThreadFlag[i])
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
	  		//Sleep(50);
#ifdef  OS_System_Windows
	    CloseHandle(hProcessHandle[i]);
#endif 
	}
	WriteLog(Log_Debug, "CNetBaseThreadPool ���� = %X  \r\n", this );
}

void CNetBaseThreadPool::ProcessFunc()
{
	int nCurrentThreadID = nGetCurrentThreadOrder;
 	bExitProcessThreadFlag[nCurrentThreadID] = false;
	uint64_t nClientID;

	bCreateThreadFlag = true; //�����߳����
	while (bRunFlag)
	{
#ifdef USE_BOOST
		if (m_NetHandleQueue[nCurrentThreadID].pop(nClientID) && bRunFlag)
		{
			boost::shared_ptr<CNetRevcBase> pClient = GetNetRevcBaseClient(nClientID);
			if (pClient != NULL)
			{
				pClient->ProcessNetData();//����ִ��
			}
		}
		else
		{
			if (bRunFlag)
			{
				std::unique_lock<std::mutex> lck(mtx[nCurrentThreadID]);
				cv[nCurrentThreadID].wait(lck);
			}
			else
				break;
		}
#else
		if (!m_NetHandleQueue[nCurrentThreadID].empty() && bRunFlag)
		{
			nClientID = m_NetHandleQueue[nCurrentThreadID].front();
			m_NetHandleQueue[nCurrentThreadID].pop();
			auto pClient = GetNetRevcBaseClient(nClientID);
			if (pClient != NULL)
			{
				pClient->ProcessNetData();//����ִ��
			}
		}
		else
		{
			if (bRunFlag)
			{
				std::unique_lock<std::mutex> lck(mtx[nCurrentThreadID]);
				cv[nCurrentThreadID].wait(lck);
			}
			else
				break;
		}
#endif


	
 	}
	bExitProcessThreadFlag[nCurrentThreadID] = true;
}

void* CNetBaseThreadPool::OnProcessThread(void* lpVoid)
{
	int nRet = 0 ;
#ifndef OS_System_Windows
	pthread_detach(pthread_self()); //�����̺߳����̷߳��룬�����߳��˳�ʱ���Զ��ͷ����߳��ڴ�
#endif

	CNetBaseThreadPool* pThread = (CNetBaseThreadPool*)lpVoid;
	pThread->ProcessFunc();

#ifndef OS_System_Windows
	pthread_exit((void*)&nRet); //�˳��߳�
#endif
	return  0;
}

bool CNetBaseThreadPool::InsertIntoTask(uint64_t nClientID)
{
	std::lock_guard<std::mutex> lock(threadLock);

	int               nThreadThread = 0;
	ClientProcessThreadMap::iterator it;

	it = clientThreadMap.find(nClientID);
	if (it != clientThreadMap.end())
	{//�ҵ� 
		nThreadThread = (*it).second;
	}
	else
	{//��δ�����
		nThreadThread = nThreadProcessCount % nTrueNetThreadPoolCount;
 		clientThreadMap.insert(ClientProcessThreadMap::value_type(nClientID, nThreadThread));
		nThreadProcessCount  ++;
	}

	m_NetHandleQueue[nThreadThread].push(nClientID);
	cv[nThreadThread].notify_one();

	return true;
}
