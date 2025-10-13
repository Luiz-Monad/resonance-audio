/*
Copyright 2018 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef RESONANCE_AUDIO_PLATFORM_VST3_BINAURAL_RENDERER_PROCESSOR_H_
#define RESONANCE_AUDIO_PLATFORM_VST3_BINAURAL_RENDERER_PROCESSOR_H_

#include <memory>

#include "binaural_renderer_controller.h"

// Resonance API
#include "api/binaural_surround_renderer.h"

// VST3 SDK
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vstrepresentation.h"

namespace Steinberg {
namespace Vst {

// -----------------------------------------------------------------------------
// Processor UID
// -----------------------------------------------------------------------------
static const FUID BinauralRendererProcessorUID(0xb9981ce8, 0xc7f1415d, 0x8bad4a8a, 0x6d1cb6e0);

//-----------------------------------------------------------------------------
// BinauralRendererVst
//-----------------------------------------------------------------------------
// This class implements the VST3 processor (audio engine) side of the plugin.
//-----------------------------------------------------------------------------
class BinauralRendererVst : public AudioEffect {
public:
	BinauralRendererVst();
	~BinauralRendererVst() SMTG_OVERRIDE;

	//--- AudioEffect overrides ---
	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate() SMTG_OVERRIDE;

	tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs,
		int32 numIns,
		SpeakerArrangement* outputs,
		int32 numOuts) SMTG_OVERRIDE;
	tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) SMTG_OVERRIDE;
	tresult PLUGIN_API setActive(TBool state) SMTG_OVERRIDE;

	tresult PLUGIN_API setupProcessing(ProcessSetup& setup) SMTG_OVERRIDE;
	tresult PLUGIN_API process(ProcessData& data) SMTG_OVERRIDE;

	tresult PLUGIN_API setState(IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API getState(IBStream* state) SMTG_OVERRIDE;

protected:

	// Parameter handling.
	void handleParameterChanges(IParameterChanges* changes);

};

}} // namespace Steinberg::Vst

#endif  // RESONANCE_AUDIO_PLATFORM_VST3_BINAURAL_RENDERER_PROCESSOR_H_
