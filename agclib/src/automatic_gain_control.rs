use std::collections::VecDeque;

use crate::{calculate_peak_amplitude, WeightedEvaluator};

fn evaluate_amplitude(amplitudes: &VecDeque<f64>) -> f64 {
    amplitudes.iter().sum::<f64>() / amplitudes.len() as f64
}

fn evaluate_mic_volume(amplitudes: &VecDeque<f64>) -> f64 {
    const FAVOR_NEW_FACTOR: f64 = 0.2;
    const INPUT_DECREASE_FACTOR: f64 = 0.5;
    const RESOLUTION: u32 = 10; // quantise to tenths on a 0–100 scale

    // Iterate oldest-to-newest so FAVOR_NEW_FACTOR gives more weight to recent samples.
    let volume = amplitudes.iter().rev().fold(0.0_f64, |vol, &amp| {
        (1.0 - FAVOR_NEW_FACTOR) * vol + FAVOR_NEW_FACTOR * amp * INPUT_DECREASE_FACTOR
    });

    // Quantise to the nearest resolution step.
    let quantised =
        ((volume * 100.0).round() as u32 / RESOLUTION * RESOLUTION) as f64 / 100.0;

    // Target volume is the amplitude inverse.
    1.0 - quantised
}

/// Evaluates the microphone target gain from a stream of 16-bit PCM buffers.
pub struct AutomaticGainControl {
    evaluators: Vec<WeightedEvaluator<f64, f64>>,
}

impl AutomaticGainControl {
    pub fn new() -> Self {
        Self {
            evaluators: vec![
                WeightedEvaluator::new(5, evaluate_amplitude),
                WeightedEvaluator::new(20, evaluate_mic_volume),
            ],
        }
    }

    /// Appends a custom evaluation stage to the chain.
    pub fn add_weighted_evaluator(&mut self, evaluator: WeightedEvaluator<f64, f64>) {
        self.evaluators.push(evaluator);
    }

    /// Returns the target microphone volume in `[0.0, 1.0]` for the given PCM buffer.
    pub fn evaluate_microphone_target_volume(&mut self, pcm: &[u8]) -> f64 {
        let mut value = calculate_peak_amplitude(pcm);
        for evaluator in &mut self.evaluators {
            value = evaluator.add_value(value);
        }
        value
    }
}

impl Default for AutomaticGainControl {
    fn default() -> Self {
        Self::new()
    }
}
