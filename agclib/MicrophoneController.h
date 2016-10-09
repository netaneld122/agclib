#pragma once

#include <endpointvolume.h>

namespace agc {

/*
	Microphone controller, uses the default recording endpoint
*/
class MicrophoneController
{
public:
	MicrophoneController();
	~MicrophoneController();

	/*
		@param volume - Scalar volume in the range 0.0-1.0
	*/
	void setVolume(float volume);
	float getVolume() const;

private:
	IAudioEndpointVolume* m_audioEndpointVolume;
};

}