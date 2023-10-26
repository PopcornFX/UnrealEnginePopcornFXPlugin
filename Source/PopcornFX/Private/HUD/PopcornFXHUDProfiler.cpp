//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXHUDProfiler.h"

#include "PopcornFXPlugin.h"
#include "Internal/ParticleScene.h"

#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Debug/DebugDrawService.h"
#include "Engine/Engine.h"
#include "UObject/UObjectIterator.h"

#include "PopcornFXSceneComponent.h"

#include "PopcornFXSDK.h"
#include <pk_kernel/include/kr_sort.h>

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "LogPopcornFXProfilerHUD"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXProfilerHUD, Log, All);

#if	(PK_PARTICLES_HAS_STATS != 0)
bool	operator==(const CParticleScene::SPopcornFXEffectTimings &other, const PopcornFX::CParticleEffect *effect);
#endif // (PK_PARTICLES_HAS_STATS != 0)

//----------------------------------------------------------------------------
//
//	APopcornFXHUDProfiler
//
//----------------------------------------------------------------------------

namespace
{
#if	(PK_PARTICLES_HAS_STATS != 0)
	class	CEffectTimingsSorter_SimulationCost
	{
	public:
		typedef	PopcornFX::TArray<CParticleScene::SPopcornFXEffectTimings>::Iterator	_TypeIt;
		PK_FORCEINLINE static bool	Less(const _TypeIt &it0, const _TypeIt &it1) { return it0->TotalTime() > it1->TotalTime(); }
		PK_FORCEINLINE static bool	Equal(const _TypeIt &it0, const _TypeIt &it1) { return it0->TotalTime() == it1->TotalTime(); }
	};

	class	CEffectTimingsSorter_ParticleCount
	{
	public:
		typedef	PopcornFX::TArray<CParticleScene::SPopcornFXEffectTimings>::Iterator	_TypeIt;
		PK_FORCEINLINE static bool	Less(const _TypeIt &it0, const _TypeIt &it1) { return it0->TotalParticleCount() > it1->TotalParticleCount(); }
		PK_FORCEINLINE static bool	Equal(const _TypeIt &it0, const _TypeIt &it1) { return it0->TotalParticleCount() == it1->TotalParticleCount(); }
	};

	class	CEffectTimingsSorter_InstanceCount
	{
	public:
		typedef	PopcornFX::TArray<CParticleScene::SPopcornFXEffectTimings>::Iterator	_TypeIt;
		PK_FORCEINLINE static bool	Less(const _TypeIt &it0, const _TypeIt &it1) { return it0->TotalInstanceCount() > it1->TotalInstanceCount(); }
		PK_FORCEINLINE static bool	Equal(const _TypeIt &it0, const _TypeIt &it1) { return it0->TotalInstanceCount() == it1->TotalInstanceCount(); }
	};
#endif // (PK_PARTICLES_HAS_STATS != 0)
}

APopcornFXHUDProfiler::APopcornFXHUDProfiler(const FObjectInitializer &objectInitializer)
:	Super(objectInitializer)
{
	SetHidden(false);
	bShowHUD = true;
}

//----------------------------------------------------------------------------

APopcornFXHUDProfiler::~APopcornFXHUDProfiler()
{
}

//----------------------------------------------------------------------------

void	APopcornFXHUDProfiler::PostActorCreated()
{
	FPopcornFXPlugin::Get().ActivateEffectsProfiler(true);
	Super::PostActorCreated();
}

//----------------------------------------------------------------------------

void	APopcornFXHUDProfiler::Destroyed()
{
	FPopcornFXPlugin::Get().ActivateEffectsProfiler(false);
	Super::Destroyed();
}

//----------------------------------------------------------------------------

void	APopcornFXHUDProfiler::DrawBar(float minX, float maxX, float yPos, float cursor, float thickness)
{
	const float		clampedCursor = FMath::Clamp(cursor, 0.0f, 1.0f);
	const float		sizeX = (maxX - minX) * clampedCursor;
	FLinearColor	color(clampedCursor, 1.f - clampedCursor, 0.f, 0.8f);
	{
		FCanvasTileItem	tile(FVector2D(minX, yPos), FVector2D(sizeX, thickness), color);
		tile.BlendMode = SE_BLEND_AlphaBlend;
		Canvas->DrawItem(tile);
	}
	{
		color.A = 0.1f;
		FCanvasTileItem	tile(FVector2D(minX + sizeX, yPos), FVector2D((maxX - minX) - sizeX, thickness), color);
		tile.BlendMode = SE_BLEND_AlphaBlend;
		Canvas->DrawItem(tile);
	}
}

//----------------------------------------------------------------------------

void	APopcornFXHUDProfiler::DrawDebugHUD(UCanvas* inCanvas, APlayerController* playerController)
{
#if	(PK_PARTICLES_HAS_STATS != 0)
	PK_NAMEDSCOPEDPROFILE_C("APopcornFXHUDProfiler", POPCORNFX_UE_PROFILER_COLOR);

	Canvas = inCanvas;
	if (!Canvas)
		return;

	UWorld	*ownerWorld = GetWorld();
	if (!PK_VERIFY(ownerWorld != null)) // if for some reason the actor doesn't have a valid world
		return;

	// Grab all medium collections from the world
	PopcornFX::TArray<CParticleScene::SPopcornFXEffectTimings>	allSceneTimings;
	double														wallUpdateTimeCPU = 0.0;
	u32															totalParticleCount_CPU = 0;
	u32															totalParticleCount_GPU = 0;
	u32															totalInstanceCount = 0;
	for (TObjectIterator<UPopcornFXSceneComponent> sceneIt; sceneIt; ++sceneIt)
	{
		UPopcornFXSceneComponent	*sceneComp = *sceneIt;
		if (sceneComp->GetWorld() != ownerWorld)
			continue;
		if (sceneComp->ParticleScene() == null)
			continue;
		allSceneTimings.Merge(sceneComp->ParticleScene()->EffectTimings());
		wallUpdateTimeCPU += sceneComp->ParticleScene()->MediumCollectionUpdateTime();
		totalParticleCount_CPU += sceneComp->ParticleScene()->MediumCollectionParticleCount_CPU();
		totalParticleCount_GPU += sceneComp->ParticleScene()->MediumCollectionParticleCount_GPU();
		totalInstanceCount += sceneComp->ParticleScene()->MediumCollectionInstanceCount();
	}
	for (u32 iTiming = 0; iTiming < allSceneTimings.Count(); ++iTiming)
	{
		CParticleScene::SPopcornFXEffectTimings	&effectTimingRef = allSceneTimings[iTiming];

		PopcornFX::CGuid	effectRef = allSceneTimings.IndexOfLast(effectTimingRef.m_Effect);
		while (effectRef.Valid() && effectRef != iTiming)
		{
			effectTimingRef.Merge(allSceneTimings[effectRef]);
			allSceneTimings.Remove_AndKeepOrder(effectRef);

			effectRef = allSceneTimings.IndexOfLast(effectTimingRef.m_Effect);
		}
	}

	if (allSceneTimings.Empty())
		return; // Nothing to draw

	const UPopcornFXSettings	*settings = FPopcornFXPlugin::Get().Settings();
	check(settings);
	const float										screenRatio = settings->HUD_DisplayScreenRatio;
	const float										hideBelowPerc = settings->HUD_HideNodesBelowPercent;
	const u32										frameCount = settings->HUD_UpdateTimeFrameCount;
	const EPopcornFXEffectsProfilerSortMode::Type	sortMode = settings->EffectsProfilerSortMode;

	// Sum flat times:
	double	totalTime = 0.0;
	double	totalTimeCPU = 0.0;
	double	totalTimeGPU = 0.0;
	for (u32 iTiming = 0; iTiming < allSceneTimings.Count(); ++iTiming)
	{
		totalTime += allSceneTimings[iTiming].TotalTime() / frameCount;
		totalTimeCPU += allSceneTimings[iTiming].TotalTimeCPU() / frameCount;
		totalTimeGPU += allSceneTimings[iTiming].TotalTimeGPU() / frameCount; // Note: not this frame's gpu time, 2-4 frames delay
	}
	// Always display gpu flat time, we won't extract hardware specs here.
	// This normalizes effect flat time against CPU wall time (time between mediumCollection->Update() and mediumCollection->UpdateFence())
	const float	timeNormalizerCPU = wallUpdateTimeCPU / totalTimeCPU;

	switch (sortMode)
	{
	case	EPopcornFXEffectsProfilerSortMode::SimulationCost:
		PopcornFX::QuickSort<CEffectTimingsSorter_SimulationCost>(allSceneTimings.Begin(), allSceneTimings.End());
		break;
	case	EPopcornFXEffectsProfilerSortMode::ParticleCount:
		PopcornFX::QuickSort<CEffectTimingsSorter_ParticleCount>(allSceneTimings.Begin(), allSceneTimings.End());
		break;
	case	EPopcornFXEffectsProfilerSortMode::InstanceCount:
		PopcornFX::QuickSort<CEffectTimingsSorter_InstanceCount>(allSceneTimings.Begin(), allSceneTimings.End());
	default:
		PK_ASSERT_NOT_REACHED();
		break;
	}

	Canvas->SetDrawColor(255, 255, 255, 255);

	const UFont				*font = GEngine->GetSmallFont();
	const FFontRenderInfo	fri = Canvas->CreateFontRenderInfo(true, true);
	const FFontRenderInfo	friNoShadow = Canvas->CreateFontRenderInfo(true, false);

	const FString	title = TEXT("PopcornFX Effects Profiler");
	float			titleWidth;
	float			lineHeight;
	Canvas->StrLen(font, title, titleWidth, lineHeight);

	float			yPos = Canvas->SizeY * screenRatio;
	float			xPos = yPos;
	const float		maxyPos = Canvas->SizeY * (1.f - screenRatio);

	Canvas->DrawText(font, title, xPos, yPos, 1.f, 1.f, fri);
	yPos += lineHeight;

	const bool		hasGPU = totalTimeGPU > 0.0f;

	const float		relTimexPos = xPos + 0;
	const float		relTimeCPUxPos = relTimexPos + 60;
	const float		relTimeGPUxPos = relTimeCPUxPos + 60;
	const float		timexPos = (hasGPU ? relTimeGPUxPos : relTimexPos) + 60;
	const float		timeCPUxPos = timexPos + 60;
	const float		timeGPUxPos = timeCPUxPos + 60;
	const float		pCountCPUxPos = (hasGPU ? timeGPUxPos : timexPos) + 60;
	const float		pCountGPUxPos = pCountCPUxPos + 70;
	const float		iCountxPos = (hasGPU ? pCountGPUxPos : pCountCPUxPos) + 70;
	const float		effectPathxPos = iCountxPos + 60;

	const float		TimeLimitPerEffectSeconds = settings->TimeLimitPerEffect * 0.001f;
	const float		CPUTimeLimitPerEffectSeconds = settings->CPUTimeLimitPerEffect * 0.001f;
	const float		GPUTimeLimitPerEffectSeconds = settings->GPUTimeLimitPerEffect * 0.001f;
	const float		TimeLimitTotalSeconds = settings->TimeLimitTotal * 0.001f;
	const float		CPUTimeLimitTotalSeconds = settings->CPUTimeLimitTotal * 0.001f;
	const float		GPUTimeLimitTotalSeconds = settings->GPUTimeLimitTotal * 0.001f;

	const u32		CPUParticleCountLimitPerEffect = settings->CPUParticleCountLimitPerEffect;
	const u32		GPUParticleCountLimitPerEffect = settings->GPUParticleCountLimitPerEffect;

	const u32		CPUParticleCountLimitTotal = settings->CPUParticleCountLimitTotal;
	const u32		GPUParticleCountLimitTotal = settings->GPUParticleCountLimitTotal;

	// Draw summary

	Canvas->DrawText(font, FString::Printf(TEXT("Timings are averaged over %d frames"), frameCount), relTimexPos, yPos, 1.f, 1.f, fri);
	yPos += lineHeight;

	Canvas->DrawText(font, FString::Printf(TEXT("Total time")), timexPos, yPos, 1.f, 1.f, fri);
	Canvas->DrawText(font, FString::Printf(TEXT("CPU count")), pCountCPUxPos, yPos, 1.f, 1.f, fri);
	Canvas->DrawText(font, FString::Printf(TEXT("Instances")), iCountxPos, yPos, 1.f, 1.f, fri);
	if (hasGPU)
	{
		Canvas->DrawText(font, FString::Printf(TEXT("Total CPU")), timeCPUxPos, yPos, 1.f, 1.f, fri);
		Canvas->DrawText(font, FString::Printf(TEXT("Total GPU")), timeGPUxPos, yPos, 1.f, 1.f, fri);
		Canvas->DrawText(font, FString::Printf(TEXT("GPU count")), pCountGPUxPos, yPos, 1.f, 1.f, fri);
	}
	yPos += lineHeight;

	// Timings
	{
		const PopcornFX::Units::SValueAndNamedUnit	readableTotalTime = PopcornFX::Units::AutoscaleTime(wallUpdateTimeCPU, 0.5f);
		const PopcornFX::Units::SValueAndNamedUnit	readableTotalTimeCPU = PopcornFX::Units::AutoscaleTime(wallUpdateTimeCPU, 0.5f);
		//const PopcornFX::Units::SValueAndNamedUnit	readableTotalTimeGPU = PopcornFX::Units::AutoscaleTime(totalUpdateTimeGPU, 0.5f);
		DrawBar(timexPos, timeCPUxPos - 1, yPos, wallUpdateTimeCPU / TimeLimitTotalSeconds, lineHeight);
		Canvas->DrawText(font, FString::Printf(TEXT("%.1f %s"), readableTotalTime.m_Value, ANSI_TO_TCHAR(readableTotalTime.m_UnitName)), timexPos, yPos, 1.f, 1.f, friNoShadow);
		if (hasGPU)
		{
			DrawBar(timeCPUxPos, timeGPUxPos - 1, yPos, wallUpdateTimeCPU / CPUTimeLimitTotalSeconds, lineHeight);
			Canvas->DrawText(font, FString::Printf(TEXT("%.1f %s"), readableTotalTimeCPU.m_Value, ANSI_TO_TCHAR(readableTotalTimeCPU.m_UnitName)), timeCPUxPos, yPos, 1.f, 1.f, friNoShadow);
			//DrawBar(timeGPUxPos, pCountCPUxPos - 1, yPos, totalUpdateTimeGPU / GPUTimeLimitTotalSeconds, lineHeight);
			//Canvas->DrawText(font, FString::Printf(TEXT("%.1f %s"), readableTotalTimeGPU.m_Value, ANSI_TO_TCHAR(readableTotalTimeGPU.m_UnitName)), timeGPUxPos, yPos, 1.f, 1.f, friNoShadow);
		}
	}
	// Counts
	{
		DrawBar(pCountCPUxPos, pCountGPUxPos - 1, yPos, (float)totalParticleCount_CPU / (float)CPUParticleCountLimitTotal, lineHeight);
		Canvas->DrawText(font, FString::Printf(TEXT("%d"), totalParticleCount_CPU), pCountCPUxPos, yPos, 1.f, 1.f, fri);
		if (hasGPU)
		{
			DrawBar(pCountGPUxPos, iCountxPos - 1, yPos, (float)totalParticleCount_GPU / (float)GPUParticleCountLimitTotal, lineHeight);
			Canvas->DrawText(font, FString::Printf(TEXT("%d"), totalParticleCount_GPU), pCountGPUxPos, yPos, 1.f, 1.f, fri);
		}

		Canvas->DrawText(font, FString::Printf(TEXT("%d"), totalInstanceCount), iCountxPos, yPos, 1.f, 1.f, fri);
	}
	yPos += lineHeight;

	if (totalTime > 0.0f && !allSceneTimings.Empty())
	{
		Canvas->DrawText(font, TEXT("%"), relTimexPos, yPos, 1.f, 1.f, fri);
		if (hasGPU)
		{
			Canvas->DrawText(font, TEXT("%CPU"), relTimeCPUxPos, yPos, 1.f, 1.f, fri);
			Canvas->DrawText(font, TEXT("%GPU"), relTimeGPUxPos, yPos, 1.f, 1.f, fri);
			Canvas->DrawText(font, TEXT("Est. CPU"), timeCPUxPos, yPos, 1.f, 1.f, fri);
			Canvas->DrawText(font, TEXT("Est. GPU"), timeGPUxPos, yPos, 1.f, 1.f, fri);
			Canvas->DrawText(font, TEXT("GPU count"), pCountGPUxPos, yPos, 1.f, 1.f, fri);
		}
		Canvas->DrawText(font, TEXT("Est. time"), timexPos, yPos, 1.f, 1.f, fri);
		Canvas->DrawText(font, TEXT("CPU count"), pCountCPUxPos, yPos, 1.f, 1.f, fri);
		Canvas->DrawText(font, TEXT("Instances"), iCountxPos, yPos, 1.f, 1.f, fri);
		Canvas->DrawText(font, TEXT("Effect path"), effectPathxPos, yPos, 1.f, 1.f, fri);
		yPos += lineHeight;

		const float			maxDrawyPos = maxyPos - lineHeight * 2; // 2 last total lines
		double				displayedTotalTime = 0.0f;
		double				displayedTotalTimeCPU = 0.0f;
		double				displayedTotalTimeGPU = 0.0f;
		u32					displayedTotalPCount_CPU = 0;
		u32					displayedTotalPCount_GPU = 0;
		u32					displayedTotalICount = 0;
		for (u32 iTiming = 0; iTiming < allSceneTimings.Count(); ++iTiming)
		{
			const float	effectTime = allSceneTimings[iTiming].TotalTime() / frameCount;
			const float	effectTimeCPU = allSceneTimings[iTiming].TotalTimeCPU() / frameCount;
			const float	effectTimeGPU = allSceneTimings[iTiming].TotalTimeGPU() / frameCount;
			const u32	effectCPUPCount = allSceneTimings[iTiming].TotalParticleCount_CPU() / frameCount;
			const u32	effectGPUPCount = allSceneTimings[iTiming].TotalParticleCount_GPU() / frameCount;
			const u32	effectInstanceCount = allSceneTimings[iTiming].TotalInstanceCount()/* / frameCount */; // Not averaged
			const float	effectTimeRelative = (effectTime / totalTime) * 100.0f;
			const float	effectTimeRelativeCPU = totalTimeCPU > 0.0 ? (effectTimeCPU / totalTimeCPU) * 100.0f : 0.0f;
			const float	effectTimeRelativeGPU = totalTimeGPU > 0.0 ? (effectTimeGPU / totalTimeGPU) * 100.0f : 0.0f;

			if (effectTimeRelative < hideBelowPerc)
			{
				if (sortMode != EPopcornFXEffectsProfilerSortMode::SimulationCost)
					continue;
				break; // We know all following will be lower
			}
			displayedTotalTime += effectTime;
			displayedTotalTimeCPU += effectTimeCPU;
			displayedTotalTimeGPU += effectTimeGPU;
			displayedTotalPCount_CPU += effectCPUPCount;
			displayedTotalPCount_GPU += effectGPUPCount;
			displayedTotalICount += effectInstanceCount;

			// Relative time
			{
				DrawBar(relTimexPos, relTimeCPUxPos - 1, yPos, effectTimeRelative / 100.0f, lineHeight);
				Canvas->DrawText(font, FString::Printf(TEXT("%.1f"), effectTimeRelative), relTimexPos, yPos, 1.f, 1.f, friNoShadow);
				if (hasGPU)
				{
					DrawBar(relTimeCPUxPos, relTimeGPUxPos - 1, yPos, effectTimeRelativeCPU / 100.0f, lineHeight);
					Canvas->DrawText(font, FString::Printf(TEXT("%.1f"), effectTimeRelativeCPU), relTimeCPUxPos, yPos, 1.f, 1.f, friNoShadow);
					DrawBar(relTimeGPUxPos, timexPos - 1, yPos, effectTimeRelativeGPU / 100.0f, lineHeight);
					Canvas->DrawText(font, FString::Printf(TEXT("%.1f"), effectTimeRelativeGPU), relTimeGPUxPos, yPos, 1.f, 1.f, friNoShadow);
				}
			}

			// Timings
			{
				const float									timeTotalCPU = effectTimeCPU * timeNormalizerCPU;
				const float									timeTotalGPU = effectTimeGPU;
				const float									timeTotal = timeTotalCPU + timeTotalGPU;
				const PopcornFX::Units::SValueAndNamedUnit	readableTime = PopcornFX::Units::AutoscaleTime(timeTotal, 0.5f);
				const PopcornFX::Units::SValueAndNamedUnit	readableTimeCPU = PopcornFX::Units::AutoscaleTime(timeTotalCPU, 0.5f);
				const PopcornFX::Units::SValueAndNamedUnit	readableTimeGPU = PopcornFX::Units::AutoscaleTime(timeTotalGPU, 0.5f);

				DrawBar(timexPos, timeCPUxPos - 1, yPos, timeTotal / TimeLimitPerEffectSeconds, lineHeight);
				Canvas->DrawText(font, FString::Printf(TEXT("%.1f %s"), readableTime.m_Value, ANSI_TO_TCHAR(readableTime.m_UnitName)), timexPos, yPos, 1.f, 1.f, friNoShadow);
				if (hasGPU)
				{
					DrawBar(timeCPUxPos, timeGPUxPos - 1, yPos, timeTotalCPU / CPUTimeLimitPerEffectSeconds, lineHeight);
					Canvas->DrawText(font, FString::Printf(TEXT("%.1f %s"), readableTimeCPU.m_Value, ANSI_TO_TCHAR(readableTimeCPU.m_UnitName)), timeCPUxPos, yPos, 1.f, 1.f, friNoShadow);
					DrawBar(timeGPUxPos, pCountCPUxPos - 1, yPos, timeTotalGPU / GPUTimeLimitPerEffectSeconds, lineHeight);
					Canvas->DrawText(font, FString::Printf(TEXT("%.1f %s"), readableTimeGPU.m_Value, ANSI_TO_TCHAR(readableTimeGPU.m_UnitName)), timeGPUxPos, yPos, 1.f, 1.f, friNoShadow);
				}
			}

			// Counts
			{
				DrawBar(pCountCPUxPos, pCountGPUxPos - 1, yPos, (float)effectCPUPCount / (float)CPUParticleCountLimitPerEffect, lineHeight);
				Canvas->DrawText(font, FString::Printf(TEXT("%d"), effectCPUPCount), pCountCPUxPos, yPos, 1.f, 1.f, friNoShadow);
				if (hasGPU)
				{
					DrawBar(pCountGPUxPos, iCountxPos - 1, yPos, (float)effectGPUPCount / (float)GPUParticleCountLimitPerEffect, lineHeight);
					Canvas->DrawText(font, FString::Printf(TEXT("%d"), effectGPUPCount), pCountGPUxPos, yPos, 1.f, 1.f, friNoShadow);
				}

				Canvas->DrawText(font, FString::Printf(TEXT("%d"), effectInstanceCount), iCountxPos, yPos, 1.f, 1.f, friNoShadow);
			}

			// Effect path
			Canvas->DrawText(font, FString::Printf(TEXT("%s"), ANSI_TO_TCHAR(allSceneTimings[iTiming].m_EffectPath.Data())), effectPathxPos, yPos, 1.f, 1.f, fri);

			yPos += lineHeight;
			if (yPos > maxDrawyPos)
				break;
		}

		yPos += lineHeight / 2;

		const float		displayedEffectsPercentage = totalTime > 0.0 ? 100.0f * (displayedTotalTime / totalTime) : 0.0f;
		const float		displayedEffectsPercentageCPU = totalTimeCPU > 0.0 ? 100.0f * (displayedTotalTimeCPU / totalTimeCPU) : 0.0f;
		const float		displayedEffectsPercentageGPU = totalTimeGPU > 0.0 ? 100.0f * (displayedTotalTimeGPU / totalTimeGPU) : 0.0f;

		// Relative time
		{
			DrawBar(relTimexPos, relTimeCPUxPos - 1, yPos, displayedEffectsPercentage / 100.0f, lineHeight);
			Canvas->DrawText(font, FString::Printf(TEXT("%5.1f"), displayedEffectsPercentage), relTimexPos, yPos, 1.f, 1.f, friNoShadow);
			if (hasGPU)
			{
				DrawBar(relTimeCPUxPos, relTimeGPUxPos - 1, yPos, displayedEffectsPercentageCPU / 100.0f, lineHeight);
				Canvas->DrawText(font, FString::Printf(TEXT("%5.1f"), displayedEffectsPercentageCPU), relTimeCPUxPos, yPos, 1.f, 1.f, friNoShadow);
				DrawBar(relTimeGPUxPos, timexPos - 1, yPos, displayedEffectsPercentageGPU / 100.0f, lineHeight);
				Canvas->DrawText(font, FString::Printf(TEXT("%5.1f"), displayedEffectsPercentageGPU), relTimeGPUxPos, yPos, 1.f, 1.f, friNoShadow);
			}
		}

		// Timings
		{
			const float									timeTotalCPU = displayedTotalTimeCPU * timeNormalizerCPU;
			const float									timeTotalGPU = displayedTotalTimeGPU;
			const float									timeTotal = timeTotalCPU + timeTotalGPU;
			const PopcornFX::Units::SValueAndNamedUnit	readableTotalDisplayedTime = PopcornFX::Units::AutoscaleTime(timeTotal, 0.5f);
			const PopcornFX::Units::SValueAndNamedUnit	readableTotalDisplayedTimeCPU = PopcornFX::Units::AutoscaleTime(timeTotalCPU, 0.5f);
			const PopcornFX::Units::SValueAndNamedUnit	readableTotalDisplayedTimeGPU = PopcornFX::Units::AutoscaleTime(timeTotalGPU, 0.5f);
			DrawBar(timexPos, timeCPUxPos - 1, yPos, timeTotal / TimeLimitTotalSeconds, lineHeight);
			Canvas->DrawText(font, FString::Printf(TEXT("%.1f %s"), readableTotalDisplayedTime.m_Value, ANSI_TO_TCHAR(readableTotalDisplayedTime.m_UnitName)), timexPos, yPos, 1.f, 1.f, friNoShadow);
			if (hasGPU)
			{
				DrawBar(timeCPUxPos, timeGPUxPos - 1, yPos, timeTotalCPU / CPUTimeLimitTotalSeconds, lineHeight);
				Canvas->DrawText(font, FString::Printf(TEXT("%.1f %s"), readableTotalDisplayedTimeCPU.m_Value, ANSI_TO_TCHAR(readableTotalDisplayedTimeCPU.m_UnitName)), timeCPUxPos, yPos, 1.f, 1.f, friNoShadow);
				DrawBar(timeGPUxPos, pCountCPUxPos - 1, yPos, timeTotalGPU / GPUTimeLimitTotalSeconds, lineHeight);
				Canvas->DrawText(font, FString::Printf(TEXT("%.1f %s"), readableTotalDisplayedTimeGPU.m_Value, ANSI_TO_TCHAR(readableTotalDisplayedTimeGPU.m_UnitName)), timeGPUxPos, yPos, 1.f, 1.f, friNoShadow);
			}
		}

		// Counts
		{
			DrawBar(pCountCPUxPos, pCountGPUxPos - 1, yPos, (float)displayedTotalPCount_CPU / (float)CPUParticleCountLimitTotal, lineHeight);
			Canvas->DrawText(font, FString::Printf(TEXT("%d"), displayedTotalPCount_CPU), pCountCPUxPos, yPos, 1.f, 1.f, friNoShadow);
			if (hasGPU)
			{
				DrawBar(pCountGPUxPos, iCountxPos - 1, yPos, (float)displayedTotalPCount_GPU / (float)GPUParticleCountLimitTotal, lineHeight);
				Canvas->DrawText(font, FString::Printf(TEXT("%d"), displayedTotalPCount_GPU), pCountGPUxPos, yPos, 1.f, 1.f, friNoShadow);
			}

			Canvas->DrawText(font, FString::Printf(TEXT("%d"), displayedTotalICount), iCountxPos, yPos, 1.f, 1.f, friNoShadow);
		}

		Canvas->DrawText(font, TEXT("Effects timings displayed here"), effectPathxPos, yPos, 1.f, 1.f, fri);
	}

#endif // (PK_PARTICLES_HAS_STATS != 0)
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

static void		_PopcornFXToggleProfilerHUD(const TArray<FString>& args, UWorld *inWorld)
{
	if (!inWorld)
	{
		UE_LOG(LogPopcornFXProfilerHUD, Warning, TEXT("ToggleProfilerHUD: no World"));
		return;
	}
	bool	hasForce = false;
	bool	forceOn = false;
	if (args.Num() >= 1)
	{
		hasForce = true;
		if (args[0] == "1")
			forceOn = true;
	}

	static FDelegateHandle	s_drawDebugDelegateHandle;
	APopcornFXHUDProfiler	*HUD = NULL;
	for (TActorIterator<APopcornFXHUDProfiler> it(inWorld); it; ++it)
	{
		HUD = *it;
		break;
	}
	if (hasForce && forceOn == (HUD != null))
		return;
	if (HUD == null)
	{
		HUD = inWorld->SpawnActor<APopcornFXHUDProfiler>();

		FDebugDrawDelegate	drawDebugDelegate = FDebugDrawDelegate::CreateUObject(HUD, &APopcornFXHUDProfiler::DrawDebugHUD);
		s_drawDebugDelegateHandle = UDebugDrawService::Register(TEXT("OnScreenDebug"), drawDebugDelegate);
	}
	else
	{
		UDebugDrawService::Unregister(s_drawDebugDelegateHandle);
		HUD->Destroy();
	}
}

FAutoConsoleCommandWithWorldAndArgs		PopcornFXHUDProfilerCommand(
	TEXT("PopcornFX.ToggleProfilerHUD"),
	TEXT("ToggleProfilerHUD [0|1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(_PopcornFXToggleProfilerHUD));

#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
