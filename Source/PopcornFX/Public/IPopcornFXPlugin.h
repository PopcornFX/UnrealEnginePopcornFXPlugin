//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "Modules/ModuleManager.h"
#if WITH_EDITOR
#	include "Toolkits/IToolkit.h"
#endif

class	IPopcornFXEffectEditor;

class POPCORNFX_API IPopcornFXPlugin : public IModuleInterface
{
public:
	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline IPopcornFXPlugin& Get()
	{
		return FModuleManager::LoadModuleChecked<IPopcornFXPlugin>("PopcornFX");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("PopcornFX");
	}

#if WITH_EDITOR
	virtual TSharedRef<class IPopcornFXEffectEditor>		CreateEffectEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, class UPopcornFXEffect* Effect) = 0;
#endif
};
