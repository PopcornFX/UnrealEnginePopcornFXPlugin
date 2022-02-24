//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXOnDefaultModule.h"

#include "Modules/ModuleManager.h"
#include "Runtime/Launch/Resources/Version.h"

/**
	Since 4.13, PopcornFXEditor is not loaded automatically in Standalone Game
	But Standalone Game needs to PopcornFXEditor stuff (Kismet nodes, SetAttribute, GetAttributes ...)

	This module is meant to workaround this, and manually load the PopcornFXEditor module if in Editor mode,
	during the "Default" LoadingPhase.
	PopcornFX module cannot do that because loaded too early ("PostConfigInit" LoadingPhase, because of vertex factories)
	And I could not find a sutable delegate where PopcornFX module could hook himself to load PopcornFXEditor.
*/

IMPLEMENT_MODULE(FPopcornFXOnDefaultModule, PopcornFXOnDefault)

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXOnDefault, Log, All);

void	FPopcornFXOnDefaultModule::StartupModule()
{
#if WITH_EDITOR
	FModuleManager		&ModuleManager = FModuleManager::Get();
	{
		const FName			ModuleName = TEXT("PopcornFXEditor");
		if (!ModuleManager.IsModuleLoaded(ModuleName))
		{
			if (ModuleManager.LoadModule(ModuleName) == nullptr)	// ptr not valid
			{
				UE_LOG(LogPopcornFXOnDefault, Error, TEXT("Failed to load PopcornFXEditor module, might crash later"));
			}
		}
	}
#endif // WITH_EDITOR
}

void	FPopcornFXOnDefaultModule::ShutdownModule()
{
}
