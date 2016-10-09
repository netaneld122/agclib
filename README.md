# agclib #
A C++ Automatic Gain Control (agc) library for Windows

Basic usage:
```
#!c++
agc::AutomaticGainControl agc;
agc::MicrophoneController micController;

while (...recording...) {
	double targetVolume = agc.evaluateMicrophoneTargetVolume(pcm);
	micController.setVolume(static_cast<float>(targetVolume));
}
```

See `agctest -> Main.cpp` for more advanced examples