#include <cmath>

#include "PCM.h"

namespace agc {
namespace pcm {

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

	// Normalize to 0-1 scale
	return peak / (1 << 15);
}

}
}