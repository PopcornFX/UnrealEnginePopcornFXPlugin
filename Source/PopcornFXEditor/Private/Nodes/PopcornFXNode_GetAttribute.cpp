//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "Nodes/PopcornFXNode_GetAttribute.h"

#include "EdGraphSchema_K2.h"

#include "PopcornFXSDK.h"
#include "PopcornFXAttributeFunctions.h"
#include "PopcornFXEmitterComponent.h"

#define LOCTEXT_NAMESPACE "PopcornFXNode_GetAttribute"

//----------------------------------------------------------------------------

UPopcornFXNode_GetAttribute::UPopcornFXNode_GetAttribute(const FObjectInitializer &objectInitializer)
:	Super(objectInitializer)
{
	m_NodeTitle = LOCTEXT("GetAttribute", "Get Attribute");
	m_NodeTooltip = LOCTEXT("GetAttributeNodeTooltip", "Gets a specific attribute value");
	m_MenuCategory = LOCTEXT("MenuCategory", "PopcornFX|Attributes");

	SetValuesPrefix("Out");

	m_CustomParameters.Add(TPair<FName, FName>(TPairInitializer<FName, FName>("InAttributeIndex", UEdGraphSchema_K2::PC_Int)));
	m_CustomParameters.Add(TPair<FName, FName>(TPairInitializer<FName, FName>("InApplyGlobalScale", UEdGraphSchema_K2::PC_Boolean)));

	m_CustomParametersTooltip.Add("Attribute Index\nint\n\nIndex of the attribute");
	m_CustomParametersTooltip.Add("Apply Global Scale\nbool\n\nWhether or not to apply global scale to the Out Value");
	m_PinFieldTypeTooltip = "Attribute Type\nPinFieldType\n\nType of the attribute";
}

//---------------------------------------------------------------------------

UClass	*UPopcornFXNode_GetAttribute::GetSelfPinClass() const
{
	return UPopcornFXEmitterComponent::StaticClass();
}

//----------------------------------------------------------------------------

bool		UPopcornFXNode_GetAttribute::SetupNativeFunctionCall(UK2Node_CallFunction *functionCall)
{
	FName			functionName;
	switch (DataType())
	{
	case	EPopcornFXPinDataType::Float:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsFloat);
		break;
	case	EPopcornFXPinDataType::Float2:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsFloat2);
		break;
	case	EPopcornFXPinDataType::Float3:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsFloat3);
		break;
	case	EPopcornFXPinDataType::Float4:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsFloat4);
		break;

	case	EPopcornFXPinDataType::Int:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsInt);
		break;
	case	EPopcornFXPinDataType::Int2:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsInt2);
		break;
	case	EPopcornFXPinDataType::Int3:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsInt3);
		break;
	case	EPopcornFXPinDataType::Int4:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsInt4);
		break;

	case	EPopcornFXPinDataType::Bool:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsBool);
		break;
	case	EPopcornFXPinDataType::Bool2:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsBool2);
		break;
	case	EPopcornFXPinDataType::Bool3:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsBool3);
		break;
	case	EPopcornFXPinDataType::Bool4:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsBool4);
		break;

	case	EPopcornFXPinDataType::Vector2D:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsVector2D);
		break;
	case	EPopcornFXPinDataType::Vector:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsVector);
		break;
	case	EPopcornFXPinDataType::LinearColor:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsLinearColor);
		break;
	case	EPopcornFXPinDataType::Rotator:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, GetAttributeAsRotator);
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
