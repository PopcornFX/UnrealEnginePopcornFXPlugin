#include "PopcornFXAttributeSamplerAnimTrackAsset.h"

UPopcornFXAttributeSamplerAnimTrackFactory::UPopcornFXAttributeSamplerAnimTrackFactory(FObjectInitializer const &PCIP)
	: Super(PCIP)
{
	SupportedClass = UPopcornFXAttributeSamplerAnimTrackAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}