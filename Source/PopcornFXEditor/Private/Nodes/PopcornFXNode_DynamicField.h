//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "K2Node_CallFunction.h"
#include "PopcornFXTypes.h"

#include "PopcornFXNode_DynamicField.generated.h"

// DEPRECATED
UENUM(BlueprintType)
namespace	EPopcornFXPinFieldType
{
	enum	Type
	{
		Float,
		Float2,
		Float3,
		Float4,
		Int,
		Int2,
		Int3,
		Int4,
		Bool,
		Bool2,
		Bool3,
		Bool4,
		Vector2D UMETA(DisplayName="Vector2D (Float2)"),
		Vector UMETA(DisplayName="Vector (Float3)"),
		LinearColor UMETA(DisplayName="LinearColor (Float4)"),
		Rotator,
	};
}

UCLASS(Abstract)
class UPopcornFXNode_DynamicField : public UK2Node
{
	GENERATED_UCLASS_BODY()

private:
	virtual void	Serialize(FArchive& Ar) override;
	virtual void	PostLoad() override;

	virtual void		AllocateDefaultPins() override;
	virtual void		PostReconstructNode() override;
	virtual FText		GetNodeTitle(ENodeTitleType::Type titleType) const override { return m_NodeTitle; }
	virtual FText		GetTooltipText() const override { return m_NodeTooltip; }
	virtual void		PinDefaultValueChanged(UEdGraphPin *pin) override;
	virtual FSlateIcon	GetIconAndTint(FLinearColor& OutColor) const override;
	virtual void		GetMenuActions(FBlueprintActionDatabaseRegistrar &actionRegistrar) const override;
	virtual FText		GetMenuCategory() const override { return m_MenuCategory; }
	virtual bool		NodeCausesStructuralBlueprintChange() const { return true; }
	virtual void		ExpandNode(class FKismetCompilerContext &compilerContext, UEdGraph *sourceGraph) override;

private:
	bool			MovePinByName(class FKismetCompilerContext &compilerContext, UK2Node_CallFunction *nativeFunctionCall, const FName &name, bool required);
	void			BreakAndHidePinByName(const FName &name);

protected:
	EPopcornFXPinDataType::Type			DataType() const { return m_DataType; }
	virtual bool						SetupNativeFunctionCall(UK2Node_CallFunction *functionCall) { check(false); return false; }
	virtual UClass						*GetSelfPinClass() const { check(false); return nullptr; }

	void			SetValuesPrefix(const FString &prefix);

protected:
	TEnumAsByte<EEdGraphPinDirection>	m_ValueDirection;

	FText		m_NodeTitle;
	FText		m_NodeTooltip;
	FText		m_MenuCategory;

	FName								m_ValueName;
	FName								m_ValueNameX;
	FName								m_ValueNameY;
	FName								m_ValueNameZ;
	FName								m_ValueNameW;
	TArray<TPair<FName, FName> >		m_CustomParameters;

	TArray<FString>						m_CustomParametersTooltip;
	FString								m_PinFieldTypeTooltip;

private:
	UPROPERTY()
	TEnumAsByte<EPopcornFXPinDataType::Type>	m_DataType;

	UPROPERTY()
	TEnumAsByte<EPopcornFXPinFieldType::Type>	m_PinType_DEPRECATED;

	UPROPERTY()
	uint32			m_BreakValue_DEPRECATED : 1;

	UPROPERTY()
	uint32			m_ParticleFieldType_DEPRECATED;
};
