#pragma once

#include <functional>
#include <list>

namespace agc {

template <typename T, typename S>
class WeightedEvaluator
{
public:
	WeightedEvaluator(
		size_t capacity,
		const std::function<S(const std::list<T>&)>& evaluator);

	S addValue(const T& value);

	bool isHistoryFull() const;

private:
	size_t m_capacity;
	std::list<T> m_history;
	std::function<void(T)> m_evaluator;
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