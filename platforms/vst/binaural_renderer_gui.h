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

#ifndef RESONANCE_AUDIO_PLATFORM_VST3_BINAURAL_RENDERER_GUI_H_
#define RESONANCE_AUDIO_PLATFORM_VST3_BINAURAL_RENDERER_GUI_H_

#include "pluginterfaces/gui/iplugview.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui4/vstgui/lib/cframe.h"
#include "vstgui4/vstgui/lib/cbitmap.h"
#include "vstgui4/vstgui/lib/iviewlistener.h"
#include "vstgui4/vstgui/lib/vstguibase.h"

namespace Steinberg {
namespace Vst {

/**
 * @brief The VST3 GUI for the Resonance Audio Binaural Renderer.
 *
 * This implements a simple editor view that displays a static image
 * (like the old VST2 version did with "resonance_audio.png").
 */
class BinauralRendererView : public VSTGUI::CView, public EditorView {
 public:
  explicit BinauralRendererView(EditController* controller);
  ~BinauralRendererView() SMTG_OVERRIDE;

  // IPlugView overrides
  tresult PLUGIN_API isPlatformTypeSupported(FIDString type) SMTG_OVERRIDE;
  tresult PLUGIN_API attached(void* parent, FIDString type) SMTG_OVERRIDE;
  tresult PLUGIN_API removed() SMTG_OVERRIDE;
  tresult PLUGIN_API onSize(ViewRect* newSize) SMTG_OVERRIDE;

  // Optional � handle GUI resize or parameter updates
  tresult PLUGIN_API onWheel(float distance) SMTG_OVERRIDE { return kResultOk; }

protected:
  // Internal frame (VSTGUI root container)
  VSTGUI::CFrame* frame_ = nullptr;

  // Background image
  VSTGUI::SharedPointer<VSTGUI::CBitmap> background_;

  // Pointer to the edit controller
  EditController* controller_ = nullptr;
};

}}  // namespace Steinberg::Vst

#endif  // RESONANCE_AUDIO_PLATFORM_VST3_BINAURAL_RENDERER_GUI_H_
