//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#if WITH_EDITOR

#include "PopcornFXDependencyModule.h"

class POPCORNFX_API FPopcornFXDependencyModulePropertyEditor : public FPopcornFXDependencyModule
{
public:
	FPopcornFXDependencyModulePropertyEditor();

	virtual void	Load() override;
	virtual void	Unload() override;
private:
	void	CreateToolBarExtension(class FToolBarBuilder &toolbarBuilder);
};

#endif // WITH_EDITOR
