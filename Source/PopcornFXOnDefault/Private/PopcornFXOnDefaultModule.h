//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "Runtime/Launch/Resources/Version.h"
#include "Modules/ModuleInterface.h"

class POPCORNFXONDEFAULT_API FPopcornFXOnDefaultModule : public IModuleInterface
{
public:
	virtual void	StartupModule() override;
	virtual void	ShutdownModule() override;
};
