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

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "binaural_renderer_vst.h"

#include <cstdlib>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "base/constants_and_types.h"

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace vraudio;

// -----------------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------------
BinauralRendererVst::BinauralRendererVst() {
  // nothing heavy here; buses are added in initialize()
  setControllerClass(FUID());  // controller created separately if you add one.
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
tresult PLUGIN_API BinauralRendererVst::setBusArrangements(SpeakerArrangement* inputs,
                                                           int32 numIns,
                                                           SpeakerArrangement* outputs,
                                                           int32 numOuts) {
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
// process
// Build planar float** arrays from VST3 buffers and call the original
// processReplacing(...) routine.
// -----------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererVst::process(ProcessData& data) {
    // Nothing to do if there are no audio buses or no samples.
    if (data.numInputs == 0 || data.numOutputs == 0 || data.numSamples <= 0) {
        return kResultOk;
    }

    // Only handle first input and first output bus in this implementation.
    AudioBusBuffers& inBus = data.inputs[0];
    AudioBusBuffers& outBus = data.outputs[0];

    const int32 sampleRate = processSetup.sampleRate;
    const int32 sampleFrames = data.numSamples;
    const int32 numInputChannels = inBus.numChannels;
    const int32 numOutputChannels = outBus.numChannels;

    if (numInputChannels == 0 || numOutputChannels == 0) {
        return kResultOk;
    }

    // Prepare temporary zero buffers in case the host leaves a channel pointer null.
    // Use vectors sized to sampleFrames so pointer stability is guaranteed.
    static std::vector<float> s_zero_in;
    static std::vector<float> s_zero_out;
    if (static_cast<int>(s_zero_in.size()) < sampleFrames) s_zero_in.assign(sampleFrames, 0.0f);
    if (static_cast<int>(s_zero_out.size()) < sampleFrames) s_zero_out.assign(sampleFrames, 0.0f);

    // Build input pointers array (planar)
    std::vector<float*> inPtrs;
    inPtrs.resize(static_cast<size_t>(numInputChannels));
    for (int32 ch = 0; ch < numInputChannels; ++ch) {
        float* ptr = nullptr;
        if (inBus.channelBuffers32 && inBus.channelBuffers32[ch]) {
            ptr = inBus.channelBuffers32[ch];
        }
        else {
            ptr = s_zero_in.data();
}
        inPtrs[ch] = ptr;
    }

    // Build output pointers array (planar)
    std::vector<float*> outPtrs;
    outPtrs.resize(static_cast<size_t>(numOutputChannels));
    for (int32 ch = 0; ch < numOutputChannels; ++ch) {
        float* ptr = nullptr;
        if (outBus.channelBuffers32 && outBus.channelBuffers32[ch]) {
            ptr = outBus.channelBuffers32[ch];
        }
        else {
            ptr = s_zero_out.data();
        }
        outPtrs[ch] = ptr;
    }

    // Call the original VST2-style routine that expects planar float** arrays.
    processReplacing(numInputChannels, inPtrs.data(), 
                     numOutputChannels, outPtrs.data(), 
                     sampleFrames, sampleRate);

    // If the host gave us more output channels than processReplacing used (it should
    // zero remaining channels itself), ensure any extra channels are cleared to be safe.
    // (processReplacing in your code clears remaining channels beyond stereo, but we
    // keep this safety step just in case.)
    if (numOutputChannels > static_cast<int32>(kNumStereoChannels)) {
        for (int32 ch = static_cast<int32>(kNumStereoChannels); ch < numOutputChannels; ++ch) {
            if (outBus.channelBuffers32 && outBus.channelBuffers32[ch]) {
                std::fill_n(outBus.channelBuffers32[ch], sampleFrames, 0.0f);
            }
        }
    }

    return kResultOk;
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
void BinauralRendererVst::processReplacing(int32 numInputChannels, float** inputs,
                                           int32 numOutputChannels, float** outputs,
                                           int32 sampleFrames, int32 sampleRate) {
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
