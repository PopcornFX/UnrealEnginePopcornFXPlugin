//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXProfiler.h"

#if POPCORNFX_PROFILER_ENABLED

#include "PopcornFXPlugin.h"

#if WITH_EDITOR
#	include "DesktopPlatformModule.h"
#	include "EditorDirectories.h"
#	include "Framework/Application/SlateApplication.h"
#endif
#include "Misc/FeedbackContext.h" // GWarn
#include "Misc/FileHelper.h"
#include "Misc/ConfigCacheIni.h"
#include "Engine/Engine.h"

#include "PopcornFXSDK.h"
#include <pk_kernel/include/kr_streams_memory.h>
#include <pk_kernel/include/kr_profiler_details.h>

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FPopcornFXProfiler"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXProfiler, Log, All);

//----------------------------------------------------------------------------

FWD_PK_API_BEGIN
//----------------------------------------------------------------------------

Profiler::CMultiReport::CMultiReport()
:	m_NextReport(0)
{
	SetReportHistoryCount(64);
}

//----------------------------------------------------------------------------

Profiler::CMultiReport::~CMultiReport()
{
}

//----------------------------------------------------------------------------

void	Profiler::CMultiReport::SetReportHistoryCount(u32 reportCount)
{
	if (m_Reports.Count() < reportCount)
	{
		m_Reports.Resize(reportCount);
	}
	else if (m_Reports.Count() > reportCount)
	{
		for (u32 i = reportCount; i < m_Reports.Count(); ++i)
			ClearReport(i);
		m_Reports.Resize(reportCount);
	}
}

//----------------------------------------------------------------------------

void	Profiler::CMultiReport::Clear()
{
	for (u32 i = 0; i < m_Reports.Count(); ++i)
		ClearReport(i);
}

//----------------------------------------------------------------------------

void	Profiler::CMultiReport::MergeReport(CProfiler *profiler)
{
	const u32	reporti = m_NextReport;
	m_NextReport = (m_NextReport + 1) % m_Reports.Count();

	ClearReport(reporti);

	profiler->BuildReport(&m_Reports[reporti].m_Report);
}

//----------------------------------------------------------------------------

void	Profiler::CMultiReport::ClearReport(u32 i)
{
	SReportEntry		&report = m_Reports[i];

	report.m_Report.Clear();
}

//----------------------------------------------------------------------------
FWD_PK_API_END

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

CPopcornFXProfiler::CPopcornFXProfiler()
	: m_TickMaxEveryFrame(8) // 8 frames per report
	, m_TickMaxEveryTime(10.f)
	, m_CurrentTickFrame(0)
	, m_CurrentTickTime(0)
	, m_HBORequestCount(0)
	, m_RecordingFrameCountToSave(-1)
{
	UEngine		*engine = GEngine;
	if (PK_VERIFY(engine != null))
	{
		m_WorldAddedHandle = engine->OnWorldAdded().AddRaw(this, &CPopcornFXProfiler::_OnWorldAdded);
		m_WorldDestroyedHandle = engine->OnWorldDestroyed().AddRaw(this, &CPopcornFXProfiler::_OnWorldDestroyed);
	}
}

//----------------------------------------------------------------------------

CPopcornFXProfiler::~CPopcornFXProfiler()
{
	UEngine			*engine = GEngine;
	if (engine != null) // happens at shutdown
	{
		engine->OnWorldAdded().Remove(m_WorldAddedHandle);
		engine->OnWorldDestroyed().Remove(m_WorldDestroyedHandle);
	}
	for (int32 i = 0; i < m_WorldTicks.Num(); ++i)
	{
		SWorldTick		&t = m_WorldTicks[i];
		t.m_World->OnTickDispatch().Remove(t.m_TickHandle);
	}

	ForceStopAll();
}

//----------------------------------------------------------------------------

void	CPopcornFXProfiler::ForceStopAll()
{
	PopcornFX::Profiler::CProfiler		*profiler = PopcornFX::Profiler::MainEngineProfiler();
	if (profiler != null && profiler->Active())
	{
		profiler->Activate(false);
		UE_LOG(LogPopcornFXProfiler, Log, TEXT("Stoping Profiler Recording"));
	}

	m_HBORequestCount = 0;
	m_RecordingFrameCountToSave = -1;
	m_RecordingOutputPath = FString();
}

//----------------------------------------------------------------------------

void	CPopcornFXProfiler::_OnWorldAdded(UWorld *world)
{
	if (world == null)
		return;

	if (m_HBORequestCount == 0)
		return;

	for (int32 i = 0; i < m_WorldTicks.Num(); ++i)
	{
		SWorldTick		&t = m_WorldTicks[i];
		if (t.m_World == world) // already added
			return;
	}

	SWorldTick		*t = new (m_WorldTicks) SWorldTick();
	if (t != null)
	{
		t->m_World = world;
		t->m_TickHandle = world->OnTickDispatch().AddRaw(this, &CPopcornFXProfiler::_OnWorldTick);
	}
}

//----------------------------------------------------------------------------

void	CPopcornFXProfiler::_OnWorldDestroyed(UWorld *world)
{
	for (int32 i = 0; i < m_WorldTicks.Num(); ++i)
	{
		SWorldTick		&t = m_WorldTicks[i];
		if (t.m_World == world)
		{
			world->OnTickDispatch().Remove(t.m_TickHandle);
			m_WorldTicks.RemoveAt(i);
			break;
		}
	}
}

//----------------------------------------------------------------------------

void	CPopcornFXProfiler::_OnWorldTick(float dt)
{
	PK_ASSERT(FPopcornFXPlugin::IsMainThread());

	if (!PK_VERIFY(m_HBORequestCount > 0) ||  // no one needs us, why are we ticking ?
		!PK_VERIFY(m_WorldTicks.Num() >= 1))
	{
		ForceStopAll(); // just in case
		return;
	}

	bool		saveProfilerReport = false;
	if (m_RecordingFrameCountToSave > 0)
	{
		--m_RecordingFrameCountToSave;
		if (m_RecordingFrameCountToSave == 0)
		{
			m_RecordingFrameCountToSave = -1;
			saveProfilerReport = true;
		}
	}

	m_CurrentTickFrame++;
	m_CurrentTickTime += dt;

	if (!saveProfilerReport &&
		m_CurrentTickFrame < m_TickMaxEveryFrame)
		//m_CurrentTickTime < m_ProfilerUpdateMaxEveryTime)
		return;

	m_CurrentTickFrame = 0;
	m_CurrentTickTime = 0;

	// IMPORTANT, do not skip
	if (saveProfilerReport)
		_End_RecordAndSaveProfilerReport();
}

//----------------------------------------------------------------------------

void	CPopcornFXProfiler::RequestHBOStart(UWorld *world)
{
	PK_ASSERT(FPopcornFXPlugin::IsMainThread());

	++m_HBORequestCount;

	_OnWorldAdded(world != null ? world : GWorld);

	if (m_HBORequestCount > 1) // was already activated
		return;
}

//----------------------------------------------------------------------------

void	CPopcornFXProfiler::RequestHBOStop()
{
	PK_ASSERT(FPopcornFXPlugin::IsMainThread());

	if (!PK_VERIFY(m_HBORequestCount > 0))
		return;
	--m_HBORequestCount;

	if (m_HBORequestCount > 0) // do not stop yet
		return;

	for (int32 i = 0; i < m_WorldTicks.Num(); ++i)
	{
		SWorldTick		&t = m_WorldTicks[i];
		t.m_World->OnTickDispatch().Remove(t.m_TickHandle);
	}
	m_WorldTicks.Empty(m_WorldTicks.Num());
}

//----------------------------------------------------------------------------

void	CPopcornFXProfiler::RecordAndSaveProfilerReport(UWorld *world, u32 frameCount, const FString &outputFilePath)
{
	if (!PK_VERIFY(frameCount > 0))
		return;

	if (!PK_VERIFY(m_RecordingFrameCountToSave < 0)) // already running !
		return;

	PopcornFX::Profiler::CProfiler		*profiler = PopcornFX::Profiler::MainEngineProfiler();
	if (profiler == null)
		return;

	if (!profiler->Active())
	{
		profiler->Activate(true);
		profiler->GrabCallstacks(false);
		profiler->Reset();
	}

	UE_LOG(LogPopcornFXProfiler, Log, TEXT("Start Recording Profiler Report for %d frames ..."), frameCount);

	m_RecordingFrameCountToSave = frameCount;
	m_RecordingOutputPath = outputFilePath;
	RequestHBOStart(world); // will activate Tick
}

//----------------------------------------------------------------------------

void	CPopcornFXProfiler::_End_RecordAndSaveProfilerReport()
{
	RequestHBOStop(); // will activate Tick

	FString				outputPath = m_RecordingOutputPath;
	m_RecordingOutputPath = FString();
	m_RecordingFrameCountToSave = -1;

	UE_LOG(LogPopcornFXProfiler, Log, TEXT("Stoping and Saving Profiler Report..."));

	PopcornFX::Profiler::CProfiler	*profiler = PopcornFX::Profiler::MainEngineProfiler();
	if (!PK_VERIFY(profiler != null))
		return;

	profiler->GrabCallstacks(false);
	profiler->Activate(false);
	profiler->Reset();
	profiler->BuildReport();

	const PopcornFX::Profiler::CProfilerReport		&latestReport = profiler->LatestReport();

	if (outputPath.IsEmpty())
	{
#if WITH_EDITOR
		TArray<FString>		saveFilenames;
		IDesktopPlatform*	desktopPlatform = FDesktopPlatformModule::Get();
		PK_ASSERT(desktopPlatform != null);

		TSharedPtr<SWindow>	parentWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
		void*				parentWindowHandle = (parentWindow.IsValid() && parentWindow->GetNativeWindow().IsValid()) ? parentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;
		PK_ASSERT(parentWindowHandle != null);

		const bool	saved = desktopPlatform->SaveFileDialog(
			parentWindowHandle,
			NSLOCTEXT("PopcornFX", "SaveProfilerReport", "Save Profiler Report (pkpr)").ToString(),
			*(FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_SAVE)),
			TEXT(""),
			TEXT("PopcornFX Profiler Report|*.pkpr"),
			EFileDialogFlags::None,
			saveFilenames);

		// The user discarded the save dialog
		if (!saved || !PK_VERIFY(saveFilenames.Num() > 0))
		{
			UE_LOG(LogPopcornFXProfiler, Warning, TEXT("Saving PopcornFX Profiler Report has been aborted"));
			return;
		}
		outputPath = saveFilenames[0];
		FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_SAVE, FPaths::GetPath(outputPath)); // Save path as default for next time.
#else
		outputPath = "ue4.pkpr"; // relative to execution dir
		//outputPath = FPaths::GameDir() / "ue4.pkpr"; // relative to game dir
#endif
	}

	// Encapsulate the write into a slow task warn
	GWarn->BeginSlowTask(NSLOCTEXT("PopcornFX", "SaveProfilerReportSlowTask", "Saving PopcornFX Profiler Report..."), false);
	{

		PopcornFX::CDynamicMemoryStream		outputStream;
		PopcornFX::Profiler::WriteProfileReport(latestReport, outputStream);

		u32	finalSize;
		u8	*data = outputStream.ExportDataAndClose(finalSize);

		TArray<uint8>	arrayData;
		arrayData.Append(data, finalSize);
		PK_FREE(data);

		if (FFileHelper::SaveArrayToFile(arrayData, *outputPath))
		{
			UE_LOG(LogPopcornFXProfiler, Log, TEXT("PopcornFX Profiler Report saved \"%s\""), *outputPath);
		}
		else
		{
			UE_LOG(LogPopcornFXProfiler, Error, TEXT("Couldn't save PopcornFX Profiler Report to \"%s\""), *outputPath);
		}
	}
	GWarn->EndSlowTask();

}

//----------------------------------------------------------------------------
//
// UE Command PopcornFX.RecordProfilerReport
//
//----------------------------------------------------------------------------

static void		_PopcornFXProfilerStartRecord(const TArray<FString>& args, UWorld* world)
{
	bool	useUETaskGraph = false; // TODO: Store that globally in plugin
	bool	recordProfileMarkers = false;
	GConfig->GetBool(TEXT("PopcornFX"), TEXT("bUseUETaskGraph"), useUETaskGraph, GEngineIni);
	GConfig->GetBool(TEXT("PopcornFX"), TEXT("bRecordProfileMarkers"), recordProfileMarkers, GEngineIni);

	if (!useUETaskGraph)
		recordProfileMarkers = false;
	if (!recordProfileMarkers) // Either .pkpr, or UE insights
	{
		CPopcornFXProfiler		*profiler = FPopcornFXPlugin::Get().Profiler();
		if (!PK_VERIFY(profiler != null))
			return;

		const u32	defaultFrameCount = 30;
		const u32	maxFrameCount = 1000;

		int32		frameCount = defaultFrameCount;
		FString		outputPath;
		const u32	argCount = args.Num();
		if (argCount >= 1)
			frameCount = FCString::Atoi(*args[0]);
		if (argCount >= 2)
			outputPath = args[1];

		if (frameCount <= 0)
		{
			UE_LOG(LogPopcornFXProfiler, Error, TEXT("Invalid number of frames (%d), aborting"), frameCount);
			return;
		}
		else if (frameCount > maxFrameCount)
		{
			UE_LOG(LogPopcornFXProfiler, Warning, TEXT("Recording more than %d frames is too dangerous, aborting"), maxFrameCount);
			frameCount = maxFrameCount;
		}

		profiler->RecordAndSaveProfilerReport(world, frameCount, outputPath);
	}
}

//----------------------------------------------------------------------------

FAutoConsoleCommandWithWorldAndArgs		PopcornFXProfilerStartRecordCommand(
	TEXT("PopcornFX.RecordProfilerReport"),
	TEXT("PopcornFX.RecordProfilerReport [FrameCount] [pkprOutputFilePath]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(_PopcornFXProfilerStartRecord));

//----------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

#endif // POPCORNFX_PROFILER_ENABLED
