//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "Nodes/PopcornFXNode_SetAttribute.h"

#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"

#include "PopcornFXSDK.h"

#include "PopcornFXAttributeFunctions.h"
#include "PopcornFXEmitterComponent.h"

#define LOCTEXT_NAMESPACE "PopcornFXNode_SetAttribute"

//----------------------------------------------------------------------------

UPopcornFXNode_SetAttribute::UPopcornFXNode_SetAttribute(const FObjectInitializer &objectInitializer)
:	Super(objectInitializer)
{
	m_NodeTitle = LOCTEXT("SetAttribute", "Set Attribute");
	m_NodeTooltip = LOCTEXT("SetAttributeNodeTooltip", "Sets a specific attribute value");
	m_MenuCategory = LOCTEXT("MenuCategory", "PopcornFX|Attributes");
	m_ValueDirection = EGPD_Input;

	SetValuesPrefix("In");

	m_CustomParameters.Add(TPair<FName, FName>(TPairInitializer<FName, FName>("InAttributeIndex", UEdGraphSchema_K2::PC_Int)));
	m_CustomParameters.Add(TPair<FName, FName>(TPairInitializer<FName, FName>("InApplyGlobalScale", UEdGraphSchema_K2::PC_Boolean)));

	m_CustomParametersTooltip.Add("Attribute Index\nint\n\nIndex of the attribute");
	m_CustomParametersTooltip.Add("Apply Global Scale\nbool\n\nWhether or not to apply global scale to the In Value");
	m_PinFieldTypeTooltip = "Attribute Type\nPinFieldType\n\nType of the attribute";
}

//---------------------------------------------------------------------------

UClass	*UPopcornFXNode_SetAttribute::GetSelfPinClass() const
{
	return UPopcornFXEmitterComponent::StaticClass();
}

//----------------------------------------------------------------------------

bool		UPopcornFXNode_SetAttribute::SetupNativeFunctionCall(UK2Node_CallFunction *functionCall)
{
	FName			functionName;
	switch (DataType())
	{
	case	EPopcornFXPinDataType::Float:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsFloat);
		break;
	case	EPopcornFXPinDataType::Float2:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsFloat2);
		break;
	case	EPopcornFXPinDataType::Float3:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsFloat3);
		break;
	case	EPopcornFXPinDataType::Float4:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsFloat4);
		break;

	case	EPopcornFXPinDataType::Int:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsInt);
		break;
	case	EPopcornFXPinDataType::Int2:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsInt2);
		break;
	case	EPopcornFXPinDataType::Int3:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsInt3);
		break;
	case	EPopcornFXPinDataType::Int4:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsInt4);
		break;

	case	EPopcornFXPinDataType::Bool:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsBool);
		break;
	case	EPopcornFXPinDataType::Bool2:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsBool2);
		break;
	case	EPopcornFXPinDataType::Bool3:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsBool3);
		break;
	case	EPopcornFXPinDataType::Bool4:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsBool4);
		break;

	case	EPopcornFXPinDataType::Vector2D:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsVector2D);
		break;
	case	EPopcornFXPinDataType::Vector:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsVector);
		break;
	case	EPopcornFXPinDataType::LinearColor:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsLinearColor);
		break;
	case	EPopcornFXPinDataType::Rotator:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsRotator);
		break;
	}
	if (!functionName.IsValid() || functionName.IsNone())
		return false;
	UFunction	*function = UPopcornFXAttributeFunctions::StaticClass()->FindFunctionByName(functionName);
	if (!PK_VERIFY(function != null))
		return false;
	functionCall->SetFromFunction(function);
	//functionCall->FunctionReference.SetExternalMember(functionName, UPopcornFXAttributeFunctions::StaticClass());
	functionCall->AllocateDefaultPins();
	return true;
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
