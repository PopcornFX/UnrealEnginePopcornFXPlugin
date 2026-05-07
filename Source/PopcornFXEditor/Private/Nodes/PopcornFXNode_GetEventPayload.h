//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "Nodes/PopcornFXNode_DynamicField.h"
#include "PopcornFXNode_GetEventPayload.generated.h"

UCLASS(MinimalApi)
class UPopcornFXNode_GetEventPayload : public UPopcornFXNode_DynamicField
{
	GENERATED_UCLASS_BODY()
private:
	virtual bool		IsCompatibleWithGraph(UEdGraph const *graph) const override;
	virtual bool		SetupNativeFunctionCall(UK2Node_CallFunction *functionCall) override;
	virtual UClass		*GetSelfPinClass() const override;
};
