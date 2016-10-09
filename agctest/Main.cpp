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

#include "WeightedEvaluator.h"

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

double calculateRMS(const std::vector<char>& pcm)
{
	size_t numberOfSamples = pcm.size() / sizeof(short);
	double sum = 0;
	for (size_t i = 0; i < numberOfSamples; i += sizeof(short)) {
		short sample = *reinterpret_cast<const short*>(&pcm[i]);
		sample = sample / (1 << 15); // Normalize
		sum += sample * sample;
	}
	return std::sqrt(sum / numberOfSamples);
}

inline double convertRMStoDecibel(double rms)
{
	return 20 * std::log10(rms) + 20;
}

double calculatePeakAmplitude(const std::vector<char>& pcm)
{
	size_t numberOfSamples = pcm.size() / sizeof(short);
	double peak = 0;
	for (size_t i = 0; i < numberOfSamples; i += sizeof(short)) {
		short sample = std::abs(*reinterpret_cast<const short*>(&pcm[i]));
		if (peak < sample) {
			peak = sample;
		}
	}
	return peak / (1 << 15); // Normalize to 0-100 scale
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

	agc::WeightedEvaluator<double, double> evaluator(5, [](const std::list<double>& amplitudes) {
		return 100 - (std::accumulate(amplitudes.begin(), amplitudes.end(), 0) / amplitudes.size());
	});

	// Record for some time
	int timeout = 1000 * TIMEOUT_IN_SECONDS;
	while (timeout >= 0) {
		if (waveHeader.dwFlags & WHDR_DONE) {
			double peakAmplitude = calculatePeakAmplitude(pcm) * 100;
			double evaluation = evaluator.addValue(peakAmplitude);
			printf("%.2f%%\t%.2f\n", peakAmplitude, evaluation);
			//std::cout << convertRMStoDecibel(calculateRMS(pcm)) << "dB" << std::endl;
			mmcheck(waveInPrepareHeader(waveHandle, &waveHeader, sizeof(WAVEHDR)));
			mmcheck(waveInAddBuffer(waveHandle, &waveHeader, sizeof(WAVEHDR)));
		}
		Sleep(20);
		timeout -= 20;
	}

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