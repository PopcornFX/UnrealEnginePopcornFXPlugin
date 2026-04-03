//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "Nodes/PopcornFXNode_Attribute.h"
#include "PopcornFXNode_GetAttribute.generated.h"

/*
*	A custom Blueprint node that represents any of the GetAttribute() functions
*/
UCLASS(MinimalApi)
class UPopcornFXNode_GetAttribute : public UPopcornFXNode_Attribute
{
	GENERATED_UCLASS_BODY()
private:
	virtual bool		SetupNativeFunctionCallByName(UK2Node_CallFunction *functionCall) override;
	virtual bool		SetupNativeFunctionCallByIndex(UK2Node_CallFunction *functionCall) override;
};
