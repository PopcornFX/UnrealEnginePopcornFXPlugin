//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "Stats/Stats.h"

DECLARE_STATS_GROUP(TEXT("PopcornFX"), STATGROUP_PopcornFX, STATCAT_Advanced);

DECLARE_STATS_GROUP(TEXT("PopcornFX Profile"), STATGROUP_PopcornFX_Profile, STATCAT_Advanced);

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Particles (total)"), STAT_PopcornFX_ParticleCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Particles (CPU)"), STAT_PopcornFX_ParticleCount_CPU, STATGROUP_PopcornFX, );

#if (PK_GPU_D3D11 == 1)
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Particles (GPU) - D3D11"), STAT_PopcornFX_ParticleCount_GPU_D3D11, STATGROUP_PopcornFX, );
#endif // (PK_GPU_D3D11 == 1)
#if (PK_GPU_D3D12 == 1)
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Particles (GPU) - D3D12"), STAT_PopcornFX_ParticleCount_GPU_D3D12, STATGROUP_PopcornFX, );
#endif // (PK_GPU_D3D12 == 1)

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Particles born"), STAT_PopcornFX_NewParticleCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Particles dead"), STAT_PopcornFX_DeadParticleCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Draw calls"), STAT_PopcornFX_DrawCallCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Raytraced draw calls"), STAT_PopcornFX_RayTracing_DrawCallsCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Emitters updated"), STAT_PopcornFX_EmitterUpdateCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Sound particles"), STAT_PopcornFX_SoundParticleCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Render: DrawRequests"), STAT_PopcornFX_DrawRequestsCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Render: DrawCalls"), STAT_PopcornFX_DrawCallsCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Render: DrawCalls (Billboard)"), STAT_PopcornFX_DrawCallsBillboardCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Render: DrawCalls (Ribbon)"), STAT_PopcornFX_DrawCallsRibbonCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Render: DrawCalls (Triangle)"), STAT_PopcornFX_DrawCallsTriangleCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Render: DrawCalls (Mesh)"), STAT_PopcornFX_DrawCallsMeshCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Render: DrawCalls (Skeletal Mesh)"), STAT_PopcornFX_DrawCallsSkelMeshCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Render: Lights"), STAT_PopcornFX_LightCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Render: Sounds"), STAT_PopcornFX_SoundCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Render: Batches"), STAT_PopcornFX_BatchesCount, STATGROUP_PopcornFX, );

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("PopcornFX Mediums"), STAT_PopcornFX_MediumCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("PopcornFX Active Mediums"), STAT_PopcornFX_ActiveMediumCount, STATGROUP_PopcornFX, );

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Broadcasted events"), STAT_PopcornFX_BroadcastedEventsCount, STATGROUP_PopcornFX, );

DECLARE_CYCLE_STAT_EXTERN(TEXT("Update: PopcornFX update time"), STAT_PopcornFX_PopcornFXUpdateTime, STATGROUP_PopcornFX, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Update1: pre-UpdateFence"), STAT_PopcornFX_PreUpdateFenceTime, STATGROUP_PopcornFX, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Update2: Update Emitters"), STAT_PopcornFX_UpdateEmittersTime, STATGROUP_PopcornFX, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Update3: Update"), STAT_PopcornFX_StartUpdateTime, STATGROUP_PopcornFX, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Update4: Sounds"), STAT_PopcornFX_UpdateSoundsTime, STATGROUP_PopcornFX, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Update5: Update Bounds"), STAT_PopcornFX_UpdateBoundsTime, STATGROUP_PopcornFX, );

DECLARE_CYCLE_STAT_EXTERN(TEXT("Compute Audio Spectrum"), STAT_PopcornFX_ComputeAudioSpectrumTime, STATGROUP_PopcornFX, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Compute Audio Waveform"), STAT_PopcornFX_ComputeAudioWaveformTime, STATGROUP_PopcornFX, );

DECLARE_CYCLE_STAT_EXTERN(TEXT("Render: GDME"), STAT_PopcornFX_GDMETime, STATGROUP_PopcornFX, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Render: GDRTI"), STAT_PopcornFX_GDRTITime, STATGROUP_PopcornFX, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Render: GatherSimpleLights"), STAT_PopcornFX_GatherSimpleLightsTime, STATGROUP_PopcornFX, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Render: SRDD"), STAT_PopcornFX_SRDDTime, STATGROUP_PopcornFX, );

DECLARE_CYCLE_STAT_EXTERN(TEXT("Render: Pk GC pools"), STAT_PopcornFX_GCPoolsTime, STATGROUP_PopcornFX, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Render: Pk StartRender"), STAT_PopcornFX_ParticleStartRenderTime, STATGROUP_PopcornFX, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Render: Pk KickRender"), STAT_PopcornFX_ParticleKickRenderTime, STATGROUP_PopcornFX, );

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Render: View count"), STAT_PopcornFX_ViewCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Render: Culled pages count"), STAT_PopcornFX_CulledPagesCount, STATGROUP_PopcornFX, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Render: Culled drawReq count"), STAT_PopcornFX_CulledDrawReqCount, STATGROUP_PopcornFX, );

DECLARE_CYCLE_STAT_EXTERN(TEXT("Samplers: Skinning wait time"), STAT_PopcornFX_SkinningWaitTime, STATGROUP_PopcornFX, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Samplers: Fetch cloth data"), STAT_PopcornFX_FetchClothData, STATGROUP_PopcornFX, );
