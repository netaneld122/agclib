#pragma once

#include <list>
#include <vector>

#include "WeightedEvaluator.h"

namespace agc {

/*
	Microphone target gain evaluator based on recorded PCM segment
*/
class AutomaticGainControl
{
public:
	AutomaticGainControl();

	/*
		Add a custom weighted evaluator to chain additional evaluation layers
	*/
	void addWeightedEvaluator(WeightedEvaluator<double, double> evaluator);

	/*
		Get the microphone target volume that should be set, on a scale from 0.0 to 1.0
	*/
	double evaluateMicrophoneTargetVolume(const std::vector<char>& pcm);

private:
	std::list<WeightedEvaluator<double, double>> m_evaluators;
};

}
