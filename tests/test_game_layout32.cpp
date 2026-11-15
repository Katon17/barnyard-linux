// Compile-only guard: keep retail size assertions, add explicit member offsets.
#include <pch.h>
#include "HUD/AStartMiniGameHudElement.h"
#include "HUD/ASimPlayerPickUpHudElement.h"
#include "MiniGame/AMiniGameState.h"
#include "Units/APrecisionCoyote.h"
#include "Units/AUnitFacialComponent.h"
#include "Units/ASoundComponent.h"
#include <cstddef>
#include "Cameras/ATrackingCameraHelper.h"
#include "FX/ATrackBoneAniEffect.h"
#include "FX/AHudTrackBoneAniEffect.h"
#include "FX/ASwirlTrackBoneAniEffect.h"
#include "Physics/ABarnyardCollision.h"
static_assert(sizeof(void*)==4 && sizeof(TWCHAR)==2);
static_assert(sizeof(AHudElement)==0x14);
static_assert(offsetof(AStartMiniGameHudElement,m_bFinished)==0x14);
static_assert(offsetof(AStartMiniGameHudElement,m_oImage)==0x20);
static_assert(offsetof(AStartMiniGameHudElement,m_wszText)==0x18c);
static_assert(offsetof(ASimPlayerPickUpHudElement,m_bUnk1)==0x14);
static_assert(offsetof(ASimPlayerPickUpHudElement,m_oBacking)==0x18);
static_assert(offsetof(APrecisionCoyote,m_bMilked)==0xa0);
static_assert(offsetof(APrecisionCoyote,m_aRespawns)==0xc4);

// Newly audited base-tail occupancy and collision multiple-inheritance sizes.
static_assert(sizeof(ACameraHelper)==0x10);
static_assert(sizeof(ATrackingCameraHelper)==0x2c);
static_assert(sizeof(AAniEffect)==0x0c);
static_assert(sizeof(ATrackBoneAniEffect)==0x54);
static_assert(sizeof(AHudTrackBoneAniEffect)==0x54);
static_assert(sizeof(ASwirlTrackBoneAniEffect)==0x38);
static_assert(sizeof(ABarnyardCollision)==0x0c);
struct CameraTailProbe : ACameraHelper {
    static void audit() { static_assert(offsetof(CameraTailProbe,m_bUnk)==0x0c); }
};
static_assert(sizeof(CameraTailProbe)==0x10);
