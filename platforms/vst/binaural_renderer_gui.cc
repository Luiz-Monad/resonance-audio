/*
Copyright 2025

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#include "binaural_renderer_gui.h"

#include "pluginterfaces/gui/iplugview.h"
#include "vstgui4/vstgui/lib/cbitmap.h"
#include "vstgui4/vstgui/lib/cframe.h"
#include "vstgui4/vstgui/lib/crect.h"
#include "vstgui4/vstgui/lib/iviewlistener.h"

#include <string>

namespace Steinberg {
namespace Vst {

using namespace VSTGUI;

BinauralRendererView::BinauralRendererView(EditController* controller)
    : CView(CRect()),
      EditorView(controller),
      frame_(nullptr),
      controller_(controller) {}

BinauralRendererView::~BinauralRendererView() {
  if (frame_) {
    frame_->forget();
    frame_ = nullptr;
  }
}

//------------------------------------------------------------------------------
// Check if host platform type is supported
//------------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererView::isPlatformTypeSupported(FIDString type) {
  // Common supported types: kPlatformTypeHWND (Windows), kPlatformTypeNSView (macOS)
  if (strcmp(type, kPlatformTypeHWND) == 0 ||
      strcmp(type, kPlatformTypeNSView) == 0) {
    return kResultTrue;
  }
  return kResultFalse;
}

//------------------------------------------------------------------------------
// Attach to parent window
//------------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererView::attached(void* parent, FIDString type) {
  // Create background image (the same static PNG from VST2)
  CBitmap* bitmap = new CBitmap("resonance_audio.png");
  background_ = VSTGUI::owned(bitmap);

  if (!background_) {
    return kResultFalse;
  }

  // Get size from the image
  const CCoord width = background_->getWidth();
  const CCoord height = background_->getHeight();

  // Define view rectangle
  CRect size(0, 0, width, height);

  // Create a frame and assign it to this view
  frame_ = new CFrame(size, nullptr);
  if (!frame_) {
    return kResultFalse;
  }

  // Attach to the native parent (DAW window)
  frame_->open(parent);
  frame_->setBackground(background_);
  frame_->setZoom(1.0);
  frame_->forget();

  // Inform host of size
  ViewRect viewRect(0, 0, static_cast<int32>(width), static_cast<int32>(height));
  setRect(viewRect);

  return kResultOk;
}

//------------------------------------------------------------------------------
// Called when GUI is detached or host closes it
//------------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererView::removed() {
  if (frame_) {
    frame_->forget();
    frame_ = nullptr;
  }
  background_ = nullptr;
  return kResultOk;
}

//------------------------------------------------------------------------------
// Handle window resizing from host
//------------------------------------------------------------------------------
tresult PLUGIN_API BinauralRendererView::onSize(ViewRect* newSize) {
  if (frame_) {
    frame_->setViewSize(CRect(0, 0,
                              static_cast<CCoord>(newSize->right - newSize->left),
                              static_cast<CCoord>(newSize->bottom - newSize->top)));
  }
  return kResultOk;
}

}}  // namespace Steinberg::Vst
