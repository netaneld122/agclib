/// Library-level error type.
#[derive(Debug, thiserror::Error)]
pub enum AgcError {
    #[error("volume {0} is out of range; must be in [0.0, 1.0]")]
    InvalidVolume(f32),

    #[error("Windows API error: {0}")]
    Windows(#[from] windows::core::Error),
}

pub type Result<T> = std::result::Result<T, AgcError>;
