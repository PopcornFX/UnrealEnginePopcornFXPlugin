//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
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
