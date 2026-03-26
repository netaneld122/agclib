#pragma once

#include <endpointvolume.h>

#include "COM.h"

namespace agc {

/*
	Microphone controller, uses the default recording endpoint.
	Non-copyable (unique ownership of the audio endpoint); moveable.
*/
class MicrophoneController
{
public:
	MicrophoneController();

	/*
		@param volume - Scalar volume in the range [0.0, 1.0]
		@throws std::invalid_argument if volume is outside [0.0, 1.0]
	*/
	void setVolume(float volume);
	float getVolume() const;

private:
	ComPtr<IAudioEndpointVolume> m_audioEndpointVolume;
};

}
