#include <stdexcept>

#include <mmdeviceapi.h>
#include <endpointvolume.h>

#include "COM.h"
#include "MicrophoneController.h"

namespace agc {

MicrophoneController::MicrophoneController()
{
	ComPtr<IMMDeviceEnumerator> deviceEnumerator;
	comcheck(CoCreateInstance(
		__uuidof(MMDeviceEnumerator),
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<LPVOID*>(deviceEnumerator.put())));

	ComPtr<IMMDevice> endpointDevice;
	comcheck(deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, endpointDevice.put()));

	comcheck(endpointDevice->Activate(
		__uuidof(IAudioEndpointVolume),
		CLSCTX_ALL,
		NULL,
		reinterpret_cast<void**>(m_audioEndpointVolume.put())));
}

void MicrophoneController::setVolume(float volume)
{
	if (volume < 0.0f || volume > 1.0f) {
		throw std::invalid_argument("volume must be in the range [0.0, 1.0]");
	}
	comcheck(m_audioEndpointVolume->SetMasterVolumeLevelScalar(volume, NULL));
}

float MicrophoneController::getVolume() const
{
	float volume;
	comcheck(m_audioEndpointVolume->GetMasterVolumeLevelScalar(&volume));
	return volume;
}

}
