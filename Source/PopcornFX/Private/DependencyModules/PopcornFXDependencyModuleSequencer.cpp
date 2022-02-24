//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR

#include "PopcornFXDependencyModuleSequencer.h"

#include "ISequencerModule.h"
#include "Sequencer/TrackEditors/PopcornFXAttributeTrackEditor.h"
#include "Sequencer/TrackEditors/PopcornFXPlayTrackEditor.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXDependencyModuleSettings"

//----------------------------------------------------------------------------

FPopcornFXDependencyModuleSequencer::FPopcornFXDependencyModuleSequencer()
{
}

//----------------------------------------------------------------------------

void	FPopcornFXDependencyModuleSequencer::Load()
{
	if (m_Loaded)
		return;

	if (!FModuleManager::Get().IsModuleLoaded("Sequencer"))
		return;

	ISequencerModule	&sequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");

	AttributeTrackCreateEditorHandle = sequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FPopcornFXAttributeTrackEditor::CreateTrackEditor));
	PlayTrackCreateEditorHandle = sequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FPopcornFXPlayTrackEditor::CreateTrackEditor));

	m_Loaded = true;
}

//----------------------------------------------------------------------------

void	FPopcornFXDependencyModuleSequencer::Unload()
{
	if (!m_Loaded)
		return;
	m_Loaded = false;

	if (!FModuleManager::Get().IsModuleLoaded("Sequencer"))
		return;

	ISequencerModule	&sequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");

	sequencerModule.UnRegisterTrackEditor(AttributeTrackCreateEditorHandle);
	sequencerModule.UnRegisterTrackEditor(PlayTrackCreateEditorHandle);
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------

#endif // WITH_EDITOR
