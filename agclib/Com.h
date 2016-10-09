#pragma once

#include <objbase.h>
#include <stdexcept>
#include <string>

namespace agc {

void comcheck(HRESULT result);

class ComException : public std::runtime_error
{
public:
	ComException(HRESULT result)
		: std::runtime_error(std::string("COM exception ") + std::to_string(result))
	{ }
};

/**
	Scoped COM Library initializer
*/
class Com
{
public:
	Com()
	{
		comcheck(CoInitialize(NULL));
	}
	
	~Com()
	{
		CoUninitialize();
	}
};

}