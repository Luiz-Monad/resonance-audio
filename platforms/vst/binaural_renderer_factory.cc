#include "public.sdk/source/main/pluginfactory.h"
#include "binaural_renderer_controller.h"
#include "binaural_renderer_processor.h"
#include "binaural_renderer_gui.h"

namespace Steinberg {
namespace Vst {

static FUnknown* createProcessorInstance(void*) {
  return (IAudioProcessor*)new BinauralRendererVst();
}

static FUnknown* createControllerInstance(void*) {
  return (IEditController*)new BinauralRendererVstController();
}

}  // namespace Vst
}  // namespace Steinberg

using namespace Steinberg::Vst;

BEGIN_FACTORY_DEF("Google", "https://resonanceaudio.com",
                  "mailto:support@resonanceaudio.com")

DEF_CLASS2(INLINE_UID_FROM_FUID(BinauralRendererProcessorUID),
           PClassInfo::kManyInstances, kVstAudioEffectClass,
           "ResonanceAudioMonitor", Vst::kDistributable, "Fx|Spatial", "1.0.0",
           kVstVersionString, Steinberg::Vst::createProcessorInstance)

DEF_CLASS2(INLINE_UID_FROM_FUID(BinauralRendererControllerUID),
           PClassInfo::kManyInstances, kVstComponentControllerClass,
           "ResonanceAudioMonitorController", 0, "Controller", "1.0.0",
           kVstVersionString, Steinberg::Vst::createControllerInstance)

END_FACTORY
