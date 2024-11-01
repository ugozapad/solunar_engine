#ifndef SINGLETON_H
#define SINGLETON_H

namespace solunar
{

template <typename T>
class Singleton
{
protected:
	Singleton()
	{
		if (!ms_instance)
			ms_instance = static_cast<T*>(this);
	}

public:
	static T* GetInstance()
	{
		return ms_instance;
	}

private:
	static T* ms_instance;
};

template <typename T>
T* Singleton<T>::ms_instance = nullptr;

}

#endif //!SINGLETON_H