//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "IPopcornFXEditorPlugin.h"

class	FPopcornFXDependencyModule;

class	FPopcornFXEditorPlugin : public IPopcornFXEditorPlugin
{
	TMap<FName, FPopcornFXDependencyModule*>	m_DeferredModuleActions;

public:
	FPopcornFXEditorPlugin();

	static float	s_GlobalScale;

	static inline FPopcornFXEditorPlugin& Get()
	{
		return static_cast<FPopcornFXEditorPlugin&>(FModuleManager::LoadModuleChecked<FPopcornFXEditorPlugin>("PopcornFXEditor"));
	}

	virtual void		StartupModule() override;
	virtual void		ShutdownModule() override;

private:
	void				OnModulesChangedCallback(FName moduleThatChanged, EModuleChangeReason reason);
	void				UnloadModuleActions(TMap<FName, FPopcornFXDependencyModule*> &moduleActions);

	FDelegateHandle		m_OnModulesChangedHandle;

	TSharedPtr<class IComponentAssetBroker>		m_PopcornFXAssetBroker;
};
