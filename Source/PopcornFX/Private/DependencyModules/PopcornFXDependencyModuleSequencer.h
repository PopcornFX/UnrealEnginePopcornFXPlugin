//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
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
