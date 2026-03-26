use windows::Win32::{
    Media::Audio::{
        eCapture, eConsole,
        Endpoints::IAudioEndpointVolume,
        IMMDeviceEnumerator, MMDeviceEnumerator,
    },
    System::Com::{CoCreateInstance, CLSCTX_ALL, CLSCTX_INPROC_SERVER},
};

use crate::{AgcError, Result};

/// Controls the scalar volume of the default audio recording endpoint.
///
/// The underlying COM interface is reference-counted by `windows-rs`; `Drop`
/// releases it automatically — do not call `Release` manually.
pub struct MicrophoneController {
    endpoint_volume: IAudioEndpointVolume,
}

impl MicrophoneController {
    pub fn new() -> Result<Self> {
        // SAFETY: COM must be initialised on this thread (via ComGuard) before
        // calling CoCreateInstance. The caller is responsible for that invariant.
        let endpoint_volume = unsafe {
            let enumerator: IMMDeviceEnumerator =
                CoCreateInstance(&MMDeviceEnumerator, None, CLSCTX_INPROC_SERVER)?;
            let device = enumerator.GetDefaultAudioEndpoint(eCapture, eConsole)?;
            device.Activate(CLSCTX_ALL, None)?
        };
        Ok(Self { endpoint_volume })
    }

    /// Sets the master scalar volume.
    ///
    /// # Errors
    ///
    /// Returns [`AgcError::InvalidVolume`] if `volume` is outside `[0.0, 1.0]`.
    pub fn set_volume(&self, volume: f32) -> Result<()> {
        if !(0.0_f32..=1.0_f32).contains(&volume) {
            return Err(AgcError::InvalidVolume(volume));
        }
        // SAFETY: endpoint_volume is a valid COM interface acquired in new().
        unsafe {
            self.endpoint_volume
                .SetMasterVolumeLevelScalar(volume, std::ptr::null())?;
        }
        Ok(())
    }

    /// Returns the current master scalar volume in `[0.0, 1.0]`.
    pub fn get_volume(&self) -> Result<f32> {
        // SAFETY: endpoint_volume is a valid COM interface acquired in new().
        unsafe { Ok(self.endpoint_volume.GetMasterVolumeLevelScalar()?) }
    }
}
