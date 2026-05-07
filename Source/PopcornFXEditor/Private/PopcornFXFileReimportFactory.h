//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXFileFactory.h"
#include "EditorReimportHandler.h"

#include "PopcornFXFileReimportFactory.generated.h"

UCLASS(MinimalAPI)
class UPopcornFXFileReimportFactory : public UPopcornFXFileFactory, public FReimportHandler
{
	GENERATED_UCLASS_BODY()
public:
	virtual bool					CanReimport(UObject *obj, TArray<FString> &outFilenames) override;
	virtual void					SetReimportPaths(UObject *obj, const TArray<FString> &newReimportPaths) override;
	virtual EReimportResult::Type	Reimport(UObject *obj) override;
	virtual int32					GetPriority() const override;
};
