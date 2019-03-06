#include "LM.h"
#include "Memory.h"
#include <new>

static void* newFunc(std::size_t size)
{
	void* p = nullptr;
	while ((p = Memory::malloc(size)) == nullptr) 
	{
		void(*l_handler)() = std::set_new_handler(nullptr);
		std::set_new_handler(l_handler);
		if (l_handler == nullptr) 
		{
			return nullptr;

		}

		l_handler();
	}
	return p;
}

static void deleteFunc(void* p)
{
	if (p == nullptr)
	{
		return;
	}
	Memory::free(p);
}

void* operator new(std::size_t size)
{
	void* result = newFunc(size);
	if (result == nullptr)
	{
		throw std::bad_alloc();
	}
	return result;
}

void* operator new (std::size_t size, const std::nothrow_t& nothrow_value) noexcept
{
	return newFunc(size);
}

void operator delete(void * p) throw()
{
	deleteFunc(p);
}

void* operator new[](std::size_t size)
{
	void* result = newFunc(size);
	if (result == nullptr)
	{
		throw std::bad_alloc();
	}
	return result;
}
	
void operator delete[](void *p) throw()
{
	deleteFunc(p);
}