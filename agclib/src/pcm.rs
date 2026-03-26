/// Maximum absolute value of a 16-bit signed PCM sample.
const MAX_AMPLITUDE: f64 = (1u32 << 15) as f64;

/// Returns the peak amplitude of a 16-bit little-endian PCM buffer,
/// normalised to `[0.0, 1.0]`. Returns `0.0` for an empty buffer.
pub fn calculate_peak_amplitude(pcm: &[u8]) -> f64 {
    let peak = pcm
        .chunks_exact(size_of::<i16>())
        .map(|bytes| {
            // SAFETY: chunks_exact guarantees exactly size_of::<i16>() bytes.
            i16::from_ne_bytes(bytes.try_into().unwrap()).unsigned_abs()
        })
        .max()
        .unwrap_or(0);

    peak as f64 / MAX_AMPLITUDE
}
