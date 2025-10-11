#include "public.sdk/source/main/pluginfactory.h"
#include "binaural_renderer_vst.h"

namespace Steinberg { namespace Vst {

static const FUID BinauralRendererProcessorUID(0xb9981ce8, 0xc7f1415d, 0x8bad4a8a, 0x6d1cb6e0);

}}

using namespace Steinberg::Vst;

BEGIN_FACTORY_DEF("Google",
                  "https://resonanceaudio.com",
                  "mailto:support@resonanceaudio.com")
    
    DEF_CLASS2(INLINE_UID_FROM_FUID(BinauralRendererProcessorUID),
               PClassInfo::kManyInstances,
               kVstAudioEffectClass,
               "ResonanceAudioMonitor",
               Vst::kDistributable,
               "Fx|Spatial",
               "1.0.0",
               kVstVersionString,
               Steinberg::Vst::BinauralRendererVst::createInstance)

END_FACTORY
