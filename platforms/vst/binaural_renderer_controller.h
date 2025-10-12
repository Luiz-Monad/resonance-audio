/*
Copyright 2025 Luiz Felipe Stangarlin

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

#ifndef RESONANCE_AUDIO_PLATFORM_VST3_BINAURAL_RENDERER_CONTROLLER_H_
#define RESONANCE_AUDIO_PLATFORM_VST3_BINAURAL_RENDERER_CONTROLLER_H_

#include <memory>

// VST3 SDK
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/vstgui.h"
#include "base/source/fstring.h"

namespace Steinberg {
namespace Vst {

// -----------------------------------------------------------------------------
// Controller UID
// -----------------------------------------------------------------------------
static const FUID BinauralRendererControllerUID(0x3daf4aa0, 0xe51045d2, 0x99c8521d, 0xc3863f7f);

// -----------------------------------------------------------------------------
// Parameter IDs
// -----------------------------------------------------------------------------
enum BinauralParams : ParamID {
	// XYZ Coordinates
	kParamXPos = 0,
	kParamYPos = 1,
	kParamZPos = 2,

	// Spherical Coordinates
	kParamAzimuth = 3,
	kParamAltitude = 4,
	kParamDistance = 5,

	kNumParams
};

//-----------------------------------------------------------------------------
// BinauralRendererVstController
//-----------------------------------------------------------------------------
// This class implements the VST3 controller side of the plugin.
//-----------------------------------------------------------------------------
class BinauralRendererVstController : public EditController
{
public:
	BinauralRendererVstController() = default;
	~BinauralRendererVstController() override = default;

	// -------------------------------------------------------------------------
	// IEditController overrides
	// -------------------------------------------------------------------------
	tresult PLUGIN_API initialize(FUnknown* context) override;
	tresult PLUGIN_API terminate() override;

	tresult PLUGIN_API setComponentState(IBStream* state) override;

private:

	// Prevent recursive updates
	bool updatingParams_ = false;
};


}
} // namespace Steinberg::Vst

#endif  // RESONANCE_AUDIO_PLATFORM_VST3_BINAURAL_RENDERER_CONTROLLER_H_
