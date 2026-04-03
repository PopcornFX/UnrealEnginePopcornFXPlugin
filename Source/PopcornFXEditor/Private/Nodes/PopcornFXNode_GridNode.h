//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "K2Node_CallFunction.h"
#include "PopcornFXAttributeSamplerGrid.h"
#include "PopcornFXTypes.h"

#include "PopcornFXNode_GridNode.generated.h"

UCLASS()
class UPopcornFXNode_GridNode : public UK2Node
{
	GENERATED_UCLASS_BODY()

protected:
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

protected:
	void			BreakAndHidePinByName(const FName &name);
	bool			MovePinByName(class FKismetCompilerContext &compilerContext, UK2Node_CallFunction *nativeFunctionCall, const FName &name, bool required);
	bool			GetGraphPinsType(EPopcornFXGridDataType::Type type, FEdGraphPinType &outPinType);
	FString			GetPinValueName(EPopcornFXGridDataType::Type value);
	FName			GetPinTypeName();
	FName			GetSelfName();

protected:
	EPopcornFXGridDataType::Type	DataType() const { return m_DataType; }
	virtual bool					SetupNativeFunctionCall(UK2Node_CallFunction *functionCall) { return true; }
	virtual UClass					*GetSelfPinClass() const;

	void			SetValuesPrefix(const FString &prefix);

protected:
	TEnumAsByte<EEdGraphPinDirection>	m_ValueDirection;

	FText		m_NodeTitle;
	FText		m_NodeTooltip;
	FText		m_MenuCategory;

	FName								m_ValuesName;
	TArray<TPair<FName, FName> >		m_CustomParameters;

	TArray<FString>						m_CustomParametersTooltip;
	FString								m_PinFieldTypeTooltip;

	UPROPERTY()
	TEnumAsByte<EPopcornFXGridDataType::Type>	m_DataType;
};
