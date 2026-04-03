//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
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
