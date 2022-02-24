//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "Nodes/PopcornFXNode_GetEventPayload.h"

#include "EdGraphSchema_K2.h"

#include "PopcornFXSDK.h"
#include "PopcornFXFunctions.h"
#include "PopcornFXEmitterComponent.h"
#include "PopcornFXSceneComponent.h"

#define LOCTEXT_NAMESPACE "PopcornFXNode_GetEventPayload"


UPopcornFXNode_GetEventPayload::UPopcornFXNode_GetEventPayload(const FObjectInitializer &objectInitializer)
	: Super(objectInitializer)
{
	m_NodeTitle = LOCTEXT("GetEventPayload", "Get Event Payload");
	m_NodeTooltip = LOCTEXT("GetEventPayloadNodeTooltip", "Retrieves a payload from the particle that triggered the event");
	m_MenuCategory = LOCTEXT("MenuCategory", "PopcornFX|Events");

	SetValuesPrefix("Out");

	m_CustomParameters.Add(TPair<FName, FName>(TPairInitializer<FName, FName>("PayloadName", UEdGraphSchema_K2::PC_Name)));
	m_CustomParameters.Add(TPair<FName, FName>(TPairInitializer<FName, FName>("InApplyGlobalScale", UEdGraphSchema_K2::PC_Boolean)));

	m_CustomParametersTooltip.Add("Payload Name\nName\n\nName of the payload to retrieve");
	m_PinFieldTypeTooltip = "Payload Type\nPinFieldType\n\nType of the payload to retrieve";
}

//---------------------------------------------------------------------------

UClass	*UPopcornFXNode_GetEventPayload::GetSelfPinClass() const
{
	return UPopcornFXEmitterComponent::StaticClass();
}

//----------------------------------------------------------------------------

bool	UPopcornFXNode_GetEventPayload::IsCompatibleWithGraph(UEdGraph const *graph) const
{
	const UEdGraphSchema_K2	*schema = Cast<UEdGraphSchema_K2>(graph->GetSchema());
	const bool				isConstructionScript = schema != null ? schema->IsConstructionScript(graph) : false;

	return !isConstructionScript && Super::IsCompatibleWithGraph(graph);
}

//----------------------------------------------------------------------------

bool		UPopcornFXNode_GetEventPayload::SetupNativeFunctionCall(UK2Node_CallFunction *functionCall)
{
	FName	functionName;

	switch (DataType())
	{
		case	EPopcornFXPinDataType::Float:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsFloat);
			break;
		case	EPopcornFXPinDataType::Float2:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsFloat2);
			break;
		case	EPopcornFXPinDataType::Float3:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsFloat3);
			break;
		case	EPopcornFXPinDataType::Float4:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsFloat4);
			break;
		case	EPopcornFXPinDataType::Int:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsInt);
			break;
		case	EPopcornFXPinDataType::Int2:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsInt2);
			break;
		case	EPopcornFXPinDataType::Int3:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsInt3);
			break;
		case	EPopcornFXPinDataType::Int4:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsInt4);
			break;
		case	EPopcornFXPinDataType::Bool:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsBool);
			break;
		case	EPopcornFXPinDataType::Bool2:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsBool2);
			break;
		case	EPopcornFXPinDataType::Bool3:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsBool3);
			break;
		case	EPopcornFXPinDataType::Bool4:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsBool4);
			break;
		case	EPopcornFXPinDataType::Vector2D:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsVector2D);
			break;
		case	EPopcornFXPinDataType::Vector:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsVector);
			break;
		case	EPopcornFXPinDataType::LinearColor:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsLinearColor);
			break;
		case	EPopcornFXPinDataType::Rotator:
			functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXFunctions, GetEventPayloadAsRotator);
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			return false;
	}

	if (!functionName.IsValid() || functionName.IsNone())
		return false;

	UFunction	*function = UPopcornFXFunctions::StaticClass()->FindFunctionByName(functionName);
	if (!PK_VERIFY(function != null))
		return false;

	functionCall->SetFromFunction(function);
	functionCall->AllocateDefaultPins();
	return true;
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
