# agclib #
C++ Automatic Gain Control library

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