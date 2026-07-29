#include "PopcornFXAttributeSamplerShapeAsset.h"

UPopcornFXAttributeSamplerShapeFactory::UPopcornFXAttributeSamplerShapeFactory(FObjectInitializer const &PCIP)
	: Super(PCIP)
{
	SupportedClass = UPopcornFXAttributeSamplerShapeAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}