//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXStats.h"

DEFINE_STAT(STAT_PopcornFX_ParticleCount);
DEFINE_STAT(STAT_PopcornFX_ParticleCount_CPU);

#if (PK_GPU_D3D11 == 1)
	DEFINE_STAT(STAT_PopcornFX_ParticleCount_GPU_D3D11);
#endif // (PK_GPU_D3D11 == 1)
#if (PK_GPU_D3D12 == 1)
	DEFINE_STAT(STAT_PopcornFX_ParticleCount_GPU_D3D12);
#endif // (PK_GPU_D3D12 == 1)

DEFINE_STAT(STAT_PopcornFX_NewParticleCount);
DEFINE_STAT(STAT_PopcornFX_DeadParticleCount);
DEFINE_STAT(STAT_PopcornFX_DrawCallCount);
DEFINE_STAT(STAT_PopcornFX_EmitterUpdateCount);
DEFINE_STAT(STAT_PopcornFX_SoundParticleCount);
DEFINE_STAT(STAT_PopcornFX_DrawRequestsCount);
DEFINE_STAT(STAT_PopcornFX_DrawCallsCount);
DEFINE_STAT(STAT_PopcornFX_RayTracing_DrawCallsCount);
DEFINE_STAT(STAT_PopcornFX_BatchesCount);

DEFINE_STAT(STAT_PopcornFX_MediumCount);

DEFINE_STAT(STAT_PopcornFX_BroadcastedEventsCount);

DEFINE_STAT(STAT_PopcornFX_PopcornFXUpdateTime);
DEFINE_STAT(STAT_PopcornFX_PreUpdateFenceTime);
DEFINE_STAT(STAT_PopcornFX_UpdateEmittersTime);
DEFINE_STAT(STAT_PopcornFX_StartUpdateTime);
DEFINE_STAT(STAT_PopcornFX_UpdateSoundsTime);
DEFINE_STAT(STAT_PopcornFX_ComputeAudioSpectrumTime);
DEFINE_STAT(STAT_PopcornFX_ComputeAudioWaveformTime);
DEFINE_STAT(STAT_PopcornFX_UpdateBoundsTime);

DEFINE_STAT(STAT_PopcornFX_GDMETime);
DEFINE_STAT(STAT_PopcornFX_GDRTITime);
DEFINE_STAT(STAT_PopcornFX_SRDDTime);
DEFINE_STAT(STAT_PopcornFX_GCPoolsTime);
DEFINE_STAT(STAT_PopcornFX_GatherSimpleLightsTime);

DEFINE_STAT(STAT_PopcornFX_ParticleStartRenderTime);
DEFINE_STAT(STAT_PopcornFX_ParticleKickRenderTime);

DEFINE_STAT(STAT_PopcornFX_ViewCount);
DEFINE_STAT(STAT_PopcornFX_CulledPagesCount);
DEFINE_STAT(STAT_PopcornFX_CulledDrawReqCount);

DEFINE_STAT(STAT_PopcornFX_SkinningWaitTime);
DEFINE_STAT(STAT_PopcornFX_FetchClothData);
