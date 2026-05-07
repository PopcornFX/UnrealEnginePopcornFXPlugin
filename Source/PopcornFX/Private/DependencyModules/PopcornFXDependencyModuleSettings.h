//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
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
