#pragma once

#include <endpointvolume.h>

namespace agc {

class MicrophoneController
{
public:
	MicrophoneController();
	~MicrophoneController();

	void setVolume(float volume);
	float getVolume() const;

private:
	IAudioEndpointVolume* m_audioEndpointVolume;
};

}