#include <list>
#include <numeric>

#include "PCM.h"
#include "AutomaticGainControl.h"

namespace agc {

double evaluateAmplitude(const std::list<double>& amplitudes)
{
	double sum = std::accumulate(amplitudes.begin(), amplitudes.end(), static_cast<double>(0));
	double average = sum / amplitudes.size();
	return average;
}

double evaluateMicVolume(const std::list<double>& amplitudes)
{
	double favorNewFactor = 0.2;
	double inputDecreaseFactor = 0.5;
	unsigned int resolution = 10; // On a scale of 0 to 100
	double volume = 0;
	for (auto& amplitude : amplitudes) {
		volume = (1 - favorNewFactor) * volume + favorNewFactor * amplitude * inputDecreaseFactor;
	}

	// Round the volume to match the resolution
	volume = static_cast<unsigned int>(std::round(volume * 100)) / resolution * resolution / 100.0;

	// Microphone target volume is the amplitude inverse
	return 1 - volume;
}

AutomaticGainControl::AutomaticGainControl()
	: m_evaluators({
		WeightedEvaluator<double, double>(5, &evaluateAmplitude),
		WeightedEvaluator<double, double>(20, &evaluateMicVolume)})
{ }

void AutomaticGainControl::addWeightedEvaluator(WeightedEvaluator<double, double>& evaluator)
{
	m_evaluators.push_back(evaluator);
}

double AutomaticGainControl::evaluateMicrophoneTargetVolume(const std::vector<char>& pcm)
{
	double peakAmplitude = pcm::calculatePeakAmplitude(pcm);
	double evaluation = peakAmplitude;
	
	// Chain evaluations
	for (auto& evaluator : m_evaluators) {
		evaluation = evaluator.addValue(evaluation);
	}
	return evaluation;
}

}