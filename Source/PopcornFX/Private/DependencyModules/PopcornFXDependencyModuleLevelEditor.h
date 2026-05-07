//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "PopcornFXDependencyModule.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"

class	FPopcornFXFileTypeActions;

class POPCORNFX_API FPopcornFXDependencyModuleLevelEditor : public FPopcornFXDependencyModule
{
public:
	static bool	CannotExecuteAction() { return false; }

	FPopcornFXDependencyModuleLevelEditor();

	virtual void	Load() override;
	virtual void	Unload() override;

private:
	void	CreateMenuBarExtension(class FMenuBarBuilder &menubarBuilder);
	void	FillPopcornFXMenu(FMenuBuilder &menuBuilder);

	FText	GetVersions();

	void	OpenDocumentationURL(const TCHAR *url);
	void	OpenSettings(const TCHAR *sectionName);
	void	OpenSourcePack();
	bool	CanOpenSourcePack();
	void	RunCommand(const TCHAR *command);

private:
	class UPopcornFXSettingsEditor	*m_Settings;

	FText		m_Versions;
};

#endif // WITH_EDITOR
