//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "Factories/Factory.h"

#include "PopcornFXFileFactory.generated.h"

UCLASS(MinimalAPI, HideCategories=Object)
class UPopcornFXFileFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
public:

	void		CleanUp() override;
	void		PostInitProperties() override;

	bool		DoesSupportClass(UClass * inClass) override;
	UClass		*ResolveSupportedClass() override;
	UObject		*FactoryCreateBinary(UClass *inClass, UObject *inParent, FName inName, EObjectFlags flags, UObject *context, const TCHAR *type, const uint8 *&buffer, const uint8 *bufferEnd, FFeedbackContext *warn, bool &bOutOperationCanceled) override;
};
