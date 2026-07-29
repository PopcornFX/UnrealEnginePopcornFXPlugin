#include "PopcornFXAttributeSamplerCurveAsset.h"

UPopcornFXAttributeSamplerCurveFactory::UPopcornFXAttributeSamplerCurveFactory(FObjectInitializer const &PCIP)
	: Super(PCIP)
{
	SupportedClass = UPopcornFXAttributeSamplerCurveAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}