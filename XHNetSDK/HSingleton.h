#pragma once

#include <iostream>
class singleton {
public:
	// ��ȡ��ʵ������
	static singleton& getInstance() {
		static singleton instance;
		return instance;
	}
private:
	// ��ֹ�ⲿ����
	singleton() = default;
	// ��ֹ�ⲿ����
	~singleton() = default;
	// ��ֹ�ⲿ���ƹ���
	singleton(const singleton&) = delete;
	// ��ֹ�ⲿ��ֵ����
	singleton& operator=(const singleton&) = delete;
};



template<typename T>
class Singleton {
public:
	static T& getInstance() {
		static T instance;
		return instance;
	}

	virtual ~Singleton() {
		std::cout << "destructor called!" << std::endl;
	}

	Singleton(const Singleton&) = delete;
	Singleton& operator =(const Singleton&) = delete;

protected:
	Singleton() {
		std::cout << "constructor called!" << std::endl;
	}
};

/********************************************/
// Example:
// 1.friend class declaration is requiered!
// 2.constructor should be private

class DerivedSingle : public Singleton<DerivedSingle> {
	// !!!! attention!!!
	// needs to be friend in order to
	// access the private constructor/destructor
	friend class Singleton<DerivedSingle>;

public:
	DerivedSingle(const DerivedSingle&) = delete;
	DerivedSingle& operator =(const DerivedSingle&) = delete;

private:
	DerivedSingle() = default;
};

//int testmain(int argc, char* argv[]) {
//	DerivedSingle& instance1 = DerivedSingle::getInstance();
//	DerivedSingle& instance2 = DerivedSingle::getInstance();
//	return 0;
//}
