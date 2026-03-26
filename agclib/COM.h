#pragma once

#include <objbase.h>
#include <stdexcept>
#include <string>

namespace agc {

void comcheck(HRESULT result);

class ComException : public std::runtime_error
{
public:
	explicit ComException(HRESULT result)
		: std::runtime_error(std::string("COM exception ") + std::to_string(result))
	{ }
};

/*
	Minimal RAII owner for a COM interface pointer.
	Non-copyable; moveable so it can be stored in containers.
*/
template <typename T>
class ComPtr
{
public:
	ComPtr() noexcept : m_ptr(nullptr) {}
	~ComPtr() noexcept { if (m_ptr) m_ptr->Release(); }

	ComPtr(const ComPtr&) = delete;
	ComPtr& operator=(const ComPtr&) = delete;

	ComPtr(ComPtr&& other) noexcept : m_ptr(other.m_ptr) { other.m_ptr = nullptr; }
	ComPtr& operator=(ComPtr&& other) noexcept
	{
		if (this != &other) {
			if (m_ptr) m_ptr->Release();
			m_ptr = other.m_ptr;
			other.m_ptr = nullptr;
		}
		return *this;
	}

	T* operator->() const noexcept { return m_ptr; }
	explicit operator bool() const noexcept { return m_ptr != nullptr; }

	// Pass &ptr to COM output parameters
	T** put() noexcept { return &m_ptr; }

private:
	T* m_ptr;
};

/*
	Scoped COM library initializer — non-copyable, non-moveable.
*/
class Com
{
public:
	Com() { comcheck(CoInitialize(NULL)); }
	~Com() noexcept { CoUninitialize(); }

	Com(const Com&) = delete;
	Com& operator=(const Com&) = delete;
};

}
