#pragma once

#include <unordered_set>
#include <string.h>

//�����ģ����
template<class T>
class class_obj_pool
{
public:
	enum
	{
		MAX_INIT_NUM = 100000,	// ��ʼ������������ݶ�10��
	};

	class_obj_pool()
	{
		clear();
	}

	~class_obj_pool()
	{
		clear();
	}
public:
	// ��ʼ��(��ʼ������һ�������������� ��������������)
	bool init(int initNum = 200, int increaseNum = 100, int reduceNum = 50)
	{
		// �������
		if (initNum <= 0 || increaseNum <= 0 || reduceNum <= 0)
		{
			printf("init(), param error, initNum=%d, nIncreaseNum=%d, reduceNum=%d\n", initNum, increaseNum, reduceNum);
			return false;
		}

		nInitNum = initNum;
		nIncreaseNum = increaseNum;
		nShrinkNum = reduceNum;

		// ��������
		nInitNum = nInitNum > MAX_INIT_NUM ? MAX_INIT_NUM : nInitNum;			// ��ʼ����
		nIncreaseNum = nIncreaseNum > MAX_INIT_NUM ? MAX_INIT_NUM : nIncreaseNum;	// һ������������

		// ��ʼ������
		for (int i = 0; i < nInitNum; ++i)
		{
			T* pObj = new(std::nothrow)T();
			if (pObj == NULL)
			{
				printf("new obj failed \n");
				continue;
			}

			// T�������ṩinit�������г�ʼ��
			if (pObj->init() == false)
			{
				printf("pObj->init() failed \n");
				delete pObj;
				continue;
			}

			free_list.insert(pObj);
		}

		return true;
	}

	// ����һ���¶���
	T* alloc()
	{
		T* pRet = NULL;

		// �����б����У���ȡһ��
		if (free_list.size() > 0)
		{
			auto iter = free_list.begin();
			pRet = *iter;

			free_list.erase(iter);
			used_list.insert(pRet);

			// �������ʹ�ü�¼���������
			if (used_list.size() > nUsedMax)
			{
				nUsedMax = used_list.size();
			}

			return pRet;
		}

		// �����б���û��ʱ�����½�һ��
		for (int i = 0; i < nIncreaseNum; i++)
		{
			T* pObj = new(std::nothrow)T();
			if (pObj == NULL)
			{
				printf("alloc(), new obj failed \n");
				continue;
			}

			// T�������ṩinit�������г�ʼ��
			if (pObj->init() == false)
			{
				printf("pObj->init() failed \n");
				delete pObj;
				continue;
			}

			// û��ֵ���ȸ�ֵ���Ѹ�ֵ��ѹ������б�
			if (pRet == NULL)
			{
				pRet = pObj;
				used_list.insert(pObj);
			}
			else
			{
				free_list.insert(pObj);
			}
		}

		// �������ʹ�ü�¼���������
		if (used_list.size() > nUsedMax)
		{
			nUsedMax = used_list.size();
		}

		return pRet;
	}

	// ����һ������
	void dealloc(T* pObj)
	{
		if (pObj == NULL)
		{
			printf("dealloc(), pObj == NULL");
			return;
		}

		// ������
		auto it = used_list.find(pObj);
		if (it == used_list.end())
		{
			printf("dealloc(), find pObj failed \n");
			return;
		}

		// T�������ṩreset()������������
		pObj->reset();

		free_list.insert(pObj);
		used_list.erase(pObj);
	}

	// ִ��һ�λ��գ�����ʹ�ø߷���������
	void dorecycle()
	{
		// �����б����ж����ҳ���һ������ʱ���Ž��л���
		int nfree = free_list.size();
		if (nfree <= nIncreaseNum)
		{
			return;
		}

		// Ҫ���յ�����
		nfree = (nfree - nIncreaseNum);
		int i = 0;
		for (auto iter : free_list )
		{
			T* pObj = iter;
			if (pObj != NULL)
			{
				delete pObj;
			}

			free_list.erase(iter++);

			// ����һ�λ��յ��������������
			i++;
			if (i >= nShrinkNum)
			{
				break;
			}
		}
	}

	// ��ӡ����������
	void showinfo()
	{
		printf("free_list.size=%d, used_list.size=%d, nUsedMax=%d \n", free_list.size(), used_list.size(), nUsedMax);
	}

	// ����
	void clear()
	{
		for (auto iter: free_list)
		{
			T* pObj = iter;
			if (pObj != NULL)
			{
				delete pObj;
			}
		}
		free_list.clear();
		for (auto iter : free_list)		
		{
			T* pObj = iter;
			if (pObj != NULL)
			{
				delete pObj;
			}
		}
		used_list.clear();

		nInitNum = 0;	// ��ʼ����
		nUsedMax = 0;	// ����ʹ�õ�������
		nIncreaseNum = 0;	// һ������������
		nShrinkNum = 0;	// ���λ��յ��������
	}
private:
	std::unordered_set<T*> 	free_list;		// �����б�
	std::unordered_set<T*> 	used_list;		// ʹ���е��б�
	int 					nInitNum;		// ��ʼ����
	int 					nUsedMax;		// ����ʹ�õ�������
	int 					nIncreaseNum;	// һ������������
	int 					nShrinkNum;		// ���λ��յ��������
};