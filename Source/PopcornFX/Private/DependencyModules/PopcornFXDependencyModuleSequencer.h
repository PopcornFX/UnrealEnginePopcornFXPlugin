//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXDependencyModule.h"

class POPCORNFX_API FPopcornFXDependencyModuleSequencer : public FPopcornFXDependencyModule
{
public:
	FPopcornFXDependencyModuleSequencer();

	virtual void	Load() override;
	virtual void	Unload() override;
private:
	FDelegateHandle		AttributeTrackCreateEditorHandle;
	FDelegateHandle		PlayTrackCreateEditorHandle;
};

#endif // WITH_EDITOR
