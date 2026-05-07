//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#if	!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#	define	POPCORNFX_PROFILER_ENABLED		1
#else
#	define	POPCORNFX_PROFILER_ENABLED		0
#endif

#if POPCORNFX_PROFILER_ENABLED

#include "PopcornFXSDK.h"
#include <pk_kernel/include/kr_profiler.h>
#include <pk_kernel/include/kr_profiler_details.h>

FWD_PK_API_BEGIN
// Might move in PopcornFX Runtime some time
namespace Profiler
{
	class CMultiReport
	{
	public:
		CMultiReport();
		~CMultiReport();

		void						SetReportHistoryCount(u32 reportCount);

		void						Clear();

		void						MergeReport(CProfiler *profiler);

		struct SReportEntry
		{
			CProfilerReport			m_Report;
		};

	private:
		void						ClearReport(u32 i);

		TArray<SReportEntry>		m_Reports;
		u32							m_NextReport;
	};
}
FWD_PK_API_END
// Statement to help the UE Header Parser not crash on FWD_PK_API_...
class	FPopcornFXPlugin;

class	CPopcornFXProfiler
{
public:
	CPopcornFXProfiler();
	~CPopcornFXProfiler();

	void			ForceStopAll();

	void			RequestHBOStart(UWorld *world);
	void			RequestHBOStop();

	void			RecordAndSaveProfilerReport(UWorld *world, u32 frameCount, const FString &outputFilePath);

private:
	void			_OnWorldTick(float dt);
	void			_OnWorldAdded(UWorld *world);
	void			_OnWorldDestroyed(UWorld *world);

	void			_End_RecordAndSaveProfilerReport();

	FDelegateHandle						m_WorldAddedHandle;
	FDelegateHandle						m_WorldDestroyedHandle;

	u32				m_TickMaxEveryFrame;
	float			m_TickMaxEveryTime;
	u32				m_CurrentTickFrame;
	float			m_CurrentTickTime;

	u32				m_HBORequestCount;

	s32				m_RecordingFrameCountToSave;
	FString			m_RecordingOutputPath;

	struct SWorldTick
	{
		UWorld				*m_World;
		FDelegateHandle		m_TickHandle;
	};
	TArray<SWorldTick>					m_WorldTicks;

};

#endif // POPCORNFX_PROFILER_ENABLED
