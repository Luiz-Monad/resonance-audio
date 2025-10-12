/*
Copyright 2018 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#include "binaural_renderer_processor.h"
#include "binaural_renderer_controller.h"

#include <cstdlib>
#include <unordered_map>
#include <vector>
#include <algorithm>

// Resonance API
#include "base/constants_and_types.h"

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace vraudio;

// -----------------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------------
BinauralRendererVst::BinauralRendererVst() {
	setControllerClass(BinauralRendererControllerUID);
}

BinauralRendererVst::~BinauralRendererVst() {
	binauralSurroundRenderer_.reset();
}

// -----------------------------------------------------------------------------
// initialize / terminate
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVst::initialize(FUnknown* context) {
	tresult result = AudioEffect::initialize(context);
	if (result != kResultOk) {
		return result;
	}

	// Add a single input bus that accepts a discrete arrangement (arbitrary channel count)
	// and a single stereo output bus.
	// Host may reconfigure the input bus arrangement later via setBusArrangements.
	addAudioInput(STR16("Input"), SpeakerArr::kAmbi3rdOrderACN + SpeakerArr::kMono);
	addAudioOutput(STR16("Output"), SpeakerArr::kStereo);

	return kResultOk;
}

tresult PLUGIN_API BinauralRendererVst::terminate() {
	binauralSurroundRenderer_.reset();
	return AudioEffect::terminate();
}

// -----------------------------------------------------------------------------
// setBusArrangements
// The host will call this to configure channel counts for input/output.
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVst::setBusArrangements(
	SpeakerArrangement* inputs,
	int32 numIns,
	SpeakerArrangement* outputs,
	int32 numOuts)
{
	// We expect at least one input and one output bus.
	if (numIns <= 0 || numOuts <= 0) {
		return kResultFalse;
	}

	// Accept any input arrangement (discrete/ambisonic). The ambisonic support check
	// is performed later in initBinauralSurroundRenderer based on channel count.
	// Keep the bus arrangement by delegating to base implementation.
	return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
}

// -----------------------------------------------------------------------------
// setBusArrangements
// The host will call this to tell ask us the precision we support.
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVst::canProcessSampleSize(int32 symbolicSampleSize)
{
	return (symbolicSampleSize == SymbolicSampleSizes::kSample32)
		? kResultTrue
		: kResultFalse;
}

// -----------------------------------------------------------------------------
// setActive
// Called when the component is activated/deactivated by host.
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVst::setActive(TBool state) {
	if (!state) {
		// deactivated: free renderer.
		binauralSurroundRenderer_.reset();
		framesPerBuffer_ = 0;
		numInputChannels_ = 0;
		sampleRateHz_ = 0;
	}
	return AudioEffect::setActive(state);
}

// -----------------------------------------------------------------------------
// setupProcess
// Initialize the BinauralSurroundRenderer
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVst::setupProcessing(ProcessSetup& setup) {
	tresult result = AudioEffect::setupProcessing(setup);
	if (result != kResultOk)
		return result;

	if (setup.sampleRate < 8000 || setup.sampleRate >= 384000) {
		binauralSurroundRenderer_.reset();
		return kInvalidArgument;
	}

	framesPerBuffer_ = static_cast<int32>(setup.maxSamplesPerBlock);
	sampleRateHz_ = static_cast<int>(setup.sampleRate);

	// Query input bus arrangement to determine channel count
	SpeakerArrangement inputArr;
	if (getBusArrangement(kInput, 0, inputArr) == kResultOk) {
		int32 numInputChannels = SpeakerArr::getChannelCount(inputArr);
		numInputChannels_ = numInputChannels;
		initBinauralSurroundRenderer(framesPerBuffer_, numInputChannels_, sampleRateHz_);
	}

	return kResultOk;
}

// -----------------------------------------------------------------------------
// process
// Build planar float** arrays from VST3 buffers and call the original
// processReplacing(...) routine.
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVst::process(ProcessData& data) {
	if (data.numInputs == 0 || data.numOutputs == 0 || data.numSamples <= 0)
		return kResultOk;

	AudioBusBuffers& inBus = data.inputs[0];
	AudioBusBuffers& outBus = data.outputs[0];

	const int32 sampleFrames = data.numSamples;
	const int32 numInputChannels = inBus.numChannels;
	const int32 numOutputChannels = outBus.numChannels;

	if (!binauralSurroundRenderer_)
		return kResultOk;

	// Directly use the channelBuffers32 arrays.
	binauralSurroundRenderer_->AddPlanarInput(inBus.channelBuffers32, numInputChannels, sampleFrames);
	binauralSurroundRenderer_->GetPlanarStereoOutput(outBus.channelBuffers32, sampleFrames);

	// Zero any unused output channels beyond stereo.
	for (int32 ch = static_cast<int32>(kNumStereoChannels); ch < numOutputChannels; ++ch)
		std::fill_n(outBus.channelBuffers32[ch], sampleFrames, 0.0f);

	return kResultOk;
}

// -----------------------------------------------------------------------------
// Handle parameter changes (optional — keep stubs for now)
// -----------------------------------------------------------------------------
void BinauralRendererVst::handleParameterChanges(IParameterChanges* changes)
{
}

// -----------------------------------------------------------------------------
// State serialization (optional — keep stubs for now)
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVst::setState(IBStream* state) {
	// Read plugin state if you serialize any parameters (none in this simplified example).
	return kResultOk;
}

tresult PLUGIN_API BinauralRendererVst::getState(IBStream* state) {
	// Write plugin state if needed
	return kResultOk;
}

// -----------------------------------------------------------------------------
// Actual Frame Processing
// -----------------------------------------------------------------------------
void BinauralRendererVst::processReplacing(
	int32 numInputChannels, float** inputs,
	int32 numOutputChannels, float** outputs,
	int32 sampleFrames, int32 sampleRate)
{
	int32 numOutputChannelsProcessed = 0;

	// Ignore zero input / output channel configurations.
	if (numOutputChannels == 0 || numInputChannels == 0) {
		return;
	}

	// Ignore mono output channel configuration.
	if (numOutputChannels == static_cast<int32>(kNumMonoChannels)) {
		std::fill_n(outputs[0], sampleFrames, 0.0f);
		return;
	}

	if (framesPerBuffer_ != sampleFrames ||
		numInputChannels_ != numInputChannels ||
		sampleRateHz_ != static_cast<int>(sampleRate)) {
		if (!initBinauralSurroundRenderer(sampleFrames, numInputChannels,
			static_cast<int>(sampleRate))) {
			binauralSurroundRenderer_.reset();
		}
	}

	if (binauralSurroundRenderer_ != nullptr) {
		binauralSurroundRenderer_->AddPlanarInput(inputs, numInputChannels,
			sampleFrames);
		binauralSurroundRenderer_->GetPlanarStereoOutput(outputs, sampleFrames);
		numOutputChannelsProcessed += static_cast<int32>(kNumStereoChannels);
	}

	// Clear remaining output buffers.
	for (int32 channel = numOutputChannelsProcessed;
		channel < numOutputChannels; ++channel) {
		std::fill_n(outputs[channel], sampleFrames, 0.0f);
	}
}

bool BinauralRendererVst::initBinauralSurroundRenderer(
	int32 framesPerBuffer, int32 numInputChannels, int sampleRateHz) {
	BinauralSurroundRenderer::SurroundFormat surround_format =
		BinauralSurroundRenderer::SurroundFormat::kInvalid;
	switch (numInputChannels) {
	case kNumFirstOrderAmbisonicChannels:
		surround_format =
			BinauralSurroundRenderer::SurroundFormat::kFirstOrderAmbisonics;
		break;
	case kNumFirstOrderAmbisonicWithNonDiegeticStereoChannels:
		surround_format = BinauralSurroundRenderer::SurroundFormat::
			kFirstOrderAmbisonicsWithNonDiegeticStereo;
		break;
	case kNumSecondOrderAmbisonicChannels:
		surround_format =
			BinauralSurroundRenderer::SurroundFormat::kSecondOrderAmbisonics;
		break;
	case kNumSecondOrderAmbisonicWithNonDiegeticStereoChannels:
		surround_format = BinauralSurroundRenderer::SurroundFormat::
			kSecondOrderAmbisonicsWithNonDiegeticStereo;
		break;
	case kNumThirdOrderAmbisonicChannels:
		surround_format =
			BinauralSurroundRenderer::SurroundFormat::kThirdOrderAmbisonics;
		break;
	case kNumThirdOrderAmbisonicChannelsWithMono:
		surround_format =
			BinauralSurroundRenderer::SurroundFormat::kThirdOrderAmbisonicsWithMono;
		break;
	case kNumThirdOrderAmbisonicWithNonDiegeticStereoChannels:
		surround_format = BinauralSurroundRenderer::SurroundFormat::
			kThirdOrderAmbisonicsWithNonDiegeticStereo;
		break;
	default:
		// Unsupported number of input channels.
		return false;
		break;
	}

	binauralSurroundRenderer_.reset(BinauralSurroundRenderer::Create(
		framesPerBuffer, sampleRateHz, surround_format));
	if (binauralSurroundRenderer_ == nullptr) {
		return false;
	}

	framesPerBuffer_ = framesPerBuffer;
	numInputChannels_ = numInputChannels;
	sampleRateHz_ = sampleRateHz;
	return true;
}
