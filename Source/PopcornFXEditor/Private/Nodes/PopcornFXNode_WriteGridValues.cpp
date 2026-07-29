//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#include "Nodes/PopcornFXNode_WriteGridValues.h"

#include "PopcornFXSDK.h"
#include "PopcornFXAttributeSamplersFunctions.h"

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
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplersFunctions, WriteGridFloatValues);
		break;
	case	EPopcornFXGridDataType::Float2:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplersFunctions, WriteGridFloat2Values);
		break;
	case	EPopcornFXGridDataType::Float3:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplersFunctions, WriteGridFloat3Values);
		break;
	case	EPopcornFXGridDataType::Float4:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplersFunctions, WriteGridFloat4Values);
		break;

	case	EPopcornFXGridDataType::Int:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplersFunctions, WriteGridIntValues);
		break;
	case	EPopcornFXGridDataType::Int2:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplersFunctions, WriteGridInt2Values);
		break;
	case	EPopcornFXGridDataType::Int3:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplersFunctions, WriteGridInt3Values);
		break;
	case	EPopcornFXGridDataType::Int4:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplersFunctions, WriteGridInt4Values);
		break;
	}
	if (!functionName.IsValid() || functionName.IsNone())
		return false;
	UFunction *function = UPopcornFXAttributeSamplersFunctions::StaticClass()->FindFunctionByName(functionName);
	if (!PK_VERIFY(function != null))
		return false;
	functionCall->SetFromFunction(function);
	functionCall->AllocateDefaultPins();
	return true;
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
