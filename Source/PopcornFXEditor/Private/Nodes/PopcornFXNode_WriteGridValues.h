//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXNode_GridNode.h"

#include "PopcornFXNode_WriteGridValues.generated.h"

UCLASS()
class UPopcornFXNode_WriteGridValues : public UPopcornFXNode_GridNode
{
	GENERATED_UCLASS_BODY()

protected:
	virtual bool	SetupNativeFunctionCall(UK2Node_CallFunction *functionCall) override;
};
