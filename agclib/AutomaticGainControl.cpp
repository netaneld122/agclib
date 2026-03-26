#include <cmath>
#include <list>
#include <numeric>

#include "PCM.h"
#include "AutomaticGainControl.h"

namespace agc {
namespace {

double evaluateAmplitude(const std::list<double>& amplitudes)
{
	double sum = std::accumulate(amplitudes.begin(), amplitudes.end(), 0.0);
	return sum / amplitudes.size();
}

double evaluateMicVolume(const std::list<double>& amplitudes)
{
	constexpr double FAVOR_NEW_FACTOR = 0.2;
	constexpr double INPUT_DECREASE_FACTOR = 0.5;
	constexpr unsigned int RESOLUTION = 10; // On a scale of 0 to 100

	double volume = 0;
	for (auto it = amplitudes.rbegin(); it != amplitudes.rend(); ++it) {
		volume = (1 - FAVOR_NEW_FACTOR) * volume + FAVOR_NEW_FACTOR * (*it) * INPUT_DECREASE_FACTOR;
	}

	// Round the volume to match the resolution
	volume = static_cast<unsigned int>(std::round(volume * 100)) / RESOLUTION * RESOLUTION / 100.0;

	// Microphone target volume is the amplitude inverse
	return 1 - volume;
}

} // namespace

AutomaticGainControl::AutomaticGainControl()
	: m_evaluators({
		WeightedEvaluator<double, double>(5, &evaluateAmplitude),
		WeightedEvaluator<double, double>(20, &evaluateMicVolume)})
{ }

void AutomaticGainControl::addWeightedEvaluator(WeightedEvaluator<double, double> evaluator)
{
	m_evaluators.push_back(std::move(evaluator));
}

double AutomaticGainControl::evaluateMicrophoneTargetVolume(const std::vector<char>& pcm)
{
	double evaluation = pcm::calculatePeakAmplitude(pcm);

	for (auto& evaluator : m_evaluators) {
		evaluation = evaluator.addValue(evaluation);
	}
	return evaluation;
}

}
