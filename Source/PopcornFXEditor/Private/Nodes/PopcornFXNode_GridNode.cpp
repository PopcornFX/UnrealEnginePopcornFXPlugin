//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#include "Nodes/PopcornFXNode_GridNode.h"

#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"
#include "ScopedTransaction.h"
#include "GraphEditorSettings.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"

#include "PopcornFXSDK.h"
#include "PopcornFXCustomVersion.h"

#define LOCTEXT_NAMESPACE "PopcornFXNode_GridNode"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXGridNode, Log, All);

//----------------------------------------------------------------------------

UPopcornFXNode_GridNode::UPopcornFXNode_GridNode(const FObjectInitializer &objectInitializer)
	: Super(objectInitializer)
{
}

//----------------------------------------------------------------------------

#define _GET_BASE_STRUCT(__type)		TBaseStructure<__type>::Get()

bool		UPopcornFXNode_GridNode::GetGraphPinsType(EPopcornFXGridDataType::Type type, FEdGraphPinType &outPinType)
{
	outPinType.ContainerType = EPinContainerType::Array;
	switch (type)
	{
	case EPopcornFXGridDataType::Float:
		outPinType.PinCategory = UEdGraphSchema_K2::PC_Real;
		outPinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
		return true;
	case EPopcornFXGridDataType::Float2:
		outPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		outPinType.PinSubCategoryObject = _GET_BASE_STRUCT(FVector2D);
		return true;
	case EPopcornFXGridDataType::Float3:
		outPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		outPinType.PinSubCategoryObject = _GET_BASE_STRUCT(FVector);
		return true;
	case EPopcornFXGridDataType::Float4:
		outPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		outPinType.PinSubCategoryObject = _GET_BASE_STRUCT(FVector4);
		return true;
	case EPopcornFXGridDataType::Int:
		outPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
		return true;
	case EPopcornFXGridDataType::Int2:
		outPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		outPinType.PinSubCategoryObject = _GET_BASE_STRUCT(FIntPoint);
		return true;
	case EPopcornFXGridDataType::Int3:
		outPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		outPinType.PinSubCategoryObject = _GET_BASE_STRUCT(FIntVector);
		return true;
	case EPopcornFXGridDataType::Int4:
		outPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		outPinType.PinSubCategoryObject = _GET_BASE_STRUCT(FIntVector4);
		return true;
	}
	checkNoEntry()
	return false;
}

#undef _GET_BASE_STRUCT

// Removes leading "EPopcornFXGridDataType::" if nessecary for Pin value
FString		UPopcornFXNode_GridNode::GetPinValueName(EPopcornFXGridDataType::Type value)
{
	static const TCHAR	kEnumName[] = TEXT("/Script/PopcornFX.EPopcornFXGridDataType");
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 7)
	UEnum				*pinTypeEnum = FindObject<UEnum>(null, kEnumName, EFindObjectFlags::ExactClass);
#else
	UEnum *pinTypeEnum = FindObject<UEnum>(null, kEnumName, true);
#endif
	check(pinTypeEnum);
	FString		name = pinTypeEnum->GetNameByValue(value).ToString();
	static const TCHAR	kEnumNameDoubleColon[] = TEXT("EPopcornFXGridDataType::");
	if (name.Len() > PK_ARRAY_COUNT(kEnumNameDoubleColon) && name.StartsWith(kEnumNameDoubleColon))
		name = name.RightChop(PK_ARRAY_COUNT(kEnumNameDoubleColon) - 1);
	return name;
}

FName	UPopcornFXNode_GridNode::GetPinTypeName()
{
	static FName	pinTypeName = "DataType";
	return pinTypeName;
}

FName	UPopcornFXNode_GridNode::GetSelfName()
{
	static FName	inSelfName = "InGrid";
	return inSelfName;
}

//----------------------------------------------------------------------------

void	UPopcornFXNode_GridNode::SetValuesPrefix(const FString &prefix)
{
	check(!prefix.IsEmpty());
	m_ValuesName = *(prefix + "Values");
}

//----------------------------------------------------------------------------

void	UPopcornFXNode_GridNode::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Create the execution pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// Create the input pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, TEXT(""), GetSelfPinClass(), GetSelfName());
	const u32	paramCount = m_CustomParameters.Num();
	for (u32 iParam = 0; iParam < paramCount; ++iParam)
	{
		UEdGraphPin	*pin = CreatePin(EGPD_Input, m_CustomParameters[iParam].Value, m_CustomParameters[iParam].Key);

		PK_ASSERT(pin != null);

		if (iParam < (u32)m_CustomParametersTooltip.Num() && !m_CustomParametersTooltip[iParam].IsEmpty())
			pin->PinToolTip = m_CustomParametersTooltip[iParam];
	}

	// Create the return value pin
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, UEdGraphSchema_K2::PN_ReturnValue);

	{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 7)
		UEnum		*pinTypeEnum = FindObject<UEnum>(null, TEXT("/Script/PopcornFX.EPopcornFXGridDataType"), EFindObjectFlags::ExactClass);
#else
		UEnum *pinTypeEnum = FindObject<UEnum>(null, TEXT("/Script/PopcornFX.EPopcornFXGridDataType"), true);
#endif
		UEdGraphPin		*pinTypePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Byte, pinTypeEnum, GetPinTypeName());
		PK_ASSERT(pinTypePin != null);

		if (!m_PinFieldTypeTooltip.IsEmpty())
			pinTypePin->PinToolTip = m_PinFieldTypeTooltip;

		pinTypePin->bNotConnectable = true;

		const FString	&newValue = GetPinValueName(m_DataType);
		pinTypePin->DefaultValue = newValue;
	}

	// Create array output pin
	const EEdGraphPinDirection	pinDirection = m_ValueDirection.GetValue();
	FEdGraphPinType				pinsType;
	GetGraphPinsType(m_DataType, pinsType);

	CreatePin(pinDirection, pinsType, m_ValuesName);
}

//----------------------------------------------------------------------------

void	UPopcornFXNode_GridNode::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FPopcornFXCustomVersion::GUID);

}

//----------------------------------------------------------------------------

void	UPopcornFXNode_GridNode::PostLoad()
{
	Super::PostLoad();
}

//----------------------------------------------------------------------------

void	UPopcornFXNode_GridNode::PostReconstructNode()
{
	Super::PostReconstructNode();
}

//----------------------------------------------------------------------------

FSlateIcon	UPopcornFXNode_GridNode::GetIconAndTint(FLinearColor& OutColor) const
{
	const UGraphEditorSettings	*editorSettings = GetDefault<UGraphEditorSettings>();
	PK_ASSERT(editorSettings != null);
	OutColor = editorSettings->FunctionCallNodeTitleColor;
	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

//----------------------------------------------------------------------------

void	UPopcornFXNode_GridNode::PinDefaultValueChanged(UEdGraphPin *pin)
{
	if (pin != FindPinChecked(GetPinTypeName()))
		return;

	if (!PK_VERIFY(!pin->DefaultValue.IsEmpty()))
		return;

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 7)
	UEnum		*pinTypeEnum = FindObject<UEnum>(null, TEXT("/Script/PopcornFX.EPopcornFXGridDataType"), EFindObjectFlags::ExactClass);
#else
	UEnum *pinTypeEnum = FindObject<UEnum>(null, TEXT("/Script/PopcornFX.EPopcornFXGridDataType"), true);
#endif
	check(pinTypeEnum);
	int32		rawValue = pinTypeEnum->GetValueByName(FName(*pin->DefaultValue)); // GetValueByName is fine with or without "EPopcornFXGridDataType::"...
	if (!PK_VERIFY(rawValue != INDEX_NONE))
	{
		rawValue = EPopcornFXGridDataType::Float;
	}
	EPopcornFXGridDataType::Type	newValue = (EPopcornFXGridDataType::Type)rawValue;

	if (newValue == m_DataType)
		return; // no need to ReconstructNode()

	m_DataType = newValue;
	ReconstructNode();
}

//----------------------------------------------------------------------------

void	UPopcornFXNode_GridNode::GetMenuActions(FBlueprintActionDatabaseRegistrar &actionRegistrar) const
{
	if (m_NodeTitle.IsEmpty() || m_NodeTooltip.IsEmpty() || m_MenuCategory.IsEmpty())
		return;
	const UClass	*actionKey = GetClass();
	if (actionRegistrar.IsOpenForRegistration(actionKey))
	{
		UBlueprintNodeSpawner	*nodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		PK_ASSERT(nodeSpawner != null);
		actionRegistrar.AddBlueprintAction(actionKey, nodeSpawner);
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXNode_GridNode::ExpandNode(class FKismetCompilerContext &compilerContext, UEdGraph *sourceGraph)
{
	Super::ExpandNode(compilerContext, sourceGraph);

	// Force rebuild output category
	UEdGraphPin	*pinTypePin = FindPinChecked(GetPinTypeName());
	PK_ASSERT(pinTypePin != null);
	if (pinTypePin->DefaultValue.IsEmpty())
	{
		compilerContext.MessageLog.Error(*LOCTEXT("PopcornFXExpandNodeError", "UPopcornFXNode_GridNode: Pin Type not specified. @@").ToString(), this);
		BreakAllNodeLinks();
		return;
	}
	PK_ONLY_IF_ASSERTS({
			const FString	&value = GetPinValueName(m_DataType);
			PK_ASSERT(pinTypePin->DefaultValue == value);
	});

	UK2Node_CallFunction	*nativeFunctionCall = compilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, sourceGraph);
	if (!SetupNativeFunctionCall(nativeFunctionCall))
	{
		compilerContext.MessageLog.Error(*LOCTEXT("PopcornFXExpandNodeError", "UPopcornFXNode_GridNode: Failed to recover native function call. @@").ToString(), this);
		BreakAllNodeLinks();
		return;
	}

	bool	success = true;

	success &= PK_VERIFY(MovePinByName(compilerContext, nativeFunctionCall, UEdGraphSchema_K2::PN_Execute, true));
	success &= PK_VERIFY(MovePinByName(compilerContext, nativeFunctionCall, UEdGraphSchema_K2::PN_Then, true));
	success &= PK_VERIFY(MovePinByName(compilerContext, nativeFunctionCall, GetSelfName(), true));

	const u32	paramCount = m_CustomParameters.Num();
	for (u32 iParam = 0; iParam < paramCount; ++iParam)
	{
		// m_CustomParameters are optional
		/*success &= */MovePinByName(compilerContext, nativeFunctionCall, m_CustomParameters[iParam].Key, false);
	}

	success &= PK_VERIFY(MovePinByName(compilerContext, nativeFunctionCall, m_ValuesName, true));

	if (!success)
	{
		compilerContext.MessageLog.Error(*LOCTEXT("PopcornFXExpandNodeError", "UPopcornFXNode_GridNode: Internal connection error - @@").ToString(), this);
	}

	BreakAllNodeLinks();
}

//----------------------------------------------------------------------------

bool	UPopcornFXNode_GridNode::MovePinByName(class FKismetCompilerContext &compilerContext, UK2Node_CallFunction *nativeFunctionCall, const FName &name, bool required)
{
	UEdGraphPin		*outPin = nativeFunctionCall->FindPin(name);
	PK_ASSERT(outPin != null || !required);
	if (outPin == null)
		return false;

	UEdGraphPin		*inPin = FindPin(name);
	PK_ASSERT((inPin != null && !inPin->bHidden) || !required);
	if (inPin == null || inPin->bHidden)
		return false;

	bool	success = compilerContext.MovePinLinksToIntermediate(*inPin, *outPin).CanSafeConnect();
	PK_ASSERT(success || !required);
	return success;
}

//----------------------------------------------------------------------------

void	UPopcornFXNode_GridNode::BreakAndHidePinByName(const FName &name)
{
	UEdGraphPin		*valuePin = FindPin(name);
	if (valuePin != null)
	{
		valuePin->BreakAllPinLinks();
		valuePin->bHidden = true;
	}
}

//---------------------------------------------------------------------------

UClass *UPopcornFXNode_GridNode::GetSelfPinClass() const
{
	return UPopcornFXAttributeSamplerGrid::StaticClass();
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
