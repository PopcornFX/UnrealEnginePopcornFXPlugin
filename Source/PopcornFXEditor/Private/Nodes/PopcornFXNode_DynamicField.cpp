//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "Nodes/PopcornFXNode_DynamicField.h"

#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"
#include "ScopedTransaction.h"
#include "GraphEditorSettings.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"

#include "PopcornFXSDK.h"
#include "PopcornFXCustomVersion.h"
#include "PopcornFXAttributeFunctions.h"
#include "PopcornFXEmitterComponent.h"

#define LOCTEXT_NAMESPACE "PopcornFXNode_DynamicField"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXNode, Log, All);

//----------------------------------------------------------------------------

namespace	PopcornFXPinDataType
{
	uint32		RequiredPinNum(EPopcornFXPinDataType::Type type)
	{
		switch (type)
		{
		case EPopcornFXPinDataType::Float:
		case EPopcornFXPinDataType::Int:
		case EPopcornFXPinDataType::Bool:
		case EPopcornFXPinDataType::Vector2D:
		case EPopcornFXPinDataType::Vector:
		case EPopcornFXPinDataType::LinearColor:
		case EPopcornFXPinDataType::Rotator:
			return 1;
		case EPopcornFXPinDataType::Float2:
		case EPopcornFXPinDataType::Int2:
		case EPopcornFXPinDataType::Bool2:
			return 2;
		case EPopcornFXPinDataType::Float3:
		case EPopcornFXPinDataType::Int3:
		case EPopcornFXPinDataType::Bool3:
			return 3;
		case EPopcornFXPinDataType::Float4:
		case EPopcornFXPinDataType::Int4:
		case EPopcornFXPinDataType::Bool4:
			return 4;
		}
		checkNoEntry();
		return 0;
	}

	bool		CanBeGlobalScaled(EPopcornFXPinDataType::Type type)
	{
		switch (type)
		{
		case EPopcornFXPinDataType::Float:
		case EPopcornFXPinDataType::Float2:
		case EPopcornFXPinDataType::Float3:
		case EPopcornFXPinDataType::Float4:
		case EPopcornFXPinDataType::Vector2D:
		case EPopcornFXPinDataType::Vector:
			return true;
		case EPopcornFXPinDataType::Int:
		case EPopcornFXPinDataType::Int2:
		case EPopcornFXPinDataType::Int3:
		case EPopcornFXPinDataType::Int4:
		case EPopcornFXPinDataType::Bool:
		case EPopcornFXPinDataType::Bool2:
		case EPopcornFXPinDataType::Bool3:
		case EPopcornFXPinDataType::Bool4:
		case EPopcornFXPinDataType::LinearColor:
		case EPopcornFXPinDataType::Rotator:
			return false;
		}
		checkNoEntry()
		return false;
	}

#define _GET_BASE_STRUCT(__type)		TBaseStructure<__type>::Get()

	bool		GetGraphPinsType(EPopcornFXPinDataType::Type type, FEdGraphPinType &outPinType)
	{
		switch (type)
		{
		case EPopcornFXPinDataType::Float:
		case EPopcornFXPinDataType::Float2:
		case EPopcornFXPinDataType::Float3:
		case EPopcornFXPinDataType::Float4:
#if (ENGINE_MAJOR_VERSION == 5)
			outPinType.PinCategory = UEdGraphSchema_K2::PC_Real;
#else
			outPinType.PinCategory = UEdGraphSchema_K2::PC_Float;
#endif // (ENGINE_MAJOR_VERSION == 5)
			return true;
		case EPopcornFXPinDataType::Int:
		case EPopcornFXPinDataType::Int2:
		case EPopcornFXPinDataType::Int3:
		case EPopcornFXPinDataType::Int4:
			outPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
			return true;
		case EPopcornFXPinDataType::Bool:
		case EPopcornFXPinDataType::Bool2:
		case EPopcornFXPinDataType::Bool3:
		case EPopcornFXPinDataType::Bool4:
			outPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
			return true;
		case EPopcornFXPinDataType::Vector2D:
			outPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			outPinType.PinSubCategoryObject = _GET_BASE_STRUCT(FVector2D);
			return true;
		case EPopcornFXPinDataType::Vector:
			outPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			outPinType.PinSubCategoryObject = _GET_BASE_STRUCT(FVector);
			return true;
		case EPopcornFXPinDataType::LinearColor:
			outPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			outPinType.PinSubCategoryObject = _GET_BASE_STRUCT(FLinearColor);
			return true;
		case EPopcornFXPinDataType::Rotator:
			outPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			outPinType.PinSubCategoryObject = _GET_BASE_STRUCT(FRotator);
			return true;
		}
		checkNoEntry()
		return false;
	}

#undef _GET_BASE_STRUCT

	// Removes leading "EPopcornFXPinFieldType::" if nessecary for Pin value
	FString		GetPinValueName(EPopcornFXPinDataType::Type value)
	{
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		static const TCHAR	kEnumName[] = TEXT("/Script/PopcornFX.EPopcornFXPinDataType");
		UEnum				*pinTypeEnum = FindObject<UEnum>(null, kEnumName, true);
#else
		static const TCHAR	kEnumName[] = TEXT("EPopcornFXPinDataType");
		UEnum				*pinTypeEnum = FindObject<UEnum>(ANY_PACKAGE, kEnumName, true);
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		check(pinTypeEnum);
		FString		name = pinTypeEnum->GetNameByValue(value).ToString();
		static const TCHAR	kEnumNameDoubleColon[] = TEXT("EPopcornFXPinDataType::");
		if (name.Len() > PK_ARRAY_COUNT(kEnumNameDoubleColon) && name.StartsWith(kEnumNameDoubleColon))
			name = name.RightChop(PK_ARRAY_COUNT(kEnumNameDoubleColon) - 1);
		return name;
	}

	FName	GetPinTypeName()
	{
		static FName	pinTypeName = "PinType";
		return pinTypeName;
	}

	FName	GetSelfName()
	{
		static FName	inSelfName = "InSelf";
		return inSelfName;
	}
}

//----------------------------------------------------------------------------

UPopcornFXNode_DynamicField::UPopcornFXNode_DynamicField(const FObjectInitializer &objectInitializer)
:	Super(objectInitializer)
,	m_ValueDirection(EGPD_Output)
,	m_DataType(EPopcornFXPinDataType::Float)
,	m_PinType_DEPRECATED(EPopcornFXPinFieldType::Float)
,	m_BreakValue_DEPRECATED(false)
,	m_ParticleFieldType_DEPRECATED(uint32(-1)) // old default value is same as new default m_PinType(Float)
{
}

//----------------------------------------------------------------------------

void	UPopcornFXNode_DynamicField::SetValuesPrefix(const FString &prefix)
{
	check(!prefix.IsEmpty());
	m_ValueName = *(prefix + "Value");
	m_ValueNameX = *(prefix + "ValueX");
	m_ValueNameY = *(prefix + "ValueY");
	m_ValueNameZ = *(prefix + "ValueZ");
	m_ValueNameW = *(prefix + "ValueW");
}

//----------------------------------------------------------------------------

void	UPopcornFXNode_DynamicField::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Create the execution pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// Create the input pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, TEXT(""), GetSelfPinClass(), PopcornFXPinDataType::GetSelfName());
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
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		UEnum		*pinTypeEnum = FindObject<UEnum>(null, TEXT("/Script/PopcornFX.EPopcornFXPinDataType"), true);
#else
		UEnum		*pinTypeEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EPopcornFXPinDataType"), true);
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		UEdGraphPin		*pinTypePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Byte, pinTypeEnum, PopcornFXPinDataType::GetPinTypeName());
		PK_ASSERT(pinTypePin != null);

		if (!m_PinFieldTypeTooltip.IsEmpty())
			pinTypePin->PinToolTip = m_PinFieldTypeTooltip;

		pinTypePin->bNotConnectable = true;

		const FString	&newValue = PopcornFXPinDataType::GetPinValueName(m_DataType);
		// graphSchema->TrySetDefaultValue(*pinTypePin, newValue); // !!! Will call ReconstructNode() !! infinite recurse !
		pinTypePin->DefaultValue = newValue;
	}

	const EEdGraphPinDirection	pinDirection = m_ValueDirection.GetValue();
	const uint32				pinCount = PopcornFXPinDataType::RequiredPinNum(m_DataType);
	FEdGraphPinType				pinsType;
	PopcornFXPinDataType::GetGraphPinsType(m_DataType, pinsType);

	if (pinCount <= 1)
	{
		CreatePin(pinDirection, pinsType, m_ValueName);
	}
	else
	{
		CreatePin(pinDirection, pinsType, m_ValueNameX);
		//if (pinCount >= 2)
		CreatePin(pinDirection, pinsType, m_ValueNameY);
		if (pinCount >= 3)
			CreatePin(pinDirection, pinsType, m_ValueNameZ);
		if (pinCount >= 4)
			CreatePin(pinDirection, pinsType, m_ValueNameW);
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXNode_DynamicField::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FPopcornFXCustomVersion::GUID);

	if (Ar.IsLoading())
	{
		//const int32		version = GetLinkerCustomVersion(FPopcornFXCustomVersion::GUID);
		//if (version < FPopcornFXCustomVersion::Node_ReplaceFieldTypeBySimplerPinType)
		if (m_ParticleFieldType_DEPRECATED != uint32(-1))
		{
			check(m_ParticleFieldType_DEPRECATED <= EPopcornFXParticleFieldType::Int4);
			EPopcornFXParticleFieldType::Type	fieldType = (EPopcornFXParticleFieldType::Type)m_ParticleFieldType_DEPRECATED;
			switch (fieldType)
			{
			case EPopcornFXParticleFieldType::Float:
				m_DataType = EPopcornFXPinDataType::Float;
				break;
			case EPopcornFXParticleFieldType::Float2:
				if (m_BreakValue_DEPRECATED)
					m_DataType = EPopcornFXPinDataType::Float2;
				else
					m_DataType = EPopcornFXPinDataType::Vector2D;
				break;
			case EPopcornFXParticleFieldType::Float3:
				if (m_BreakValue_DEPRECATED)
					m_DataType = EPopcornFXPinDataType::Float3;
				else
					m_DataType = EPopcornFXPinDataType::Vector;
				break;
			case EPopcornFXParticleFieldType::Float4:
				if (m_BreakValue_DEPRECATED)
					m_DataType = EPopcornFXPinDataType::Float4;
				else
					m_DataType = EPopcornFXPinDataType::LinearColor;
				break;
			case EPopcornFXParticleFieldType::Int:
				m_DataType = EPopcornFXPinDataType::Int;
				break;
			case EPopcornFXParticleFieldType::Int2:
				m_DataType = EPopcornFXPinDataType::Int2;
				break;
			case EPopcornFXParticleFieldType::Int3:
				m_DataType = EPopcornFXPinDataType::Int3;
				break;
			case EPopcornFXParticleFieldType::Int4:
				m_DataType = EPopcornFXPinDataType::Int4;
				break;
			}
			UE_LOG(LogPopcornFXNode, Log, TEXT("%s: Upgraded BP Node %s from FieldType %s %d to %d"), *GetFullName(), *m_NodeTitle.ToString(), m_BreakValue_DEPRECATED ? TEXT("breaked") : TEXT("merged"), int32(m_ParticleFieldType_DEPRECATED), int32(m_PinType_DEPRECATED));

			m_ParticleFieldType_DEPRECATED = uint32(-1);
		}
		if (m_PinType_DEPRECATED != EPopcornFXPinFieldType::Float)
		{
			RemovePin(FindPinChecked(PopcornFXPinDataType::GetPinTypeName()));

			m_DataType = *reinterpret_cast<EPopcornFXPinDataType::Type*>(&m_PinType_DEPRECATED);

			UE_LOG(LogPopcornFXNode, Log, TEXT("%s: Upgraded BP Node %s from FieldType %d to %d (enum change)"), *GetFullName(), *m_NodeTitle.ToString(), int32(m_PinType_DEPRECATED), int32(m_DataType));
			m_PinType_DEPRECATED = EPopcornFXPinFieldType::Float;
		}
	}
}

//----------------------------------------------------------------------------

void	UPopcornFXNode_DynamicField::PostLoad()
{
	Super::PostLoad();
}

//----------------------------------------------------------------------------

void	UPopcornFXNode_DynamicField::PostReconstructNode()
{
	Super::PostReconstructNode();

	// Break and Hide unwanted pins

	const uint32				pinCount = PopcornFXPinDataType::RequiredPinNum(m_DataType);
	if (pinCount <= 1)
	{
		BreakAndHidePinByName(m_ValueNameX);
		BreakAndHidePinByName(m_ValueNameY);
		BreakAndHidePinByName(m_ValueNameZ);
		BreakAndHidePinByName(m_ValueNameW);
	}
	else
	{
		BreakAndHidePinByName(m_ValueName);
		//if (pinCount < 1) // false
		//	BreakAndHidePinByName(m_ValueNameX);
		if (pinCount < 2)
			BreakAndHidePinByName(m_ValueNameY);
		if (pinCount < 3)
			BreakAndHidePinByName(m_ValueNameZ);
		if (pinCount < 4)
			BreakAndHidePinByName(m_ValueNameW);
	}

	// InApplyGlobalScale can exists in m_CustomParameters
	if (!PopcornFXPinDataType::CanBeGlobalScaled(m_DataType))
		BreakAndHidePinByName("InApplyGlobalScale");
}

//----------------------------------------------------------------------------

FSlateIcon	UPopcornFXNode_DynamicField::GetIconAndTint(FLinearColor& OutColor) const
{
	const UGraphEditorSettings	*editorSettings = GetDefault<UGraphEditorSettings>();
	PK_ASSERT(editorSettings != null);
	OutColor = editorSettings->FunctionCallNodeTitleColor;
	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

//----------------------------------------------------------------------------

void	UPopcornFXNode_DynamicField::PinDefaultValueChanged(UEdGraphPin *pin)
{
	if (pin != FindPinChecked(PopcornFXPinDataType::GetPinTypeName()))
		return;

	if (!PK_VERIFY(!pin->DefaultValue.IsEmpty()))
		return;

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
	UEnum		*pinTypeEnum = FindObject<UEnum>(null, TEXT("/Script/PopcornFX.EPopcornFXPinDataType"), true);
#else
	UEnum		*pinTypeEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EPopcornFXPinDataType"), true);
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
	check(pinTypeEnum);
	int32		rawValue = pinTypeEnum->GetValueByName(FName(*pin->DefaultValue)); // GetValueByName is fine with or without "EPopcornFXPinDataType::"...
	if (!PK_VERIFY(rawValue != INDEX_NONE))
	{
		rawValue = EPopcornFXPinDataType::Float;
	}
	EPopcornFXPinDataType::Type	newValue = (EPopcornFXPinDataType::Type)rawValue;

	if (newValue == m_DataType)
		return; // no need to ReconstructNode()

	m_DataType = newValue;
	ReconstructNode();
}

//----------------------------------------------------------------------------

void	UPopcornFXNode_DynamicField::GetMenuActions(FBlueprintActionDatabaseRegistrar &actionRegistrar) const
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

void	UPopcornFXNode_DynamicField::ExpandNode(class FKismetCompilerContext &compilerContext, UEdGraph *sourceGraph)
{
	Super::ExpandNode(compilerContext, sourceGraph);

	// Force rebuild output category
	UEdGraphPin	*pinTypePin = FindPinChecked(PopcornFXPinDataType::GetPinTypeName());
	PK_ASSERT(pinTypePin != null);
	if (pinTypePin->DefaultValue.IsEmpty())
	{
		compilerContext.MessageLog.Error(*LOCTEXT("PopcornFXExpandNodeError", "UPopcornFXNode_FieldDynamic: Pin Type not specified. @@").ToString(), this);
		BreakAllNodeLinks();
		return;
	}
	PK_ONLY_IF_ASSERTS({
			const FString	&value = PopcornFXPinDataType::GetPinValueName(m_DataType);
			PK_ASSERT(pinTypePin->DefaultValue == value);
	});

	UK2Node_CallFunction	*nativeFunctionCall = compilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, sourceGraph);
	if (!SetupNativeFunctionCall(nativeFunctionCall))
	{
		compilerContext.MessageLog.Error(*LOCTEXT("PopcornFXExpandNodeError", "UPopcornFXNode_FieldDynamic: Failed to recover native function call. @@").ToString(), this);
		BreakAllNodeLinks();
		return;
	}

	bool	success = true;

	success &= PK_VERIFY(MovePinByName(compilerContext, nativeFunctionCall, UEdGraphSchema_K2::PN_Execute, true));
	success &= PK_VERIFY(MovePinByName(compilerContext, nativeFunctionCall, UEdGraphSchema_K2::PN_Then, true));
	success &= PK_VERIFY(MovePinByName(compilerContext, nativeFunctionCall, PopcornFXPinDataType::GetSelfName(), true));

	const u32	paramCount = m_CustomParameters.Num();
	for (u32 iParam = 0; iParam < paramCount; ++iParam)
	{
		// m_CustomParameters are optional
		/*success &= */MovePinByName(compilerContext, nativeFunctionCall, m_CustomParameters[iParam].Key, false);
	}

	const uint32	pinCount = PopcornFXPinDataType::RequiredPinNum(m_DataType);

	if (pinCount <= 1)
	{
		success &= PK_VERIFY(MovePinByName(compilerContext, nativeFunctionCall, m_ValueName, true));
	}
	else
	{
		success &= PK_VERIFY(MovePinByName(compilerContext, nativeFunctionCall, m_ValueNameX, true));
		//if (pinCount >= 2)
		success &= PK_VERIFY(MovePinByName(compilerContext, nativeFunctionCall, m_ValueNameY, true));
		if (pinCount >= 3)
			success &= PK_VERIFY(MovePinByName(compilerContext, nativeFunctionCall, m_ValueNameZ, true));
		if (pinCount >= 4)
			success &= PK_VERIFY(MovePinByName(compilerContext, nativeFunctionCall, m_ValueNameW, true));
	}
	success &= PK_VERIFY(MovePinByName(compilerContext, nativeFunctionCall, UEdGraphSchema_K2::PN_ReturnValue, true));

	if (!success)
	{
		compilerContext.MessageLog.Error(*LOCTEXT("PopcornFXExpandNodeError", "UPopcornFXNode_FieldDynamic: Internal connection error - @@").ToString(), this);
	}

	BreakAllNodeLinks();
}

//----------------------------------------------------------------------------

bool	UPopcornFXNode_DynamicField::MovePinByName(class FKismetCompilerContext &compilerContext, UK2Node_CallFunction *nativeFunctionCall, const FName &name, bool required)
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

void	UPopcornFXNode_DynamicField::BreakAndHidePinByName(const FName &name)
{
	UEdGraphPin		*valuePin = FindPin(name);
	if (valuePin != null)
	{
		valuePin->BreakAllPinLinks();
		valuePin->bHidden = true;
	}
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
