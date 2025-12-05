//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXNode_GridNode.h"

#include "PopcornFXNode_ReadGridValues.generated.h"

UCLASS()
class UPopcornFXNode_ReadGridValues : public UPopcornFXNode_GridNode
{
	GENERATED_UCLASS_BODY()

protected:
	virtual bool	SetupNativeFunctionCall(UK2Node_CallFunction *functionCall) override;
};
