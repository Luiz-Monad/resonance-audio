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

#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;

// -----------------------------------------------------------------------------
// Parameters
// -----------------------------------------------------------------------------

// --- Channels ---
ChannelsParameter::ChannelsParameter()
	: RangeParameter(
		STR16("Input Channels"), kParamNumInputChannels, STR16("ch"),
		1.0, 64.0, 1.0  // min, max, default
	)
{
	setPrecision(0);
}
ChannelsParameter* Steinberg::Vst::Param_NumInputChannels = new ChannelsParameter();

// -----------------------------------------------------------------------------
// Initialize
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVstController::initialize(FUnknown* context) {
	auto result = EditController::initialize(context);
	if (result != kResultOk) return result;

	// --- Channels ---
	parameters.addParameter(Param_NumInputChannels);

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

	updatingParams_ = true;

	// --- Channels ---
	int16 savedChannels;
	if (!streamer.readInt16(savedChannels)) return kResultFalse;
	setParamNormalized(kParamNumInputChannels, savedChannels);

	updatingParams_ = false;

	return kResultOk;
}
