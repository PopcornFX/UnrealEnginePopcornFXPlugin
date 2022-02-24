//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "GameFramework/HUD.h"

#include "PopcornFXHUDProfiler.generated.h"

class	CPopcornFXHUDProfiler_Data;

UCLASS(MinimalAPI, Config=Game)
class APopcornFXHUDProfiler : public AHUD
{
	GENERATED_UCLASS_BODY()
public:
	~APopcornFXHUDProfiler();

	void				DrawDebugHUD(UCanvas *canvas, APlayerController *controller);

	virtual void		PostActorCreated() override;
	virtual void		Destroyed() override;

private:
	void				DrawBar(float minX, float maxX, float yPos, float cursor, float thickness);
};
