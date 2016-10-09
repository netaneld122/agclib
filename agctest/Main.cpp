#include <Windows.h>
#include <mmsystem.h>

#include <limits>
#include <numeric>
#include <cassert>
#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

#include "COM.h"
#include "PCM.h"
#include "AutomaticGainControl.h"
#include "MicrophoneController.h"

enum Channels
{
	MONO = 1,
	STEREO = 2
};

WAVEFORMATEX getWaveFormat(unsigned int samplingRate, Channels channels)
{
	WAVEFORMATEX format = {0};
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nSamplesPerSec = samplingRate;
	format.wBitsPerSample = sizeof(short) * 8;
	format.nChannels = channels;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nChannels * sizeof(short);
	format.nBlockAlign = format.nChannels * sizeof(short);
	format.cbSize = 0;
	return format;
}

WAVEHDR getWaveHeader(unsigned int samplingRate, char* buffer, size_t bufferSize)
{
	WAVEHDR waveHeader = {0};
	waveHeader.lpData = static_cast<LPSTR>(buffer);
	waveHeader.dwBufferLength = static_cast<DWORD>(bufferSize);
	waveHeader.dwBytesRecorded = 0;
	waveHeader.dwUser = 0;
	waveHeader.dwFlags = 0;
	waveHeader.dwLoops = 0;
	return waveHeader;
}

void mmcheck(MMRESULT result)
{
	if (result != MMSYSERR_NOERROR) {
		throw std::runtime_error(std::string("Multimedia error ") + std::to_string(result));
	}
}

const unsigned int SAMPLING_RATE = 32000; // 32kHz
const Channels CHANNELS = MONO;
const unsigned int TIMEOUT_IN_SECONDS = 30;

void doit()
{
	// Open handle to microphone
	HWAVEIN waveHandle;
	WAVEFORMATEX waveFormat = getWaveFormat(SAMPLING_RATE, CHANNELS);
	mmcheck(waveInOpen(&waveHandle, WAVE_MAPPER, &waveFormat, 0, 0, WAVE_FORMAT_DIRECT));

	// Prepare buffering
	const unsigned int PCM_BUFFER_SIZE = SAMPLING_RATE * sizeof(short) / 10;
	std::vector<char> pcm(PCM_BUFFER_SIZE);
	WAVEHDR waveHeader = getWaveHeader(SAMPLING_RATE, &pcm[0], PCM_BUFFER_SIZE);
	mmcheck(waveInPrepareHeader(waveHandle, &waveHeader, sizeof(WAVEHDR)));
	mmcheck(waveInAddBuffer(waveHandle, &waveHeader, sizeof(WAVEHDR)));

	std::cout << "Press <enter> to start recording..." << std::endl;
	std::cin.ignore();

	// Start recording
	mmcheck(waveInStart(waveHandle));

	// AGC
	agc::AutomaticGainControl agc;
	agc::Com com;
	agc::MicrophoneController micController;

	// Save values later to be restored
	double micVolumeBeforeTest = micController.getVolume();

	std::cout << "Recording for " << TIMEOUT_IN_SECONDS << " seconds" << std::endl;

	// Record for some time
	int timeout = 1000 * TIMEOUT_IN_SECONDS;
	while (timeout >= 0) {
		if (waveHeader.dwFlags & WHDR_DONE) {

			// AGC
			double peakAmplitude = agc::pcm::calculatePeakAmplitude(pcm);
			double targetVolume = agc.evaluateMicrophoneTargetVolume(pcm);
			double currentMicVolume = micController.getVolume();
			printf("%.2f%%", peakAmplitude * 100);
			if (targetVolume < currentMicVolume - 0.1) {
				printf("\t\t%.2f -> %.2f", currentMicVolume, targetVolume);
				micController.setVolume(static_cast<float>(targetVolume));
				currentMicVolume = targetVolume;
			}
			std::cout << std::endl;
			
			// Prepare buffers again
			mmcheck(waveInPrepareHeader(waveHandle, &waveHeader, sizeof(WAVEHDR)));
			mmcheck(waveInAddBuffer(waveHandle, &waveHeader, sizeof(WAVEHDR)));
		}
		Sleep(20);
		timeout -= 20;
	}

	// Restore the microphone volume as it was before the test
	printf("Restoring volume to be %.2f", micVolumeBeforeTest);
	micController.setVolume(static_cast<float>(micVolumeBeforeTest));

	// Stop recording
	mmcheck(waveInStop(waveHandle));

	// Clean buffering
	while (!(waveHeader.dwFlags & WHDR_DONE)) { Sleep(100); }
	mmcheck(waveInUnprepareHeader(waveHandle, &waveHeader, sizeof(WAVEHDR)));

	// Close handle to microphone
	mmcheck(waveInClose(waveHandle));
}

int main(int argc, char* argv[])
{
	doit();
	return ERROR_SUCCESS;
}