/*
Copyright 2025 Luiz Felipe Stangarlin

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#include "binaural_renderer_controller.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;

// -----------------------------------------------------------------------------
// Initialize
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVstController::initialize(FUnknown* context) {
	tresult result = EditController::initialize(context);
	if (result != kResultOk) {
		return result;
	}

	return kResultOk;
}

// -----------------------------------------------------------------------------
// Terminate
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVstController::terminate() {
	return EditController::terminate();
}

// -----------------------------------------------------------------------------
// Set Component State (load from stream)
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVstController::setComponentState(IBStream* state) {
	if (!state) {
		return kResultFalse;
	}

	IBStreamer streamer(state, kLittleEndian);

	return kResultFalse;
}
