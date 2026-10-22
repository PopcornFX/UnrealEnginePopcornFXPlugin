//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "PropertyHandle.h"

inline bool			IsValidHandle(const TSharedPtr<IPropertyHandle> &pty) { return pty.IsValid() && pty->IsValidHandle(); }
