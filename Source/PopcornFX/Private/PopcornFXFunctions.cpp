//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXFunctions.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXSceneActor.h"
#include "PopcornFXEmitterComponent.h"
#include "PopcornFXSceneComponent.h"
#include "Assets/PopcornFXEffect.h"
#include "PopcornFXTypes.h"

#include "Internal/ResourceHandlerImage_UE.h"

#include "Engine/Engine.h"

#include "PopcornFXSDK.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXFunctions"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXFunctions, Log, All);

//----------------------------------------------------------------------------

UPopcornFXFunctions::UPopcornFXFunctions(class FObjectInitializer const &pcip)
	: Super(pcip)
{
}

//----------------------------------------------------------------------------

const FString	&UPopcornFXFunctions::PopcornFXPluginVersion()
{
	return FPopcornFXPlugin::PluginVersionString();
}

//----------------------------------------------------------------------------

const FString	&UPopcornFXFunctions::PopcornFXRuntimeVersion()
{
	return FPopcornFXPlugin::PopornFXRuntimeVersionString();
}

//----------------------------------------------------------------------------

float	UPopcornFXFunctions::PopcornFXGlobalScale()
{
	return FPopcornFXPlugin::GlobalScale();
}

//----------------------------------------------------------------------------

float	UPopcornFXFunctions::PopcornFXGlobalScaleInv()
{
	return FPopcornFXPlugin::GlobalScaleRcp();
}

//----------------------------------------------------------------------------

int32	UPopcornFXFunctions::PopcornFXTotalParticleCount()
{
	return FPopcornFXPlugin::TotalParticleCount();
}

//----------------------------------------------------------------------------

void	UPopcornFXFunctions::NotifySettingChanged()
{
	FPopcornFXPlugin::Get().HandleSettingsModified();
}

//----------------------------------------------------------------------------

UPopcornFXEmitterComponent*	UPopcornFXFunctions::SpawnEmitterAtLocation(UObject* WorldContextObject, class UPopcornFXEffect* Effect, FName SceneName, FVector Location, FRotator Rotation, bool bStartEmitter, bool bAutoDestroy)
{
	if (Effect == null)
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Spawn Emitter At Location: No effect specified"));
		return null;
	}
	UWorld	*world = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (world == null)
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Spawn Emitter At Location: Couldn't get the world from '%s'"), *WorldContextObject->GetName());
		return null;
	}

	UPopcornFXEmitterComponent	*emitter = null;
	if (FApp::CanEverRender() && !world->IsNetMode(NM_DedicatedServer))
	{
		emitter = UPopcornFXEmitterComponent::CreateStandaloneEmitterComponent(Effect, null, world, null, bAutoDestroy);
		if (emitter == null)
		{
			UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Spawn Emitter At Location: Failed to create Effect '%s'"), *Effect->GetName());
			return null;
		}
		emitter->bEnableUpdates = true;
		emitter->bAutoActivate = true;
		emitter->SceneName = SceneName;
		if (!emitter->ResolveScene(true))
		{
			emitter->DestroyComponent();
			return null;
		}
		emitter->SetWorldLocation(Location);
		emitter->SetWorldRotation(Rotation);
		emitter->SetEffect(Effect, bStartEmitter);
	}
	return emitter;
}

//----------------------------------------------------------------------------

UPopcornFXEmitterComponent *UPopcornFXFunctions::SpawnEmitterAttached(class UPopcornFXEffect* Effect, class USceneComponent* AttachToComponent, FName SceneName, FName AttachPointName, FVector Location, FRotator Rotation, EAttachLocation::Type LocationType, bool bStartEmitter, bool bAutoDestroy)
{
	if (AttachToComponent == null)
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Spawn Emitter Attached: No AttachToComponent specified"));
		return null;
	}
	if (Effect == null)
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Spawn Emitter Attached: No effect specified"));
		return null;
	}
	UWorld	*world = AttachToComponent->GetWorld();
	if (world == null)
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Spawn Emitter Attached: Couldn't get the world from '%s'"), *AttachToComponent->GetName());
		return null;
	}

	UPopcornFXEmitterComponent	*emitter = null;
	if (FApp::CanEverRender() && !world->IsNetMode(NM_DedicatedServer))
	{
		emitter = UPopcornFXEmitterComponent::CreateStandaloneEmitterComponent(Effect, null, world, null, bAutoDestroy);
		if (emitter == null)
		{
			UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Spawn Emitter Attached: Failed to create Effect '%s'"), *Effect->GetName());
			return null;
		}
		emitter->bEnableUpdates = true;
		emitter->bAutoActivate = true;
		emitter->SceneName = SceneName;
		if (!emitter->ResolveScene(true))
		{
			emitter->DestroyComponent();
			return null;
		}
		emitter->AttachToComponent(AttachToComponent, FAttachmentTransformRules::KeepRelativeTransform, AttachPointName);
		if (LocationType == EAttachLocation::KeepWorldPosition)
			emitter->SetWorldLocationAndRotation(Location, Rotation);
		else
			emitter->SetRelativeLocationAndRotation(Location, Rotation);
		emitter->SetEffect(Effect, bStartEmitter);
	}
	return emitter;
}

//----------------------------------------------------------------------------

void		UPopcornFXFunctions::NotifyObjectChanged(class UObject *object)
{
	FPopcornFXPlugin::Get().NotifyObjectChanged(object);
}

//----------------------------------------------------------------------------

//static
bool		UPopcornFXFunctions::IsTextureCPUInUse(const FString &virtualPath)
{
	PopcornFX::IResourceHandler		*ihandler = PopcornFX::Resource::DefaultManager()->FindHandler<PopcornFX::CImage>();
	if (!PK_VERIFY(ihandler != null))
		return false;
	CResourceHandlerImage_UE		*handler = PopcornFX::checked_cast<CResourceHandlerImage_UE*>(ihandler);
	const PopcornFX::CString		virtPath = ToPk(virtualPath);
	return handler->IsUsed(virtPath, false);
}

//static
bool		UPopcornFXFunctions::IsTextureGPUInUse(const FString &virtualPath)
{
#if (PK_GPU_D3D11 != 0)
	PopcornFX::IResourceHandler		*ihandler = PopcornFX::Resource::DefaultManager()->FindHandler<PopcornFX::CImageGPU_D3D11>();
	if (!PK_VERIFY(ihandler != null))
		return false;
	CResourceHandlerImage_UE_D3D11	*handler = PopcornFX::checked_cast<CResourceHandlerImage_UE_D3D11*>(ihandler);
	const PopcornFX::CString		virtPath = ToPk(virtualPath);
	return handler->IsUsed(virtPath, false);
#else
	return false;
#endif
}

//----------------------------------------------------------------------------


//static
bool		UPopcornFXFunctions::RegisterVirtualTextureOverride_CPU_FloatR(const FString &virtualPath, int32 width, int32 height, const TArray<float> &pixels)
{
	if (!PK_VERIFY(pixels.Num() > 0))
		return false;

	if (!PK_VERIFY(width > 0) || !PK_VERIFY(height > 0))
		return false;

	PopcornFX::IResourceHandler		*ihandler = PopcornFX::Resource::DefaultManager()->FindHandler<PopcornFX::CImage>();
	if (!PK_VERIFY(ihandler != null))
		return false;
	CResourceHandlerImage_UE		*handler = PopcornFX::checked_cast<CResourceHandlerImage_UE*>(ihandler);
	const PopcornFX::CString		virtPath = ToPk(virtualPath);
	const PopcornFX::CUint2			size(width, height);

	PK_ASSERT(pixels.GetTypeSize() == sizeof(float));
	return handler->RegisterVirtualTexture(
		virtPath, false,
		PopcornFX::CImage::Format_Fp32Lum,
		size,
		pixels.GetData(),
		pixels.Num() * pixels.GetTypeSize()
	);
}

//static
bool		UPopcornFXFunctions::RegisterVirtualTextureOverride_CPU_FloatRGBA(const FString &virtualPath, int32 width, int32 height, const TArray<FLinearColor> &pixels)
{
	if (!PK_VERIFY(pixels.Num() > 0))
		return false;

	if (!PK_VERIFY(width > 0) || !PK_VERIFY(height > 0))
		return false;

	PopcornFX::IResourceHandler		*ihandler = PopcornFX::Resource::DefaultManager()->FindHandler<PopcornFX::CImage>();
	if (!PK_VERIFY(ihandler != null))
		return false;
	CResourceHandlerImage_UE		*handler = PopcornFX::checked_cast<CResourceHandlerImage_UE*>(ihandler);
	const PopcornFX::CString		virtPath = ToPk(virtualPath);
	const PopcornFX::CUint2			size(width, height);

	PK_ASSERT(pixels.GetTypeSize() == sizeof(FLinearColor));
	return handler->RegisterVirtualTexture(
		virtPath, false,
		PopcornFX::CImage::Format_Fp32RGBA,
		size,
		pixels.GetData(),
		pixels.Num() * pixels.GetTypeSize()
	);
}

//static
bool		UPopcornFXFunctions::UnregisterVirtualTextureOverride_CPU(const FString &virtualPath)
{
	PopcornFX::IResourceHandler		*ihandler = PopcornFX::Resource::DefaultManager()->FindHandler<PopcornFX::CImage>();
	if (!PK_VERIFY(ihandler != null))
		return false;
	const PopcornFX::CString		virtPath = ToPk(virtualPath);
	CResourceHandlerImage_UE		*handler = PopcornFX::checked_cast<CResourceHandlerImage_UE*>(ihandler);
	return handler->UnregisterVirtualTexture(virtPath, false);
}

//----------------------------------------------------------------------------

// static
bool		UPopcornFXFunctions::RegisterVirtualTextureOverride_GPU(const FString &virtualPath, UTexture *texture)
{
#if (PK_GPU_D3D11 != 0)
	if (!PK_VERIFY(texture != null))
		return false;

	PopcornFX::IResourceHandler		*ihandler = PopcornFX::Resource::DefaultManager()->FindHandler<PopcornFX::CImageGPU_D3D11>();
	if (!PK_VERIFY(ihandler != null))
		return false;
	CResourceHandlerImage_UE_D3D11	*handler = PopcornFX::checked_cast<CResourceHandlerImage_UE_D3D11*>(ihandler);
	const PopcornFX::CString		virtPath = ToPk(virtualPath);
	return handler->RegisterVirtualTexture(virtPath, false, texture);
#else
	return false;
#endif
}

//static
bool		UPopcornFXFunctions::UnregisterVirtualTextureOverride_GPU(const FString &virtualPath)
{
#if (PK_GPU_D3D11 != 0)
	PopcornFX::IResourceHandler		*ihandler = PopcornFX::Resource::DefaultManager()->FindHandler<PopcornFX::CImageGPU_D3D11>();
	if (!PK_VERIFY(ihandler != null))
		return false;
	const PopcornFX::CString		virtPath = ToPk(virtualPath);
	CResourceHandlerImage_UE_D3D11	*handler = PopcornFX::checked_cast<CResourceHandlerImage_UE_D3D11*>(ihandler);
	return handler->UnregisterVirtualTexture(virtPath, false);
#else
	return false;
#endif
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsFloat(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, float &OutValue, bool InApplyGlobalScale)
{
	if (!PK_VERIFY(InSelf != null))
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Get Event Payload: Invalid InSelf"));
		return false;
	}

	if (InApplyGlobalScale)
		OutValue *= FPopcornFXPlugin::GlobalScale();

	return InSelf->GetPayloadValue(PayloadName, EPopcornFXPayloadType::Float, &OutValue);
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsFloat2(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, float &OutValueX, float &OutValueY, bool InApplyGlobalScale)
{
	if (!PK_VERIFY(InSelf != null))
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Get Event Payload: Invalid InSelf"));
		return false;
	}
	PK_ALIGN(0x10) float	outValue[2];
	if (!InSelf->GetPayloadValue(PayloadName, EPopcornFXPayloadType::Float2, outValue))
		return false;
	OutValueX = outValue[0];
	OutValueY = outValue[1];

	if (InApplyGlobalScale)
	{
		const float globalScale = FPopcornFXPlugin::GlobalScale();

		OutValueX *= globalScale;
		OutValueY *= globalScale;
	}

	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsVector2D(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, FVector2D &OutValue, bool InApplyGlobalScale)
{
	float	outValues[2];
	if (!GetEventPayloadAsFloat2(InSelf, PayloadName, outValues[0], outValues[1], InApplyGlobalScale))
		return false;
	OutValue.X = outValues[0];
	OutValue.Y = outValues[1];
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsFloat3(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, float &OutValueX, float &OutValueY, float &OutValueZ, bool InApplyGlobalScale)
{
	if (!PK_VERIFY(InSelf != null))
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Get Event Payload: Invalid InSelf"));
		return false;
	}
	PK_ALIGN(0x10) float	outValue[3];
	if (!InSelf->GetPayloadValue(PayloadName, EPopcornFXPayloadType::Float3, outValue))
		return false;
	OutValueX = outValue[0];
	OutValueY = outValue[1];
	OutValueZ = outValue[2];

	if (InApplyGlobalScale)
	{
		const float globalScale = FPopcornFXPlugin::GlobalScale();

		OutValueX *= globalScale;
		OutValueY *= globalScale;
		OutValueZ *= globalScale;
	}

	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsVector(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, FVector &OutValue, bool InApplyGlobalScale)
{
	float	outValues[3];
	if (!GetEventPayloadAsFloat3(InSelf, PayloadName, outValues[0], outValues[1], outValues[2], InApplyGlobalScale))
		return false;
	OutValue.X = outValues[0];
	OutValue.Y = outValues[1];
	OutValue.Z = outValues[2];
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsFloat4(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, float &OutValueX, float &OutValueY, float &OutValueZ, float &OutValueW, bool InApplyGlobalScale)
{
	if (!PK_VERIFY(InSelf != null))
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Get Event Payload: Invalid InSelf"));
		return false;
	}
	PK_ALIGN(0x10) float	outValue[4];
	if (!InSelf->GetPayloadValue(PayloadName, EPopcornFXPayloadType::Float4, outValue))
		return false;
	OutValueX = outValue[0];
	OutValueY = outValue[1];
	OutValueZ = outValue[2];
	OutValueW = outValue[3];

	if (InApplyGlobalScale)
	{
		const float globalScale = FPopcornFXPlugin::GlobalScale();

		OutValueX *= globalScale;
		OutValueY *= globalScale;
		OutValueZ *= globalScale;
		OutValueW *= globalScale;
	}

	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsLinearColor(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, FLinearColor &OutValue)
{
	return GetEventPayloadAsFloat4(InSelf, PayloadName, OutValue.R, OutValue.G, OutValue.B, OutValue.A, false);
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsInt(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, int32 &OutValue)
{
	if (!PK_VERIFY(InSelf != null))
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Get Event Payload: Invalid InSelf"));
		return false;
	}
	return InSelf->GetPayloadValue(PayloadName, EPopcornFXPayloadType::Int, &OutValue);
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsInt2(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, int32 &OutValueX, int32 &OutValueY)
{
	if (!PK_VERIFY(InSelf != null))
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Get Event Payload: Invalid InSelf"));
		return false;
	}
	PK_ALIGN(0x10) int32	outValue[2];
	if (!InSelf->GetPayloadValue(PayloadName, EPopcornFXPayloadType::Int2, outValue))
		return false;
	OutValueX = outValue[0];
	OutValueY = outValue[1];

	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsInt3(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ)
{
	if (!PK_VERIFY(InSelf != null))
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Get Event Payload: Invalid InSelf"));
		return false;
	}
	PK_ALIGN(0x10) int32	outValue[3];
	if (!InSelf->GetPayloadValue(PayloadName, EPopcornFXPayloadType::Int3, outValue))
		return false;
	OutValueX = outValue[0];
	OutValueY = outValue[1];
	OutValueZ = outValue[2];
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsInt4(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, int32 &OutValueX, int32 &OutValueY, int32 &OutValueZ, int32 &OutValueW)
{
	if (!PK_VERIFY(InSelf != null))
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Get Event Payload: Invalid InSelf"));
		return false;
	}
	PK_ALIGN(0x10) int32	outValue[4];
	if (!InSelf->GetPayloadValue(PayloadName, EPopcornFXPayloadType::Int4, outValue))
		return false;
	OutValueX = outValue[0];
	OutValueY = outValue[1];
	OutValueZ = outValue[2];
	OutValueW = outValue[3];
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsBool(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, bool &OutValue)
{
	if (!PK_VERIFY(InSelf != null))
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Get Event Payload: Invalid InSelf"));
		return false;
	}
	return InSelf->GetPayloadValue(PayloadName, EPopcornFXPayloadType::Bool, &OutValue);
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsBool2(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, bool &OutValueX, bool &OutValueY)
{
	if (!PK_VERIFY(InSelf != null))
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Get Event Payload: Invalid InSelf"));
		return false;
	}
	PK_ALIGN(0x10) bool	outValue[2];
	if (!InSelf->GetPayloadValue(PayloadName, EPopcornFXPayloadType::Bool2, outValue))
		return false;
	OutValueX = outValue[0];
	OutValueY = outValue[1];
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsBool3(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, bool &OutValueX, bool &OutValueY, bool &OutValueZ)
{
	if (!PK_VERIFY(InSelf != null))
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Get Event Payload: Invalid InSelf"));
		return false;
	}
	PK_ALIGN(0x10) bool	outValue[3];
	if (!InSelf->GetPayloadValue(PayloadName, EPopcornFXPayloadType::Bool3, outValue))
		return false;
	OutValueX = outValue[0];
	OutValueY = outValue[1];
	OutValueZ = outValue[2];
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsBool4(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, bool &OutValueX, bool &OutValueY, bool &OutValueZ, bool &OutValueW)
{
	if (!PK_VERIFY(InSelf != null))
	{
		UE_LOG(LogPopcornFXFunctions, Warning, TEXT("Get Event Payload: Invalid InSelf"));
		return false;
	}
	PK_ALIGN(0x10) bool	outValue[4];
	if (!InSelf->GetPayloadValue(PayloadName, EPopcornFXPayloadType::Bool4, outValue))
		return false;
	OutValueX = outValue[0];
	OutValueY = outValue[1];
	OutValueZ = outValue[2];
	OutValueW = outValue[3];
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXFunctions::GetEventPayloadAsRotator(const UPopcornFXEmitterComponent *InSelf, FName PayloadName, FRotator &OutValue, bool InApplyGlobalScale)
{
	PK_ALIGN(0x10) float	outValue[4];
	if (GetEventPayloadAsFloat4(InSelf, PayloadName, outValue[0], outValue[1], outValue[2], outValue[3], InApplyGlobalScale))
	{
		OutValue = FQuat(outValue[0], outValue[1], outValue[2], outValue[3]).Rotator();
		return true;
	}

	return false;
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
