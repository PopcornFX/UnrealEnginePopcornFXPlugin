//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXEmitter.h"

#include "Assets/PopcornFXEffect.h"
#include "PopcornFXEmitterComponent.h"
#include "PopcornFXSceneActor.h"
#include "PopcornFXSceneComponent.h"

#include "PopcornFXSettingsEditor.h"

#include "Components/BillboardComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"

#if WITH_EDITOR
#	include "ScopedTransaction.h"
#	include "Engine/World.h"
#endif // WITH_EDITOR

#include "PopcornFXSDK.h"

APopcornFXEmitter::APopcornFXEmitter(const class FObjectInitializer& PCIP)
: Super(PCIP)
{
	PopcornFXEmitterComponent = CreateDefaultSubobject<UPopcornFXEmitterComponent>(TEXT("PopcornFXEmitterComponent_Default"));
	RootComponent = PopcornFXEmitterComponent;

	SetFlags(RF_Transactional);

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (!IsRunningCommandlet() && SpriteComponent != null)
	{
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTextureObject;
			FName ID_Effects;
			FText NAME_Effects;
			FConstructorStatics()
				: SpriteTextureObject(TEXT("/PopcornFX/SlateBrushes/icon_PopcornFX_Logo_256x"))
				, ID_Effects(TEXT("Effects")) // do not change, recognize by the engine
				, NAME_Effects(NSLOCTEXT("SpriteCategory", "Effects", "Effects")) // do not change, recognize by the engine
			{
			}
		};
		static FConstructorStatics ConstructorStatics;
		SpriteComponent->Sprite = ConstructorStatics.SpriteTextureObject.Get();

		SpriteComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
		SpriteComponent->bHiddenInGame = true;
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Effects;
		SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Effects;
		SpriteComponent->SetupAttachment(PopcornFXEmitterComponent);
		SpriteComponent->bReceivesDecals = false;
	}
#endif // WITH_EDITOR

}

//----------------------------------------------------------------------------

bool	APopcornFXEmitter::SetEffect(class UPopcornFXEffect *effect)
{
	UPopcornFXEmitterComponent* EmitterComponent = PopcornFXEmitterComponent;
	check(EmitterComponent);
	return EmitterComponent->SetEffect(effect);
}

//----------------------------------------------------------------------------

UPopcornFXEffect	*APopcornFXEmitter::GetEffect()
{
	return PopcornFXEmitterComponent->Effect;
}

//----------------------------------------------------------------------------

void	APopcornFXEmitter::OnInterpToggle(bool bEnable)
{
	PK_ASSERT(PopcornFXEmitterComponent != null);
	PK_ASSERT(PopcornFXEmitterComponent->SceneValid());

	if (PopcornFXEmitterComponent != null)
	{
		if (bEnable)
			PopcornFXEmitterComponent->StartEmitter();
		else
			PopcornFXEmitterComponent->StopEmitter();
	}
}

//----------------------------------------------------------------------------

#if WITH_EDITOR

bool	APopcornFXEmitter::GetReferencedContentObjects(TArray<UObject*>& Objects) const
{
	Super::GetReferencedContentObjects(Objects);

	// Used by the "Sync to Content Browser" right-click menu option in the editor.
	if (PopcornFXEmitterComponent)
	{
		if (PopcornFXEmitterComponent->Effect)
			Objects.AddUnique(PopcornFXEmitterComponent->Effect);
	}
	return true;
}

//----------------------------------------------------------------------------

bool	APopcornFXEmitter::EditorSpawnSceneIFN()
{
	UPopcornFXEmitterComponent* EmitterComponent = PopcornFXEmitterComponent;
	if (!EmitterComponent->ResolveScene(true))
	{
		if (IsClassLoaded<UPopcornFXSettingsEditor>() &&
			GetMutableDefault<UPopcornFXSettingsEditor>()->bAutoInsertSceneActor)
		{
			FScopedTransaction		transaction(NSLOCTEXT("PopcornFXEmitter", "EditorSpawnSceneIFN", "Spawn default scene"));

			FActorSpawnParameters	params;
			params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			//params.bNoFail = true;
			params.ObjectFlags = RF_Transactional;

			// ! Do not force the name !
			// Or UE will magicaly just return the last deleted Actor with the same name !
			//params.Name = FName(TEXT("PopcornFX_DefaultScene"));

			APopcornFXSceneActor	*scene = GetWorld()->SpawnActor<APopcornFXSceneActor>(APopcornFXSceneActor::StaticClass(), params);
			if (PK_VERIFY(scene != null && scene->PopcornFXSceneComponent != null))
			{
				scene->PopcornFXSceneComponent->SceneName = EmitterComponent->SceneName;
				PK_ONLY_IF_ASSERTS(bool sceneResolved = )EmitterComponent->ResolveScene(true);
				PK_ASSERT(sceneResolved);
				PK_ASSERT(EmitterComponent->Scene == scene);
			}
		}
	}
	return EmitterComponent->SceneValid();
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

//void APopcornFXEmitter::CheckForErrors()
//{
//
//}
