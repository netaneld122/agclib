# agclib #
C++ Automatic Gain Control (agc) library for Windows

Basic usage:
```
#!c++
agc::AutomaticGainControl agc;
agc::MicrophoneController mic;

while (...recording...) {
	double targetVolume = agc.evaluateMicrophoneTargetVolume(pcm);
	mic.setVolume(static_cast<float>(targetVolume));
}
```

See `agctest -> Main.cpp` for more advanced examples