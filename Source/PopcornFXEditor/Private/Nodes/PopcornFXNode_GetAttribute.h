//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "Nodes/PopcornFXNode_DynamicField.h"
#include "PopcornFXNode_GetAttribute.generated.h"

UCLASS(MinimalApi)
class UPopcornFXNode_GetAttribute : public UPopcornFXNode_DynamicField
{
	GENERATED_UCLASS_BODY()
private:
	virtual bool		SetupNativeFunctionCall(UK2Node_CallFunction *functionCall) override;
	virtual UClass		*GetSelfPinClass() const override;
};
