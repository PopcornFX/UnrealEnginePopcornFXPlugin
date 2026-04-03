//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#include "Nodes/PopcornFXNode_SetAttribute.h"

#include "EdGraphSchema_K2.h"

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
	m_ValueDirection = EGPD_Input;

	SetValuesPrefix("In");

	m_CustomParameters.Add(TPair<FName, FName>(TPairInitializer<FName, FName>("InApplyGlobalScale", UEdGraphSchema_K2::PC_Boolean)));

	m_CustomParametersTooltip.Add("Apply Global Scale\nbool\n\nWhether or not to apply global scale to the In Value");
}

//----------------------------------------------------------------------------

bool		UPopcornFXNode_SetAttribute::SetupNativeFunctionCallByName(UK2Node_CallFunction *functionCall)
{
	FName			functionName;
	switch (DataType())
	{
	case	EPopcornFXPinDataType::Float:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsFloatByName);
		break;
	case	EPopcornFXPinDataType::Float2:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsFloat2ByName);
		break;
	case	EPopcornFXPinDataType::Float3:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsFloat3ByName);
		break;
	case	EPopcornFXPinDataType::Float4:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsFloat4ByName);
		break;

	case	EPopcornFXPinDataType::Int:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsIntByName);
		break;
	case	EPopcornFXPinDataType::Int2:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsInt2ByName);
		break;
	case	EPopcornFXPinDataType::Int3:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsInt3ByName);
		break;
	case	EPopcornFXPinDataType::Int4:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsInt4ByName);
		break;

	case	EPopcornFXPinDataType::Bool:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsBoolByName);
		break;
	case	EPopcornFXPinDataType::Bool2:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsBool2ByName);
		break;
	case	EPopcornFXPinDataType::Bool3:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsBool3ByName);
		break;
	case	EPopcornFXPinDataType::Bool4:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsBool4ByName);
		break;

	case	EPopcornFXPinDataType::Vector2D:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsVector2DByName);
		break;
	case	EPopcornFXPinDataType::Vector:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsVectorByName);
		break;
	case	EPopcornFXPinDataType::LinearColor:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsLinearColorByName);
		break;
	case	EPopcornFXPinDataType::Rotator:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeFunctions, SetAttributeAsRotatorByName);
		break;
	}
	if (!functionName.IsValid() || functionName.IsNone())
		return false;
	UFunction	*function = UPopcornFXAttributeFunctions::StaticClass()->FindFunctionByName(functionName);
	if (!PK_VERIFY(function != null))
		return false;
	functionCall->SetFromFunction(function);
	functionCall->AllocateDefaultPins();
	return true;
}

//----------------------------------------------------------------------------

bool		UPopcornFXNode_SetAttribute::SetupNativeFunctionCallByIndex(UK2Node_CallFunction *functionCall)
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
	UFunction *function = UPopcornFXAttributeFunctions::StaticClass()->FindFunctionByName(functionName);
	if (!PK_VERIFY(function != null))
		return false;
	functionCall->SetFromFunction(function);
	functionCall->AllocateDefaultPins();
	return true;
}

//----------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
