//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "PopcornFXDependencyModule.h"

class POPCORNFX_API FPopcornFXDependencyModuleSettings : public FPopcornFXDependencyModule
{
public:
	FPopcornFXDependencyModuleSettings();

	virtual void	Load() override;
	virtual void	Unload() override;
};
