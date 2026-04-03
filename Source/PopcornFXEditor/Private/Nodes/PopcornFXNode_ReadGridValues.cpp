//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#include "Nodes/PopcornFXNode_ReadGridValues.h"

#define LOCTEXT_NAMESPACE "PopcornFXNode_WriteGridValues"

#include "PopcornFXSDK.h"

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXReadGridValues, Log, All);

//----------------------------------------------------------------------------

UPopcornFXNode_ReadGridValues::UPopcornFXNode_ReadGridValues(const FObjectInitializer &objectInitializer)
: Super(objectInitializer)
{
	m_ValueDirection = EGPD_Output;
	m_DataType = EPopcornFXGridDataType::Float;
	m_NodeTitle = LOCTEXT("ReadGridValues", "Read Grid Values");
	m_NodeTooltip = LOCTEXT("ReadGridValuesNodeTooltip", "Read grid values. Only works with CPU simulated particles");
	m_MenuCategory = LOCTEXT("MenuCategory", "PopcornFX|Attributes");

	SetValuesPrefix("Out");
}

//----------------------------------------------------------------------------

bool		UPopcornFXNode_ReadGridValues::SetupNativeFunctionCall(UK2Node_CallFunction *functionCall)
{
	FName			functionName;
	switch (DataType())
	{
	case	EPopcornFXGridDataType::Float:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, ReadGridFloatValues);
		break;
	case	EPopcornFXGridDataType::Float2:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, ReadGridFloat2Values);
		break;
	case	EPopcornFXGridDataType::Float3:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, ReadGridFloat3Values);
		break;
	case	EPopcornFXGridDataType::Float4:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, ReadGridFloat4Values);
		break;

	case	EPopcornFXGridDataType::Int:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, ReadGridIntValues);
		break;
	case	EPopcornFXGridDataType::Int2:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, ReadGridInt2Values);
		break;
	case	EPopcornFXGridDataType::Int3:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, ReadGridInt3Values);
		break;
	case	EPopcornFXGridDataType::Int4:
		functionName = GET_FUNCTION_NAME_CHECKED(UPopcornFXAttributeSamplerGrid, ReadGridInt4Values);
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
