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
	auto result = AudioEffect::initialize(context);
	if (result != kResultOk) return result;

	// Add a single input bus that accepts a discrete arrangement (arbitrary channel count)
	// and a single stereo output bus.
	// Host may reconfigure the input bus arrangement later via setBusArrangements.
	addAudioInput(STR16("Input"), SpeakerArr::kAmbi3rdOrderACN);
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
	}
	active_ = state;
	return AudioEffect::setActive(state);
}

// -----------------------------------------------------------------------------
// Check if we can actually resample the output
// -----------------------------------------------------------------------------
bool areProblematicRates(int destination_frequency, int source_frequency) {
	auto kTransitionBandwidthRatio = 13;
	auto a = std::abs(destination_frequency);
	auto b = std::abs(source_frequency);
	auto gcd = 0;
	while (b != 0) {
		gcd = b;
		b = a % b;
		a = gcd;
	}
	const auto destination =
		static_cast<size_t>(destination_frequency / gcd);
	const auto source =
		static_cast<size_t>(source_frequency / gcd);
	const auto max_rate =
		std::max(destination, source);
	const auto cutoff_frequency =
		static_cast<float>(source_frequency) / static_cast<float>(2 * max_rate);
	const auto filter_length_bw =
		max_rate * kTransitionBandwidthRatio;
	const auto filter_length =
		filter_length_bw + filter_length_bw % 2;
	const auto transposed_length =
		filter_length + max_rate - (filter_length % max_rate);

	// Check if this would exceed the resampler's buffer limits
	return transposed_length > kMaxSupportedNumFrames;
}

// -----------------------------------------------------------------------------
// setupProcess
// Initialize the Resonance
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVst::setupProcessing(ProcessSetup& setup) {
	auto result = AudioEffect::setupProcessing(setup);
	if (result != kResultOk) return result;

	binauralSurroundRenderer_.reset();
	framesPerBuffer_ = static_cast<int32>(setup.maxSamplesPerBlock);
	sampleRateHz_ = static_cast<int>(setup.sampleRate);

	if (areProblematicRates(sampleRateHz_, 44100) ||
		areProblematicRates(sampleRateHz_, 48000)) {
		return kInvalidArgument;
	}

	return kResultOk;
}

// -----------------------------------------------------------------------------
// process
// Push the input to Resonance and pull the output.
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVst::process(ProcessData& data) {
	handleParameterChanges(data.inputParameterChanges);

	if (!active_ || data.numInputs == 0 || data.numOutputs == 0 || data.numSamples <= 0)
		return kResultOk;

	auto& inBus = data.inputs[0];
	auto& outBus = data.outputs[0];

	const auto sampleFrames = data.numSamples;
	const auto numInputChannels = inBus.numChannels;
	const auto numOutputChannels = outBus.numChannels;

	if (numInputChannels < channels_)
		return kInvalidArgument;

	if (!binauralSurroundRenderer_)
		if (!initBinauralSurroundRenderer(framesPerBuffer_, channels_, sampleRateHz_))
			return kNotInitialized;

	// Directly use the channelBuffers32 arrays.
	binauralSurroundRenderer_->AddPlanarInput(inBus.channelBuffers32, channels_, sampleFrames);
	binauralSurroundRenderer_->GetPlanarStereoOutput(outBus.channelBuffers32, sampleFrames);

	return kResultOk;
}

// -----------------------------------------------------------------------------
// Handle parameter changes
// -----------------------------------------------------------------------------
void BinauralRendererVst::handleParameterChanges(IParameterChanges* changes)
{
	if (!changes) return;

	const auto numParamsChanged = changes->getParameterCount();
	for (auto i = 0; i < numParamsChanged; ++i)
	{
		auto* paramQueue = changes->getParameterData(i);
		if (!paramQueue) continue;

		const auto pid = paramQueue->getParameterId();
		const auto numPoints = paramQueue->getPointCount();
		if (numPoints <= 0) continue;

		ParamValue value = 0.0;
		int32 sampleOffset = 0;
		if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) != kResultTrue)
			continue;

		switch (pid)
		{
			// --- Input Channels ---
		case kParamNumInputChannels:
			channels_ = static_cast<int>(Param_NumInputChannels->toPlain(value));
			break;

		default:
			break;
		}
	}
}

// -----------------------------------------------------------------------------
// Get Component State (save to stream)
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVst::getState(IBStream* state) {
	if (!state)
		return kResultFalse;

	IBStreamer streamer(state, kLittleEndian);

	// --- Channels ---
	if (!streamer.writeInt16(Param_NumInputChannels->toNormalized(channels_)))
		return kResultFalse;

	return kResultOk;
}

// -----------------------------------------------------------------------------
// Load Component State (restore from stream)
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVst::setState(IBStream* state) {
	if (!state)
		return kResultFalse;

	IBStreamer streamer(state, kLittleEndian);

	// --- Channels ---
	int16 savedChannels;
	if (!streamer.readInt16(savedChannels)) return kResultFalse;
	channels_ = savedChannels;

	return kResultOk;
}

// -----------------------------------------------------------------------------
// ResonanceAudio setup
// -----------------------------------------------------------------------------
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
	sampleRateHz_ = sampleRateHz;
	return true;
}
