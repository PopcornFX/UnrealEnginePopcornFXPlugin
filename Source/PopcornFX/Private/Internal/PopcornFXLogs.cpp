//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#include "Logging/TokenizedMessage.h"
#include "Logging/LogVerbosity.h"

#include "PopcornFXSDK.h"
#include <pk_kernel/include/kr_log_listeners.h>

//----------------------------------------------------------------------------

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFX, Log, All);

//----------------------------------------------------------------------------

namespace
{
	const EMessageSeverity::Type			kSeverityToUE[] = {
		EMessageSeverity::Info,				//	Level_Debug = 0,
		EMessageSeverity::Info,				//	Level_Info,
		EMessageSeverity::Warning,			//	Level_Warning,
		EMessageSeverity::Error,			//	Level_Error,
		EMessageSeverity::Error,			//	Level_ErrorCritical,
		EMessageSeverity::Error,			//	Level_ErrorInternal,	// triggered by kr_assert, if you need to use one explicitely, prefer using an assert, or a release_assert
		EMessageSeverity::Info,				//	Level_None,	// used mainly to disable all logging when calling 'SetGlobalLogLevel()'
	};
	PK_STATIC_ASSERT(PK_ARRAY_COUNT(kSeverityToUE) == PopcornFX::CLog::__MaxLogLevels);

	const ELogVerbosity::Type				kLogVerbosityToUE[] = {
		ELogVerbosity::Verbose,				//	Level_Debug = 0,
		ELogVerbosity::Log,					//	Level_Info,
		ELogVerbosity::Warning,				//	Level_Warning,
		// Errors will block pakaging
		ELogVerbosity::Warning, // ELogVerbosity::Error,				//	Level_Error,
		ELogVerbosity::Warning,				//	Level_ErrorCritical,
		ELogVerbosity::Fatal,				//	Level_ErrorInternal,	// triggered by kr_assert, if you need to use one explicitely, prefer using an assert, or a release_assert
		ELogVerbosity::Log,					//	Level_None,	// used mainly to disable all logging when calling 'SetGlobalLogLevel()'
	};
	PK_STATIC_ASSERT(PK_ARRAY_COUNT(kSeverityToUE) == PK_ARRAY_COUNT(kLogVerbosityToUE));

	class	CLogListenerUE : public PopcornFX::ILogListener
	{
	public:
		CLogListenerUE()
		{
		}

		virtual void	Notify(PopcornFX::CLog::ELogLevel level, PopcornFX::CGuid logClass, const char *message) override
		{
#ifndef PK_RETAIL
			PK_ASSERT(level < PK_ARRAY_COUNT(kSeverityToUE));
			PopcornFX::CString	s = PopcornFX::CString::Format("[%d][%s]> %s", level, PopcornFX::CLog::LogClassToString(logClass), message);
			if (!LogPopcornFX.IsSuppressed(kLogVerbosityToUE[level]))
			{
				const ELogVerbosity::Type		logLevel = kLogVerbosityToUE[level];
				FMsg::Logf(__FILE__, __LINE__, LogPopcornFX.GetCategoryName(), logLevel, TEXT("%s"), UTF8_TO_TCHAR(s.Data()));
			}
#endif
		}
	private:
	};
} // namespace

//----------------------------------------------------------------------------

void	AddDefaultGlobalListenersOverride(void *userHandle)
{
	PopcornFX::CLog::AddGlobalListener(PK_NEW(CLogListenerUE));
}

//----------------------------------------------------------------------------
