mod automatic_gain_control;
mod com;
mod microphone_controller;
mod pcm;
mod weighted_evaluator;

pub mod error;

pub use automatic_gain_control::AutomaticGainControl;
pub use com::ComGuard;
pub use error::{AgcError, Result};
pub use microphone_controller::MicrophoneController;
pub use pcm::calculate_peak_amplitude;
pub use weighted_evaluator::WeightedEvaluator;
