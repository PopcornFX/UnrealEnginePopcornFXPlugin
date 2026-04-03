//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "Nodes/PopcornFXNode_Attribute.h"
#include "PopcornFXNode_SetAttribute.generated.h"

/*
*	A custom Blueprint node that represents any of the SetAttribute() functions
*/
UCLASS(MinimalApi)
class UPopcornFXNode_SetAttribute : public UPopcornFXNode_Attribute
{
	GENERATED_UCLASS_BODY()
private:
	virtual bool		SetupNativeFunctionCallByName(UK2Node_CallFunction *functionCall) override;
	virtual bool		SetupNativeFunctionCallByIndex(UK2Node_CallFunction *functionCall) override;
};
