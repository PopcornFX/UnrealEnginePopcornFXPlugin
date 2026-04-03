//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "Nodes/PopcornFXNode_DynamicField.h"
#include "PopcornFXNode_Attribute.generated.h"


UENUM(BlueprintType)
namespace EPopcornFXAttributeFindMode
{
	enum	Type
	{
		ByName,
		ByIndex,
	};
}

/*
*	A custom Blueprint node that represents either a GetAttribute() or a SetAttribute() function
*/
UCLASS(Abstract)
class UPopcornFXNode_Attribute : public UPopcornFXNode_DynamicField
{
	GENERATED_UCLASS_BODY()
private:
	virtual bool		SetupNativeFunctionCall(UK2Node_CallFunction *functionCall) override;
	virtual bool		SetupNativeFunctionCallByName(UK2Node_CallFunction *functionCall) { check(false); return false; }
	virtual bool		SetupNativeFunctionCallByIndex(UK2Node_CallFunction *functionCall) { check(false); return false; }

	virtual void		AllocateDefaultPins() override;
	virtual void		PostReconstructNode() override;
	virtual void		PinDefaultValueChanged(UEdGraphPin *pin) override;
	virtual void		AutowireNewNode(UEdGraphPin *FromPin) override;
	virtual UClass		*GetSelfPinClass() const override;

	FName				GetFindModeName() const;

protected:
	FName				m_FindByName;
	FName				m_FindByIndex;

protected:
	UPROPERTY()
	TEnumAsByte<EPopcornFXAttributeFindMode::Type>	m_FindMode;
};
