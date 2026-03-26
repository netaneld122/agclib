use std::marker::PhantomData;

use windows::Win32::System::Com::{CoInitializeEx, CoUninitialize, COINIT_APARTMENTTHREADED};

use crate::Result;

/// RAII guard for COM library initialisation.
///
/// Calls `CoInitializeEx` on construction and `CoUninitialize` on drop.
/// Marked `!Send` because COM apartment-threading must remain on the
/// thread that called `CoInitializeEx`.
pub struct ComGuard {
    // PhantomData<*mut ()> makes ComGuard !Send and !Sync.
    _not_send: PhantomData<*mut ()>,
}

impl ComGuard {
    pub fn new() -> Result<Self> {
        // SAFETY: CoInitializeEx is safe to call; the only requirement is that
        // CoUninitialize is called the same number of times on the same thread,
        // which Drop guarantees.
        unsafe { CoInitializeEx(None, COINIT_APARTMENTTHREADED)? };
        Ok(Self {
            _not_send: PhantomData,
        })
    }
}

impl Drop for ComGuard {
    fn drop(&mut self) {
        // SAFETY: CoUninitialize balances the CoInitializeEx in new().
        unsafe { CoUninitialize() };
    }
}
