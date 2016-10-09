#include "COM.h"

namespace agc {

void comcheck(HRESULT result)
{
	if (FAILED(result)) {
		throw ComException(result);
	}
}

}