//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "Nodes/PopcornFXNode_WriteGridValues.h"

#include "PopcornFXSDK.h"

#define LOCTEXT_NAMESPACE "PopcornFXNode_WriteGridValues"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXWriteGridValues, Log, All);

//----------------------------------------------------------------------------

UPopcornFXNode_WriteGridValues::UPopcornFXNode_WriteGridValues(const FObjectInitializer &objectInitializer)
: Super(objectInitializer)
{
	m_ValueDirection = EGPD_Input;
	m_DataType = EPopcornFXGridDataType::Float;
	m_NodeTitle = LOCTEXT("WriteGridValues", "Write Grid Values");
	m_NodeTooltip = LOCTEXT("WriteGridValuesNodeTooltip", "Write grid values. Only works with CPU simulated particles");
	m_MenuCategory = LOCTEXT("MenuCategory", "PopcornFX|Attributes");

	SetValuesPrefix("In");
}

//----------------------------------------------------------------------------

bool		UPopcornFXNode_WriteGridValues::SetupNativeFunctionCall(UK2Node_CallFunction *functionCall)
{
	FName			functionName;
	switch (DataType())
	{
	case	EPopcornFXGridDataType::Float:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, WriteGridFloatValues);
		break;
	case	EPopcornFXGridDataType::Float2:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, WriteGridFloat2Values);
		break;
	case	EPopcornFXGridDataType::Float3:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, WriteGridFloat3Values);
		break;
	case	EPopcornFXGridDataType::Float4:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, WriteGridFloat4Values);
		break;

	case	EPopcornFXGridDataType::Int:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, WriteGridIntValues);
		break;
	case	EPopcornFXGridDataType::Int2:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, WriteGridInt2Values);
		break;
	case	EPopcornFXGridDataType::Int3:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, WriteGridInt3Values);
		break;
	case	EPopcornFXGridDataType::Int4:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, WriteGridInt4Values);
		break;
	}
	if (!functionName.IsValid() || functionName.IsNone())
		return false;
	UFunction *function = UPopcornFXAttributeSamplerGrid::StaticClass()->FindFunctionByName(functionName);
	if (!PK_VERIFY(function != null))
		return false;
	functionCall->SetFromFunction(function);
	functionCall->AllocateDefaultPins();
	return true;
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
