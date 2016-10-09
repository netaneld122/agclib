#pragma once

#include <vector>

namespace agc {
namespace pcm {

/*
	Calculate the peak amplitude of the PCM in a scale of 0.0 to 1.0
*/
double calculatePeakAmplitude(const std::vector<char>& pcm);

}
}