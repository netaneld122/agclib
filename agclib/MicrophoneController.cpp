#pragma once

#include <cassert>

#include <mmdeviceapi.h>
#include <endpointvolume.h>

#include "Com.h"
#include "MicrophoneController.h"

namespace agc {

static const IID IID_IAudioEndpointVolume = __uuidof(IAudioEndpointVolume);

MicrophoneController::MicrophoneController()
{
	// Get device enumerator
	IMMDeviceEnumerator* deviceEnumerator = NULL;
	comcheck(CoCreateInstance(
		__uuidof(MMDeviceEnumerator),
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<LPVOID*>(&deviceEnumerator)));

	// Get default recording endpoint
	IMMDevice* pEndptDev = NULL;
	comcheck(deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pEndptDev));
	deviceEnumerator->Release();
	
	IAudioEndpointVolume* pAudioEndpointVolume = NULL;
	comcheck(pEndptDev->Activate(IID_IAudioEndpointVolume, CLSCTX_ALL, NULL, (void**)&pAudioEndpointVolume));
	pEndptDev->Release();

	m_audioEndpointVolume = pAudioEndpointVolume;
}

MicrophoneController::~MicrophoneController()
{
	m_audioEndpointVolume->Release();
}

void MicrophoneController::setVolume(float volume)
{
	assert(volume >= 0 && volume <= 1);
	comcheck(m_audioEndpointVolume->SetMasterVolumeLevelScalar(volume, NULL));
}

float MicrophoneController::getVolume() const
{
	float volume;
	comcheck(m_audioEndpointVolume->GetMasterVolumeLevelScalar(&volume));
	return volume;
}

}