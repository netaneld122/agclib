#pragma once

#include <functional>
#include <list>

namespace agc {

/*
	Generic tool that helps perform weighted evaluations based on a history of previously saved values.
	Usage example:

		WeightedEvaluator<double, double> evaluator(3, [](const std::list<double>& history) {
			double sum = std::accumulate(history.begin(), history.end(), static_cast<double>(0));
			return sum / history.size();
		});

		evaluator.addValue(0);	// returns 1
		evaluator.addValue(10);	// returns 5
		evaluator.addValue(5);	// returns 5
		evaluator.addValue(15);	// returns 10
*/
template <typename T, typename S>
class WeightedEvaluator
{
public:
	/*
		@param capacity	 - The size of the evaluations history list that should be remembered
		@param evaluator - Callback that performs evaluations based on a list of previous values
	*/
	WeightedEvaluator(
		size_t capacity,
		const std::function<S(const std::list<T>&)>& evaluator);

	/*
		Add a new value T to the history of values and trigger a new evaluation that returns S
	*/
	S addValue(const T& value);

	/*
		Returns whether the evaluation history list is full
	*/
	bool isHistoryFull() const;

private:
	size_t m_capacity;
	std::list<T> m_history;
	std::function<S(const std::list<T>&)> m_evaluator;
};

template <typename T, typename S>
WeightedEvaluator<T, S>::WeightedEvaluator(
	size_t capacity,
	const std::function<S(const std::list<T>&)>& evaluator)
	: m_capacity(capacity)
	, m_history(capacity)
	, m_evaluator(evaluator)
{ }

template <typename T, typename S>
S WeightedEvaluator<T, S>::addValue(const T& value)
{
	if (isHistoryFull()) {
		m_history.pop_back();
	}
	m_history.push_front(value);
	return m_evaluator(m_history);
}

template <typename T, typename S>
bool WeightedEvaluator<T, S>::isHistoryFull() const
{
	return m_history.size() == m_capacity;
}

}