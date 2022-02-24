//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXDependencyModule.h"

#include "PopcornFXPlugin.h"

//----------------------------------------------------------------------------

FPopcornFXDependencyModule::FPopcornFXDependencyModule()
:	m_Loaded(false)
{
	m_Plugin = (FPopcornFXPlugin*)&IPopcornFXPlugin::Get();
}

//----------------------------------------------------------------------------

FPopcornFXDependencyModule::~FPopcornFXDependencyModule()
{
	PK_ASSERT(!Loaded());
	// Unload(); // vtable if fucked up here, cannot call the virtual function
}

//----------------------------------------------------------------------------
