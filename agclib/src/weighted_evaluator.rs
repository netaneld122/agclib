use std::collections::VecDeque;

/// Maintains a bounded history of values and applies an evaluator on each update.
///
/// The history is ordered **newest-first**: `history[0]` is the most recently
/// added value. The evaluator receives an immutable view of the full history.
///
/// # Example
///
/// ```
/// use agclib::WeightedEvaluator;
///
/// let mut ev = WeightedEvaluator::new(3, |h| h.iter().sum::<f64>() / h.len() as f64);
/// assert_eq!(ev.add_value(0.0),  0.0);   // history = [0]
/// assert_eq!(ev.add_value(10.0), 5.0);   // history = [10, 0]
/// assert_eq!(ev.add_value(5.0),  5.0);   // history = [5, 10, 0]
/// assert_eq!(ev.add_value(15.0), 10.0);  // history = [15, 5, 10]  (0 evicted)
/// ```
pub struct WeightedEvaluator<T, S> {
    capacity: usize,
    history: VecDeque<T>,
    #[allow(clippy::type_complexity)]
    evaluator: Box<dyn Fn(&VecDeque<T>) -> S>,
}

impl<T, S> WeightedEvaluator<T, S> {
    /// Creates a new evaluator with the given history capacity and callback.
    ///
    /// The history starts empty; `is_history_full` returns `false` until
    /// `capacity` values have been added.
    pub fn new(capacity: usize, evaluator: impl Fn(&VecDeque<T>) -> S + 'static) -> Self {
        Self {
            capacity,
            history: VecDeque::with_capacity(capacity),
            evaluator: Box::new(evaluator),
        }
    }

    /// Prepends `value` to the history (evicting the oldest entry when full)
    /// and returns the evaluator's result over the updated history.
    pub fn add_value(&mut self, value: T) -> S {
        if self.history.len() == self.capacity {
            self.history.pop_back();
        }
        self.history.push_front(value);
        (self.evaluator)(&self.history)
    }

    /// Returns `true` once the history has accumulated `capacity` values.
    pub fn is_history_full(&self) -> bool {
        self.history.len() == self.capacity
    }
}
