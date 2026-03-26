use std::{
    io::{self, BufRead},
    mem::size_of,
    thread,
    time::{Duration, Instant},
};

use windows::core::PSTR;
use windows::Win32::Media::Audio::{
    waveInAddBuffer, waveInClose, waveInOpen, waveInPrepareHeader, waveInStart, waveInStop,
    waveInUnprepareHeader, HWAVEIN, WAVEFORMATEX, WAVEHDR, WAVE_FORMAT_DIRECT, WAVE_FORMAT_PCM,
    WAVE_MAPPER,
};

use agclib::{AutomaticGainControl, ComGuard, MicrophoneController};

const SAMPLING_RATE: u32 = 32_000;
const CHANNELS: u16 = 1;
const TIMEOUT: Duration = Duration::from_secs(30);
const POLL_INTERVAL: Duration = Duration::from_millis(20);
const DRAIN_INTERVAL: Duration = Duration::from_millis(100);
const VOLUME_EPSILON: f64 = 0.001;

/// Size of one 100 ms PCM chunk (mono, 16-bit, 32 kHz).
const PCM_BUFFER_SIZE: usize = SAMPLING_RATE as usize * size_of::<i16>() / 10;

/// `MMSYSERR_NOERROR` — success return value for multimedia API calls.
const MMSYSERR_NOERROR: u32 = 0;

/// Bit flag set by the driver when it finishes filling a `WAVEHDR` buffer.
const WHDR_DONE: u32 = 0x0001;

fn mmcheck(result: u32) -> Result<(), String> {
    if result == MMSYSERR_NOERROR {
        Ok(())
    } else {
        Err(format!("multimedia error {result}"))
    }
}

fn run_agc() -> Result<(), Box<dyn std::error::Error>> {
    // --- Open recording device ---
    let wave_format = WAVEFORMATEX {
        wFormatTag: WAVE_FORMAT_PCM as u16,
        nSamplesPerSec: SAMPLING_RATE,
        wBitsPerSample: (size_of::<i16>() * 8) as u16,
        nChannels: CHANNELS,
        nAvgBytesPerSec: SAMPLING_RATE * CHANNELS as u32 * size_of::<i16>() as u32,
        nBlockAlign: CHANNELS * size_of::<i16>() as u16,
        cbSize: 0,
    };

    let mut wave_handle = HWAVEIN::default();
    // SAFETY: wave_format is fully initialised and outlives this call.
    mmcheck(unsafe {
        waveInOpen(
            Some(&mut wave_handle),
            WAVE_MAPPER,
            &wave_format,
            0,
            0,
            WAVE_FORMAT_DIRECT,
        )
    })?;

    // --- Prepare buffer ---
    // `pcm` is heap-allocated; its address is stable as long as we never
    // push/pop. We pass a raw pointer to the driver below.
    let mut pcm = vec![0u8; PCM_BUFFER_SIZE];

    // `wave_header` is boxed so its address on the heap is stable for the
    // lifetime of the recording session.
    let mut wave_header = Box::new(WAVEHDR {
        lpData: PSTR(pcm.as_mut_ptr()),
        dwBufferLength: PCM_BUFFER_SIZE as u32,
        ..Default::default()
    });

    // SAFETY: wave_header and pcm are valid, non-overlapping, and will remain
    // at their current addresses for the duration of the recording session.
    mmcheck(unsafe {
        waveInPrepareHeader(
            wave_handle,
            wave_header.as_mut(),
            size_of::<WAVEHDR>() as u32,
        )
    })?;
    mmcheck(unsafe {
        waveInAddBuffer(
            wave_handle,
            wave_header.as_mut(),
            size_of::<WAVEHDR>() as u32,
        )
    })?;

    println!("Press <enter> to start recording...");
    io::stdin().lock().lines().next();

    // SAFETY: wave_handle is valid and the buffer has been prepared and queued.
    mmcheck(unsafe { waveInStart(wave_handle) })?;

    // --- AGC setup ---
    let _com = ComGuard::new()?;
    let mut agc = AutomaticGainControl::new();
    let mic = MicrophoneController::new()?;
    let original_volume = mic.get_volume()?;

    println!("Recording for {} seconds", TIMEOUT.as_secs());

    // --- Recording loop ---
    let deadline = Instant::now() + TIMEOUT;
    while Instant::now() < deadline {
        if wave_header.dwFlags & WHDR_DONE != 0 {
            let target_volume = agc.evaluate_microphone_target_volume(&pcm);
            let current_volume = f64::from(mic.get_volume()?);

            print!("target={:.2}%", (1.0 - target_volume) * 100.0);

            if target_volume < current_volume - VOLUME_EPSILON {
                print!("\t\tDOWN {current_volume:.2} -> {target_volume:.2}");
                #[allow(clippy::cast_possible_truncation)]
                mic.set_volume(target_volume as f32)?;
            } else if target_volume > current_volume + VOLUME_EPSILON {
                print!("\t\tUP   {current_volume:.2} -> {target_volume:.2}");
                #[allow(clippy::cast_possible_truncation)]
                mic.set_volume(target_volume as f32)?;
            }
            println!();

            // SAFETY: same as initial prepare/add calls above.
            mmcheck(unsafe {
                waveInPrepareHeader(
                    wave_handle,
                    wave_header.as_mut(),
                    size_of::<WAVEHDR>() as u32,
                )
            })?;
            mmcheck(unsafe {
                waveInAddBuffer(
                    wave_handle,
                    wave_header.as_mut(),
                    size_of::<WAVEHDR>() as u32,
                )
            })?;
        }
        thread::sleep(POLL_INTERVAL);
    }

    // --- Restore and clean up ---
    println!("Restoring previous microphone volume (->{original_volume:.2})");
    mic.set_volume(original_volume)?;

    // SAFETY: wave_handle is valid and recording has been started.
    mmcheck(unsafe { waveInStop(wave_handle) })?;

    // Drain the last buffer before unpreparing.
    while wave_header.dwFlags & WHDR_DONE == 0 {
        thread::sleep(DRAIN_INTERVAL);
    }

    // SAFETY: wave_header is the same buffer that was prepared above.
    mmcheck(unsafe {
        waveInUnprepareHeader(
            wave_handle,
            wave_header.as_mut(),
            size_of::<WAVEHDR>() as u32,
        )
    })?;
    mmcheck(unsafe { waveInClose(wave_handle) })?;

    Ok(())
}

fn main() {
    if let Err(e) = run_agc() {
        eprintln!("Error: {e}");
        std::process::exit(1);
    }
}
