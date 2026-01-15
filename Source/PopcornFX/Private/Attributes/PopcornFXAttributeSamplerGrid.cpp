//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeSamplerGrid.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_samplers_classes.h>
#include <pk_render_helpers/include/frame_collector/rh_frame_data.h>

#include "PopcornFXStats.h"
#include "GPUSim/PopcornFXGPUSim.h"
#include "Assets/PopcornFXEffect.h"
#include "Assets/PopcornFXEffectPriv.h"
#include "Internal/ParticleScene.h"
#include "Internal/ResourceHandlerImage_UE.h"
#include "Platforms/PopcornFXPlatform.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureRenderTargetVolume.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/Package.h"

#if (PK_GPU_D3D12 != 0)
#	include <pk_particles/include/Samplers/D3D12/image_gpu_d3d12.h>
#	include <pk_particles/include/Samplers/D3D12/rectangle_list_gpu_d3d12.h>

#	include "Windows/HideWindowsPlatformTypes.h"
#	include "D3D12RHIPrivate.h"
#	include "D3D12Util.h"
#endif // (PK_GPU_D3D12 != 0)

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSamplerGrid"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSamplerGrid, Log, All);

//----------------------------------------------------------------------------

namespace
{
	PopcornFX::EBaseTypeID	ToPk(const EPopcornFXGridDataType::Type type)
	{
		switch (type)
		{
		case EPopcornFXGridDataType::Float:
			return PopcornFX::EBaseTypeID::BaseType_Float;
		case EPopcornFXGridDataType::Float2:
			return PopcornFX::EBaseTypeID::BaseType_Float2;
		case EPopcornFXGridDataType::Float3:
			return PopcornFX::EBaseTypeID::BaseType_Float3;
		case EPopcornFXGridDataType::Float4:
			return PopcornFX::EBaseTypeID::BaseType_Float4;
		case EPopcornFXGridDataType::Int:
			return PopcornFX::EBaseTypeID::BaseType_I32;
		case EPopcornFXGridDataType::Int2:
			return PopcornFX::EBaseTypeID::BaseType_Int2;
		case EPopcornFXGridDataType::Int3:
			return PopcornFX::EBaseTypeID::BaseType_Int3;
		case EPopcornFXGridDataType::Int4:
			return PopcornFX::EBaseTypeID::BaseType_Int4;
		default:
			return PopcornFX::EBaseTypeID::BaseType_Void;
		}
	}

	//----------------------------------------------------------------------------

	EPopcornFXGridDataType::Type ToUE(const PopcornFX::EBaseTypeID type)
	{
		switch (type)
		{
		case PopcornFX::EBaseTypeID::BaseType_Float:
			return EPopcornFXGridDataType::Float;
		case PopcornFX::EBaseTypeID::BaseType_Float2:
			return EPopcornFXGridDataType::Float2;
		case PopcornFX::EBaseTypeID::BaseType_Float3:
			return EPopcornFXGridDataType::Float3;
		case PopcornFX::EBaseTypeID::BaseType_Float4:
			return EPopcornFXGridDataType::Float4;
		case PopcornFX::EBaseTypeID::BaseType_I32:
			return EPopcornFXGridDataType::Int;
		case PopcornFX::EBaseTypeID::BaseType_Int2:
			return EPopcornFXGridDataType::Int2;
		case PopcornFX::EBaseTypeID::BaseType_Int3:
			return EPopcornFXGridDataType::Int3;
		case PopcornFX::EBaseTypeID::BaseType_Int4:
			return EPopcornFXGridDataType::Int4;
		default:
			return EPopcornFXGridDataType::Float4;
		}
	}

	//----------------------------------------------------------------------------

	FString	DataTypeToString(const EPopcornFXGridDataType::Type type)
	{
		switch (type)
		{
		case EPopcornFXGridDataType::Type::Float:
			return "float1";
		case EPopcornFXGridDataType::Type::Float2:
			return "float2";
		case EPopcornFXGridDataType::Type::Float3:
			return "float3";
		case EPopcornFXGridDataType::Type::Float4:
			return "float4";
		case EPopcornFXGridDataType::Type::Int:
			return "int1";
		case EPopcornFXGridDataType::Type::Int2:
			return "int2";
		case EPopcornFXGridDataType::Type::Int3:
			return "int3";
		case EPopcornFXGridDataType::Type::Int4:
			return "int4";
		default:
			return "unknown";
		}
	}

	//----------------------------------------------------------------------------

	PopcornFX::EBaseTypeID	ToPk(EPixelFormat format)
	{
		switch (format)
		{
		case	PF_R32_FLOAT:
			return PopcornFX::BaseType_Float;
		case	PF_G32R32F:
			return PopcornFX::BaseType_Float2;
		case	PF_R32G32B32F:
			return PopcornFX::BaseType_Float3;
		case	PF_A32B32G32R32F:
			return PopcornFX::BaseType_Float4;
		case PF_R32_UINT:
			return PopcornFX::BaseType_I32;
		case PF_R32G32_UINT:
			return PopcornFX::BaseType_Int2;
		case PF_R32G32B32_UINT:
			return PopcornFX::BaseType_Int3;
		case PF_R32G32B32A32_UINT:
			return PopcornFX::BaseType_Int4;
		default:
			return PopcornFX::BaseType_Void;
		};
	}
};

//----------------------------------------------------------------------------

struct	FAttributeSamplerGridData
{
	bool												m_ReloadGrid = true;
	PopcornFX::PParticleSamplerDescriptor_Grid_Default	m_Desc;

	void	Clear()
	{
		m_Desc = null;
		m_ReloadGrid = true;
	}

	FAttributeSamplerGridData() {}

	~FAttributeSamplerGridData()
	{
	}
};

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerGrid::SetRenderTarget(class UTextureRenderTarget *InRenderTarget)
{
	Properties.RenderTarget = InRenderTarget;

	// Check supported properties here?
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerGrid::SetAsMaterialTextureParameter(UMaterialInstanceDynamic *Material, FName ParameterName)
{
	if (Material == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplerGrid, Warning, TEXT("SetAsMaterialTextureParameter: couldn't set grid as material texture parameter: null material"));
		return;
	}
	UTexture *texture = GridTexture();
	if (texture == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplerGrid, Warning, TEXT("SetAsMaterialTextureParameter: couldn't set grid as material texture parameter: empty grid texture"));
		return;
	}
	Material->SetTextureParameterValue(ParameterName, texture);
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerGrid::UPopcornFXAttributeSamplerGrid(const FObjectInitializer &PCIP)
:	Super(PCIP)
{
	bAutoActivate = true;

	Properties.RenderTarget = null;
	Properties.bAssetGrid = false;

	Properties.bSRGB = false;

	Properties.Order = EPopcornFXGridOrder::ThreeD;

	Properties.SizeX = 128;
	Properties.SizeY = 128;
	Properties.SizeZ = 1;

	Properties.DataType = EPopcornFXGridDataType::Float4;

	// UPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::Grid;

	m_Data = new FAttributeSamplerGridData();
	check(m_Data != null);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerGrid::OnUnregister()
{
	if (m_Data != null)
	{
		// Unregister the component during OnUnregister instead of BeginDestroy.
		// In editor mode, BeginDestroy is only called when saving a level:
		// Components ReregisterComponent() do not have a matching BeginDestroy call in editor
		m_Data->m_Desc = null;
		m_Data->m_ReloadGrid = true;
	}
	Super::OnUnregister();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerGrid::BeginDestroy()
{
	if (m_Data != null)
	{
		delete m_Data;
		m_Data = null;
	}
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

UTexture	*UPopcornFXAttributeSamplerGrid::GridTexture()
{
	return Properties.bAssetGrid != 0 ? Properties.RenderTarget : m_GridTexture;
}

//----------------------------------------------------------------------------

#if WITH_EDITOR

void	UPopcornFXAttributeSamplerGrid::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	if (propertyChangedEvent.Property != NULL)
	{
		const FString	propertyName = propertyChangedEvent.Property->GetName();

		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesGrid, bAssetGrid) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesGrid, bSRGB) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesGrid, RenderTarget) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesGrid, Order) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesGrid, SizeX) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesGrid, SizeY) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesGrid, SizeZ) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesGrid, DataType))
		{
			// Rebuild
			m_Data->m_ReloadGrid = true;
		}
	}

	if (!ArePropertiesSupported())
	{
#if WITH_EDITOR
		OnSamplerValidStateChanged.Broadcast();
#endif
	}

	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerGrid::CopyPropertiesFrom(const UPopcornFXAttributeSampler *other)
{
	const FPopcornFXAttributeSamplerPropertiesGrid *newGridProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesGrid *>(other->GetProperties());
	if (!PK_VERIFY(newGridProperties != null))
	{
		UE_LOG(LogPopcornFXAttributeSamplerGrid, Error, TEXT("New properties are null or not curve properties"));
		return;
	}

	Super::CopyPropertiesFrom(other);

	if (newGridProperties->bAssetGrid != Properties.bAssetGrid ||
		newGridProperties->bSRGB != Properties.bSRGB ||
		newGridProperties->RenderTarget != Properties.RenderTarget ||
		newGridProperties->SizeX != Properties.SizeX ||
		newGridProperties->SizeY != Properties.SizeY ||
		newGridProperties->SizeZ != Properties.SizeZ ||
		newGridProperties->DataType != Properties.DataType)
	{
		// Rebuild
		m_Data->m_ReloadGrid = true;
	}

	Properties = *newGridProperties;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerGrid::SetupDefaults(UPopcornFXEffect *effect, const uint32 samplerIdx, bool updateUnlockedValues)
{
	Super::SetupDefaults(effect, samplerIdx, updateUnlockedValues);

	const PopcornFX::PCParticleAttributeList &attrListPtr = effect->Effect()->AttributeList();

	if (attrListPtr == null || *(attrListPtr->DefaultAttributes()) == null)
	{
		return;
	}

	PopcornFX::TMemoryView<const PopcornFX::CParticleAttributeSamplerDeclaration *const>	samplerList = attrListPtr->UniqueSamplerList();
	if (samplerList.Count() == 0)
	{
		return;
	}
	const PopcornFX::CParticleAttributeSamplerDeclaration * const	samplerDesc = attrListPtr->UniqueSamplerList()[samplerIdx];
	PK_ASSERT(samplerDesc != null);
	if (samplerDesc == null)
	{
		return;
	}
	const PopcornFX::PResourceDescriptor		defaultSampler = samplerDesc->AttribSamplerDefaultValue();

	const PopcornFX::CResourceDescriptor_Grid	*grid = PopcornFX::HBO::Cast<PopcornFX::CResourceDescriptor_Grid>(defaultSampler.Get());
	if (grid != null)
	{
		Properties.Order = static_cast<EPopcornFXGridOrder>(grid->Order() - 1);
		PK_ASSERT(Properties.Order >= EPopcornFXGridOrder::OneD && Properties.Order <= EPopcornFXGridOrder::ThreeD);
		// Don't reset the sizes to 1 when the grid order is reduced, the user may want to keep them for later
		// if they increase the order back
		if (updateUnlockedValues)
		{
			PopcornFX::CInt4	dimensions = grid->Dimensions4();
			Properties.SizeX = dimensions.x();
			if (Properties.Order >= EPopcornFXGridOrder::TwoD)
				Properties.SizeY = dimensions.y();
			if (Properties.Order >= EPopcornFXGridOrder::ThreeD)
				Properties.SizeZ = dimensions.z();
		}

		switch (grid->DataType())
		{
		case PopcornFX::Nodegraph::EDataType::DataType_Float1:
			Properties.DataType = EPopcornFXGridDataType::Float;
			break;
		case PopcornFX::Nodegraph::EDataType::DataType_Float2:
			Properties.DataType = EPopcornFXGridDataType::Float2;
			break;
		case PopcornFX::Nodegraph::EDataType::DataType_Float3:
			// Unsupported
			Properties.DataType = EPopcornFXGridDataType::Float3;
			break;
		case PopcornFX::Nodegraph::EDataType::DataType_Float4:
			Properties.DataType = EPopcornFXGridDataType::Float4;
			break;
		case PopcornFX::Nodegraph::EDataType::DataType_Int1:
			Properties.DataType = EPopcornFXGridDataType::Int;
			break;
		case PopcornFX::Nodegraph::EDataType::DataType_Int2:
			Properties.DataType = EPopcornFXGridDataType::Int2;
			break;
		case PopcornFX::Nodegraph::EDataType::DataType_Int3:
			// Unsupported
			Properties.DataType = EPopcornFXGridDataType::Int3;
			break;
		case PopcornFX::Nodegraph::EDataType::DataType_Int4:
			Properties.DataType = EPopcornFXGridDataType::Int4;
			break;
		default:
			break;
		}
	}
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerGrid::HasRenderTargetChanged() const
{
	const UTextureRenderTarget2D		*RT2D = Cast<UTextureRenderTarget2D>(Properties.RenderTarget);
	const UTextureRenderTargetVolume	*RTVolume = Cast<UTextureRenderTargetVolume>(Properties.RenderTarget);
	const CUint4						&dim = m_Data->m_Desc->m_GridDimensions;
	if (RT2D)
	{
		if (dim.x() != RT2D->SizeX || dim.x() != RT2D->SizeY
			|| m_Data->m_Desc->m_DataType != ToPk(RT2D->GetFormat()))
		{
			m_Data->m_ReloadGrid = true;
		}
	}
	else if (RTVolume)
	{
		if (dim.x() != RTVolume->SizeX || dim.y() != RTVolume->SizeY || dim.z() != RTVolume->SizeZ
			|| m_Data->m_Desc->m_DataType != ToPk(RTVolume->GetFormat()))
		{
			m_Data->m_ReloadGrid = true;
		}
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerGrid::IsRenderTargetCompatible(const UTextureRenderTarget *texture, UPopcornFXEmitterComponent *emitter, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	const PopcornFX::CResourceDescriptor_Grid	*defaultGridSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Grid>(defaultSampler);
	if (!PK_VERIFY(defaultGridSampler != null))
		return false;
	const u32									gridOrder = static_cast<u32>(Properties.Order + 1);
	const PopcornFX::Nodegraph::SDataTypeTraits	&srcTypeTraits = PopcornFX::Nodegraph::SDataTypeTraits::Traits((PopcornFX::Nodegraph::EDataType)defaultGridSampler->Type());
	const PopcornFX::EBaseTypeID				srcBaseType = srcTypeTraits.BaseType();

	const UTextureRenderTarget2D		*RT2D = Cast<UTextureRenderTarget2D>(texture);
	const UTextureRenderTargetVolume	*RTVolume = Cast<UTextureRenderTargetVolume>(texture);
	EPixelFormat						RTFormat = PF_Unknown;

	if (RT2D != null)
	{
		// Warn the user that using a 2D render target for a 1D grid is not optimal
		if (defaultGridSampler->Order() == 1)
		{
			FString warningMsg = FString::Printf(TEXT("Using a 2D render target for a 1D grid creates overhead"));
			UE_LOG_WARNING_SAMPLER(LogPopcornFXAttributeSamplerGrid, Warning, grid, this, emitter, warningMsg);
		}
		if (defaultGridSampler->Order() > 2)
		{
			FString errorMsg = FString::Printf(TEXT("Can't use a 2D render target for a %dD grid"), defaultGridSampler->Order());
#if WITH_EDITOR
			m_IncompatibleProperties.FindOrAdd(emitter).m_Properties.Add(TEXT("RenderTarget"), errorMsg);
#endif
			UE_LOG_INCOMPATIBLE_SAMPLER(LogPopcornFXAttributeSamplerGrid, Error, grid, this, emitter, errorMsg);
			return false;
		}
		RTFormat = RT2D->GetFormat();
	}

	if (RTVolume != null)
	{
		if (defaultGridSampler->Order() != 3)
		{
			FString errorMsg = FString::Printf(TEXT("Can't use a volume render target (= 3D) for a %dD grid"), defaultGridSampler->Order());
#if WITH_EDITOR
			m_IncompatibleProperties.FindOrAdd(emitter).m_Properties.Add(TEXT("RenderTarget"), errorMsg);
#endif
			UE_LOG_INCOMPATIBLE_SAMPLER(LogPopcornFXAttributeSamplerGrid, Error, grid, this, emitter, errorMsg);
			return false;
		}
		RTFormat = RTVolume->GetFormat();
	}

	FString dataTypeString = DataTypeToString(ToUE(srcBaseType));

	switch (RTFormat)
	{
	case	PF_R32_FLOAT:
	{
		if (srcBaseType == PopcornFX::EBaseTypeID::BaseType_Float)
			return true;
		FString errorMsg = FString::Printf(TEXT("Can't use a RTF_R32f (R32_FLOAT) render target for a %s grid"), *dataTypeString);
#if WITH_EDITOR
		m_IncompatibleProperties.FindOrAdd(emitter).m_Properties.Add(TEXT("RenderTarget"), errorMsg);
#endif
		UE_LOG_INCOMPATIBLE_SAMPLER(LogPopcornFXAttributeSamplerGrid, Error, grid, this, emitter, errorMsg);
		return false;
	}
	case	PF_G32R32F:
	{
		if (srcBaseType == PopcornFX::EBaseTypeID::BaseType_Float2)
			return true;
		FString errorMsg = FString::Printf(TEXT("Can't use a RTF_RG32f (G32R32F) render target for a %s grid"), *dataTypeString);
#if WITH_EDITOR
		m_IncompatibleProperties.FindOrAdd(emitter).m_Properties.Add(TEXT("RenderTarget"), errorMsg);
#endif
		UE_LOG_INCOMPATIBLE_SAMPLER(LogPopcornFXAttributeSamplerGrid, Error, grid, this, emitter, errorMsg);
		return false;
	}
	case	PF_A32B32G32R32F:
	{
		if (srcBaseType == PopcornFX::EBaseTypeID::BaseType_Float4)
			return true;
		FString errorMsg = FString::Printf(TEXT("Can't use a RTF_RGBA32f (A32B32G32R32F) render target for a %s grid"), *dataTypeString);
#if WITH_EDITOR
		m_IncompatibleProperties.FindOrAdd(emitter).m_Properties.Add(TEXT("RenderTarget"), errorMsg);
#endif
		UE_LOG_INCOMPATIBLE_SAMPLER(LogPopcornFXAttributeSamplerGrid, Error, grid, this, emitter, errorMsg);
		return false;
	}
	default:
		return false;
	};
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerGrid::ArePropertiesSupported()
{
	if (Properties.bAssetGrid)
	{
		if (Properties.RenderTarget == null)
		{
			FString errorMsg = TEXT("Null render target");
#if WITH_EDITOR
			m_UnsupportedProperties.FindOrAdd(TEXT("RenderTarget"), *errorMsg);
#endif
			UE_LOG_UNSUPPORTED_SAMPLER(LogPopcornFXAttributeSamplerGrid, Error, grid, this, errorMsg);
			return false;
		}
		return true;
	}
	if (Properties.SizeX <= 0)
	{
		FString errorMsg = TEXT("Size X must be > 0");
#if WITH_EDITOR
		m_UnsupportedProperties.FindOrAdd(TEXT("SizeX"), errorMsg);
#endif
		UE_LOG_UNSUPPORTED_SAMPLER(LogPopcornFXAttributeSamplerGrid, Error, grid, this, errorMsg);
		return false;
	}
	if (Properties.SizeY <= 0)
	{
		FString errorMsg = TEXT("Size Y must be > 0");
#if WITH_EDITOR
		m_UnsupportedProperties.FindOrAdd(TEXT("SizeY"), errorMsg);
#endif
		UE_LOG_UNSUPPORTED_SAMPLER(LogPopcornFXAttributeSamplerGrid, Error, grid, this, errorMsg);
		return false;
	}
	if (Properties.SizeZ <= 0)
	{
		FString errorMsg = TEXT("Size Z must be > 0");
#if WITH_EDITOR
		m_UnsupportedProperties.FindOrAdd(TEXT("SizeZ"), errorMsg);
#endif
		UE_LOG_UNSUPPORTED_SAMPLER(LogPopcornFXAttributeSamplerGrid, Error, grid, this, errorMsg);
		return false;
	}

	if (Properties.Order == EPopcornFXGridOrder::OneD || Properties.Order == EPopcornFXGridOrder::TwoD)
	{
		if (Properties.DataType == EPopcornFXGridDataType::Int ||
			Properties.DataType == EPopcornFXGridDataType::Int2 ||
			Properties.DataType == EPopcornFXGridDataType::Int4)
		{
			FString errorMsg = TEXT("Int data types are only supported for 3D grids");
#if WITH_EDITOR
			m_UnsupportedProperties.FindOrAdd(TEXT("DataType"), *errorMsg);
#endif
			UE_LOG_UNSUPPORTED_SAMPLER(LogPopcornFXAttributeSamplerGrid, Error, grid, this, errorMsg);
			return false;
		}
	}

	PopcornFX::EBaseTypeID		dataType = ToPk(Properties.DataType);
	
	if (!PK_VERIFY(dataType != PopcornFX::EBaseTypeID::BaseType_Void))
		return false;

	if (dataType == PopcornFX::EBaseTypeID::BaseType_Float3 || dataType == PopcornFX::EBaseTypeID::BaseType_Int3)
	{
		FString errorMsg = FString::Printf(TEXT("Unsupported data type: %s"),
			dataType == PopcornFX::EBaseTypeID::BaseType_Float3 ? *FString("float3") : *FString("int3"));
#if WITH_EDITOR
		m_UnsupportedProperties.FindOrAdd(TEXT("DataType"), *errorMsg);
#endif
		UE_LOG_UNSUPPORTED_SAMPLER(LogPopcornFXAttributeSamplerGrid, Error, grid, this, errorMsg);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerGrid::ArePropertiesCompatible(UPopcornFXEmitterComponent *emitter, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	if (Properties.bAssetGrid)
	{
		return IsRenderTargetCompatible(Properties.RenderTarget, emitter, defaultSampler);
	}

	// Make sure the sampler matches what the effect expects
	// Mismatchs should only happen when using an external sampler
	const PopcornFX::CResourceDescriptor_Grid	*defaultGridSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Grid>(defaultSampler);
	if (!PK_VERIFY(defaultGridSampler != null))
		return false;

	FString										errorMsg;
	const u32									gridOrder = static_cast<u32>(Properties.Order + 1);
	const PopcornFX::Nodegraph::SDataTypeTraits &srcTypeTraits = PopcornFX::Nodegraph::SDataTypeTraits::Traits((PopcornFX::Nodegraph::EDataType)defaultGridSampler->Type());
	if (defaultGridSampler->Order() != gridOrder)
	{
		errorMsg = FString::Printf(TEXT("Effect '%s' needs a %dD grid, sampler '%s' is set to %dD"),
			*emitter->Effect->GetName(), defaultGridSampler->Order(), *GetName(), gridOrder);
#if WITH_EDITOR
		m_IncompatibleProperties.FindOrAdd(emitter).m_Properties.Add(TEXT("Other"), *errorMsg);
		m_Data->Clear();
		OnSamplerValidStateChanged.Broadcast();
#endif
		UE_LOG_INCOMPATIBLE_SAMPLER(LogPopcornFXAttributeSamplerGrid, Error, grid, this, emitter, errorMsg);
		return false;
	}

	PopcornFX::EBaseTypeID		dataType = ToPk(Properties.DataType);
	if (srcTypeTraits.BaseType() != dataType)
	{
		errorMsg = FString::Printf(TEXT("Effect '%s' uses %s data type, sampler '%s' is set to %s"),
			*emitter->Effect->GetName(), UTF8_TO_TCHAR(srcTypeTraits.Name()),
			*GetName(), UTF8_TO_TCHAR(PopcornFX::CBaseTypeTraits::Traits(dataType).Name));
#if WITH_EDITOR
		m_IncompatibleProperties.FindOrAdd(emitter).m_Properties.Add(TEXT("Other"), *errorMsg);
		m_Data->Clear();
		OnSamplerValidStateChanged.Broadcast();
#endif
		UE_LOG_INCOMPATIBLE_SAMPLER(LogPopcornFXAttributeSamplerGrid, Error, grid, this, emitter, errorMsg);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor	*UPopcornFXAttributeSamplerGrid::_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	const PopcornFX::CResourceDescriptor_Grid	*defaultGridSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Grid>(defaultSampler);
	if (!PK_VERIFY(defaultGridSampler != null))
		return null;

	if (Properties.bAssetGrid && Properties.RenderTarget && m_Data->m_Desc != null
		&& HasRenderTargetChanged())
	{
		m_Data->m_ReloadGrid = true;
	}

	if (m_Data->m_ReloadGrid)
	{
		if (!RebuildGridSampler(emitter, defaultSampler))
		{
#if WITH_EDITOR
			OnSamplerValidStateChanged.Broadcast();
			emitter->SetWarningSprite();
#endif
			return null;
		}
		m_Data->m_ReloadGrid = false;
	}

	return m_Data->m_Desc.Get();
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerGrid::RebuildGridSampler(UPopcornFXEmitterComponent *emitter, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	if (!_RebuildGridSampler(emitter, defaultSampler))
	{
		const FString	imageName = Properties.RenderTarget != null ? Properties.RenderTarget->GetName() : FString(TEXT("null"));
		UE_LOG(LogPopcornFXAttributeSamplerGrid, Warning,
				TEXT("Failed to setup grid attribute sampler '%s' (Owner = %s)"),
				*GetName(), GetOwner() != null ? *GetOwner()->GetName() : *FString(TEXT("null")));
		m_Data->Clear();
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerGrid::_RebuildGridSampler(UPopcornFXEmitterComponent *emitter, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerGrid::Build grid sampler", POPCORNFX_UE_PROFILER_COLOR);

	PopcornFX::PParticleSamplerDescriptor_Grid_Default	descriptor = PK_NEW(PopcornFX::CParticleSamplerDescriptor_Grid_Default);
	if (!PK_VERIFY(descriptor != null))
		return false;

	bool	needsGPUHandle = false;
#if (PK_GPU_D3D12 != 0)
	needsGPUHandle = g_PopcornFXRHIAPI == SUERenderContext::D3D12;
#endif

	EPixelFormat	pixelFormat = PF_Unknown;
	bool			isVolumeTexture = false;

	m_GridTexture = null;

	UTexture	*texture = null;
	if (Properties.bAssetGrid)
	{
		texture = Properties.RenderTarget;

		UTextureRenderTarget2D		*RT2D = Cast<UTextureRenderTarget2D>(texture);
		UTextureRenderTargetVolume	*RTVolume = Cast<UTextureRenderTargetVolume>(texture);
		if (RT2D != null)
		{
			const EPixelFormat	RTFormat = GetPixelFormatFromRenderTargetFormat(RT2D->RenderTargetFormat);
			const bool			hasCorrectGamma = Properties.bSRGB == RT2D->IsSRGB();

			if (!hasCorrectGamma)
			{
				UE_LOG(LogPopcornFXAttributeSamplerGrid, Warning, TEXT("Converting UTextureRenderTarget2D format to sRGB=%d"), Properties.bSRGB);
			}
			if ((!RT2D->bCanCreateUAV || !hasCorrectGamma) && needsGPUHandle)
			{
				RT2D->bCanCreateUAV = true;
				RT2D->SRGB = Properties.bSRGB;
				RT2D->UpdateResource();
			}

			PopcornFX::EBaseTypeID	dataType;
			switch (RTFormat)
			{
			case	PF_R32_FLOAT:
				dataType = PopcornFX::BaseType_Float;
				break;
			case	PF_G32R32F:
				dataType = PopcornFX::BaseType_Float2;
				break;
			case	PF_A32B32G32R32F:
			default:
				dataType = PopcornFX::BaseType_Float4;
				break;
			};
			descriptor->m_DataType = dataType;
			descriptor->m_GridDimensions = CUint4(RT2D->SizeX, RT2D->SizeY, 1, 1);
			pixelFormat = RTFormat;
		}
		else if (RTVolume != null)
		{
			const bool	hasCorrectGamma = Properties.bSRGB == RTVolume->SRGB;

			if (!hasCorrectGamma)
			{
				UE_LOG(LogPopcornFXAttributeSamplerGrid, Warning, TEXT("Converting UTextureRenderTargetVolume format to sRGB=%d"), Properties.bSRGB);
			}
			if ((!RTVolume->bCanCreateUAV || !hasCorrectGamma) || needsGPUHandle)
			{
				RTVolume->bCanCreateUAV = true;
				RTVolume->SRGB = Properties.bSRGB;
				RTVolume->UpdateResource();
			}
			descriptor->m_DataType = PopcornFX::BaseType_Float4;
			descriptor->m_GridDimensions = CUint4(RTVolume->SizeX, RTVolume->SizeY, RTVolume->SizeZ, 1);
			isVolumeTexture = true;
			pixelFormat = RTVolume->GetFormat();
		}
		else
		{
#if WITH_EDITOR
			m_UnsupportedProperties.FindOrAdd(TEXT("RenderTarget"), TEXT("Can only setup attribute sampler from UTextureRenderTarget2D or UTextureRenderTargetVolume"));
			m_IncompatibleProperties.FindOrAdd(emitter).m_Properties.Add(TEXT("RenderTarget"), TEXT("Can only setup attribute sampler from UTextureRenderTarget2D or UTextureRenderTargetVolume"));
#endif
			UE_LOG(LogPopcornFXAttributeSamplerGrid, Warning, TEXT("Couldn't build grid attribute sampler: Can only setup attribute sampler from UTextureRenderTarget2D or UTextureRenderTargetVolume"));
			return false;
		}
	}
	else
	{
		PopcornFX::EBaseTypeID		dataType = PopcornFX::BaseType_Void;

		switch (Properties.DataType)
		{
		case	EPopcornFXGridDataType::Float:
			dataType = PopcornFX::BaseType_Float;
			pixelFormat = PF_R32_FLOAT;
			break;
		case	EPopcornFXGridDataType::Float2:
			dataType = PopcornFX::BaseType_Float2;
			pixelFormat = PF_G32R32F;
			break;
		// Unsupported
		//case	EPopcornFXGridDataType::Float3:
		//	dataType = PopcornFX::BaseType_Float3;
		//	pixelFormat = PF_R32G32B32F;
		//	break;
		case	EPopcornFXGridDataType::Float4:
			dataType = PopcornFX::BaseType_Float4;
			pixelFormat = PF_A32B32G32R32F;
			break;
		case	EPopcornFXGridDataType::Int:
			dataType = PopcornFX::BaseType_I32;
			pixelFormat = PF_R32_UINT;
			break;
		case	EPopcornFXGridDataType::Int2:
			dataType = PopcornFX::BaseType_Int2;
			pixelFormat = PF_R32G32_UINT;
			break;
		// Unsupported
		//case	EPopcornFXGridDataType::Int3:
		//	dataType = PopcornFX::BaseType_Int3;
		//	pixelFormat = PF_R32G32B32_UINT;
		//	break;
		case	EPopcornFXGridDataType::Int4:
			dataType = PopcornFX::BaseType_Int4;
			pixelFormat = PF_R32G32B32A32_UINT;
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			return false;
		};

		if (needsGPUHandle)
		{
			if (Properties.Order == EPopcornFXGridOrder::ThreeD)
			{
				UTextureRenderTargetVolume	*newTexture = NewObject<UTextureRenderTargetVolume>(GetTransientPackage(), TEXT("PopcornFX Grid Attribute Sampler Volume"), RF_Transient);
				check(newTexture != null);
				newTexture->bCanCreateUAV = true;
				newTexture->SRGB = Properties.bSRGB;
				newTexture->Init(Properties.SizeX, Properties.SizeY, Properties.SizeZ, pixelFormat);
				newTexture->UpdateResourceImmediate(true);
				texture = newTexture;
				isVolumeTexture = true;
			}
			else if (Properties.Order == EPopcornFXGridOrder::OneD || Properties.Order == EPopcornFXGridOrder::TwoD)
			{
				if (Properties.DataType == EPopcornFXGridDataType::Int ||
					Properties.DataType == EPopcornFXGridDataType::Int2 ||
					Properties.DataType == EPopcornFXGridDataType::Int4)
				{
#if WITH_EDITOR
					m_UnsupportedProperties.FindOrAdd(TEXT("DataType"), TEXT("Can only build 1D and 2D textures with float data types"));
					m_IncompatibleProperties.FindOrAdd(emitter).m_Properties.Add(TEXT("DataType"), TEXT("Can only build 1D and 2D textures with float data types"));
#endif
					UE_LOG(LogPopcornFXAttributeSamplerGrid, Warning, TEXT("Couldn't build grid attribute sampler: Can only build 1D and 2D textures with float data types"));
					return false;
				}
				UTextureRenderTarget2D	*newTexture = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), TEXT("PopcornFX Grid Attribute Sampler 2D"), RF_Transient);
				check(newTexture != null);
				newTexture->bAutoGenerateMips = false;
				newTexture->bCanCreateUAV = true;
				newTexture->SRGB = Properties.bSRGB;
				newTexture->InitCustomFormat(Properties.SizeX, Properties.SizeY, pixelFormat, Properties.bSRGB);
				newTexture->UpdateResourceImmediate(true);
				texture = newTexture;
			}
			else
			{
#if WITH_EDITOR
				m_UnsupportedProperties.FindOrAdd(TEXT("Order"), TEXT("Only 1D, 2D and 3D grids are supported"));
				m_IncompatibleProperties.FindOrAdd(emitter).m_Properties.Add(TEXT("Order"), TEXT("Only 1D, 2D and 3D grids are supported"));
#endif
				UE_LOG(LogPopcornFXAttributeSamplerGrid, Warning, TEXT("Couldn't build grid attribute sampler: Only 1D, 2D and 3D grids are supported"));
				return false;
			}
			if (texture == null)
			{
#if WITH_EDITOR
				m_UnsupportedProperties.FindOrAdd(TEXT("Other"), TEXT("Couldn't create transient texture"));
				m_IncompatibleProperties.FindOrAdd(emitter).m_Properties.Add(TEXT("Other"), TEXT("Couldn't create transient texture"));
#endif
				UE_LOG(LogPopcornFXAttributeSamplerGrid, Warning, TEXT("Couldn't build grid attribute sampler: couldn't create transient texture"));
				return false;
			}
			m_GridTexture = texture;
		}
		descriptor->m_DataType = dataType;
		descriptor->m_GridDimensions = CUint4(Properties.SizeX, Properties.SizeY, Properties.SizeZ, 1);
	}

	if (needsGPUHandle)
	{
		check(texture != null);

		// WIP: Flush render commands to make sure the RHI resource is available.
		FlushRenderingCommands();
	}

	// Attribute samplers don't work with D3D11
#if (PK_GPU_D3D12 != 0)
	// There is no way currently to know if the current sampler descriptor will be used by the CPU or GPU sim, so we'll have to load both, if possible
	// GPU sim image load is trivial: it will just grab the ref to the native resource
	if (g_PopcornFXRHIAPI == SUERenderContext::D3D12)
	{
		// Simplified CResourceHandlerImage_UE_D3D12::NewFromTexture()
		FTextureReferenceRHIRef	texRef = texture->TextureReference.TextureReferenceRHI;
		if (!IsValidRef(texRef))
		{
#if WITH_EDITOR
			m_IncompatibleProperties.FindOrAdd(emitter).m_Properties.Add(TEXT("Other"), TEXT("UTexture TextureReference not available"));
#endif
			UE_LOG(LogPopcornFXAttributeSamplerGrid, Warning, TEXT("Couldn't build grid attribute sampler: UTexture TextureReference not available \"%s\""), *texture->GetPathName());
			return false;
		}
		FRHITexture	*texRHI = texRef->GetReferencedTexture();
		if (texRHI == null)
		{
#if WITH_EDITOR
			m_IncompatibleProperties.FindOrAdd(emitter).m_Properties.Add(TEXT("Other"), TEXT("UTexture TextureReference FRHITexture not available"));
#endif
			UE_LOG(LogPopcornFXAttributeSamplerGrid, Warning, TEXT("Couldn't build grid attribute sampler: UTexture TextureReference FRHITexture not available \"%s\""), *texture->GetPathName());
			return false;
		}
		ID3D12Resource	*gpuTexture = static_cast<ID3D12Resource*>(texRHI->GetNativeResource());
		if (gpuTexture == null)
		{
#if WITH_EDITOR
			m_IncompatibleProperties.FindOrAdd(emitter).m_Properties.Add(TEXT("Other"), TEXT("UTexture TextureReference FRHITexture D3D12 not available"));
#endif
			UE_LOG(LogPopcornFXAttributeSamplerGrid, Warning, TEXT("Couldn't build grid attribute sampler: UTexture TextureReference FRHITexture D3D12 not available \"%s\""), *texture->GetPathName());
			return false;
		}

		descriptor->SetupD3D12Resources(gpuTexture,
										(u32)GPixelFormats[pixelFormat].PlatformFormat,
										(u32)(isVolumeTexture ? D3D12_UAV_DIMENSION_TEXTURE3D : D3D12_UAV_DIMENSION_TEXTURE2D));
	}
#endif // PK_GPU_D3D12 != 0

	// Build a CPU sim visible memory buffer
	{
		const u32	dataSizeInBytes = descriptor->m_GridDimensions.AxialProduct() * PopcornFX::CBaseTypeTraits::Traits(descriptor->m_DataType).Size;

		descriptor->m_RawDataRef = PopcornFX::CRefCountedMemoryBuffer::CallocAligned(dataSizeInBytes, 0x10);
		if (!PK_VERIFY(descriptor->m_RawDataRef != null))
			return false;
		descriptor->m_RawDataPtr = descriptor->m_RawDataRef->Data<u8>();
		descriptor->m_RawDataByteCount = descriptor->m_RawDataRef->DataSizeInBytes();
	}

	m_Data->m_Desc = descriptor;
	return true;
}

//---------------------------------------------------------------------------
// 
//	Read functions
// 
//---------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerGrid::CanReadFromGrid(UPopcornFXAttributeSamplerGrid *Grid, EPopcornFXGridDataType::Type WantedType)
{
	if (!Grid)
	{
		UE_LOG(LogPopcornFXAttributeSamplerGrid, Error, TEXT("Null grid sampler when trying to read from grid attribute sampler"));
		return false;
	}

	if (Grid->Properties.DataType != WantedType)
	{
		UE_LOG(LogPopcornFXAttributeSamplerGrid, Error, TEXT("Trying to read %s values from a %s grid attribute sampler '%s'"
			" in actor '%s'%s"),
			*DataTypeToString(WantedType), *DataTypeToString(Grid->Properties.DataType),
			*Grid->GetName(), Grid->GetAttachmentRootActor() ? *Grid->GetAttachmentRootActor()->GetName() : TEXT("null"),
			Grid->bIsInline ? *FString::Printf(TEXT(" for emitter '%s'"), *Grid->GetOuter()->GetName()) : TEXT(""));
		return false;
	}

	if (!Grid->m_Data || Grid->m_Data->m_Desc == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplerGrid, Error, TEXT("Null grid descriptor when trying to read from grid attribute sampler '%s'"
			" in actor '%s'%s"),
			*Grid->GetName(), Grid->GetAttachmentRootActor() ? *Grid->GetAttachmentRootActor()->GetName() : TEXT("null"),
			Grid->bIsInline ? *FString::Printf(TEXT(" for emitter '%s'"), *Grid->GetOuter()->GetName()) : TEXT(""));
		return false;
	}

	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerGrid::ReadGridFloatValues(UPopcornFXAttributeSamplerGrid *InSelf, TArray<float> &OutValues)
{
	if (!CanReadFromGrid(InSelf, EPopcornFXGridDataType::Float))
		return false;
	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->GetCellCount();

	PopcornFX::TStridedMemoryView<float> values(reinterpret_cast<float*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(float));

	OutValues.SetNum(expectedCount);
	for (u32 i = 0; i < expectedCount; i++)
	{
		OutValues[i] = values[i];
	}
	return true;
}

bool	UPopcornFXAttributeSamplerGrid::ReadGridFloat2Values(UPopcornFXAttributeSamplerGrid *InSelf, TArray<FVector2D> &OutValues)
{
	if (!CanReadFromGrid(InSelf, EPopcornFXGridDataType::Float2))
		return false;

	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->GetCellCount();

	PopcornFX::TStridedMemoryView<PopcornFX::CFloat2> values(reinterpret_cast<PopcornFX::CFloat2*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(PopcornFX::CFloat2));

	OutValues.SetNum(expectedCount);
	for (u32 i = 0; i < expectedCount; i++)
	{
		OutValues[i].X = values[i].x();
		OutValues[i].Y = values[i].y();
	}
	return true;
}

bool	UPopcornFXAttributeSamplerGrid::ReadGridFloat3Values(UPopcornFXAttributeSamplerGrid *InSelf, TArray<FVector> &OutValues)
{
	if (!CanReadFromGrid(InSelf, EPopcornFXGridDataType::Float3))
		return false;

	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->Properties.SizeX *InSelf->Properties.SizeY *InSelf->Properties.SizeZ;

	PopcornFX::TStridedMemoryView<PopcornFX::CFloat3> values(reinterpret_cast<PopcornFX::CFloat3*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(PopcornFX::CFloat3));

	OutValues.SetNum(expectedCount);
	for (u32 i = 0; i < expectedCount; i++)
	{
		OutValues[i].X = values[i].x();
		OutValues[i].Y = values[i].y();
		OutValues[i].Z = values[i].y();
	}
	return true;
}

bool	UPopcornFXAttributeSamplerGrid::ReadGridFloat4Values(UPopcornFXAttributeSamplerGrid *InSelf, TArray<FVector4> &OutValues)
{
	if (!CanReadFromGrid(InSelf, EPopcornFXGridDataType::Float4))
		return false;

	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->GetCellCount();

	PopcornFX::TStridedMemoryView<PopcornFX::CFloat4> values(reinterpret_cast<PopcornFX::CFloat4*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(PopcornFX::CFloat4));

	OutValues.SetNum(expectedCount);
	for (u32 i = 0; i < expectedCount; i++)
	{
		OutValues[i].X = values[i].x();
		OutValues[i].Y = values[i].y();
		OutValues[i].Z = values[i].y();
		OutValues[i].W = values[i].w();
	}
	return true;
}

bool	UPopcornFXAttributeSamplerGrid::ReadGridIntValues(UPopcornFXAttributeSamplerGrid *InSelf, TArray<int> &OutValues)
{
	if (!CanReadFromGrid(InSelf, EPopcornFXGridDataType::Int))
		return false;

	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->GetCellCount();

	PopcornFX::TStridedMemoryView<int> values(reinterpret_cast<int*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(int));

	OutValues.SetNum(expectedCount);
	for (u32 i = 0; i < expectedCount; i++)
	{
		OutValues[i] = values[i];
	}
	return true;
}

bool	UPopcornFXAttributeSamplerGrid::ReadGridInt2Values(UPopcornFXAttributeSamplerGrid *InSelf, TArray<FIntPoint> &OutValues)
{
	if (!CanReadFromGrid(InSelf, EPopcornFXGridDataType::Int2))
		return false;

	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->GetCellCount();

	PopcornFX::TStridedMemoryView<PopcornFX::CInt2> values(reinterpret_cast<PopcornFX::CInt2*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(PopcornFX::CInt2));

	OutValues.SetNum(expectedCount);
	for (u32 i = 0; i < expectedCount; i++)
	{
		OutValues[i].X = values[i].x();
		OutValues[i].Y = values[i].y();
	}
	return true;
}

bool	UPopcornFXAttributeSamplerGrid::ReadGridInt3Values(UPopcornFXAttributeSamplerGrid *InSelf, TArray<FIntVector> &OutValues)
{
	if (!CanReadFromGrid(InSelf, EPopcornFXGridDataType::Int3))
		return false;

	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->GetCellCount();

	PopcornFX::TStridedMemoryView<PopcornFX::CInt3> values(reinterpret_cast<PopcornFX::CInt3*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(PopcornFX::CInt3));

	OutValues.SetNum(expectedCount);
	for (u32 i = 0; i < expectedCount; i++)
	{
		OutValues[i].X = values[i].x();
		OutValues[i].Y = values[i].y();
		OutValues[i].Z = values[i].y();
	}
	return true;
}

bool	UPopcornFXAttributeSamplerGrid::ReadGridInt4Values(UPopcornFXAttributeSamplerGrid *InSelf, TArray<FIntVector4> &OutValues)
{
	if (!CanReadFromGrid(InSelf, EPopcornFXGridDataType::Int4))
		return false;

	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->GetCellCount();

	PopcornFX::TStridedMemoryView<PopcornFX::CInt4> values(reinterpret_cast<PopcornFX::CInt4*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(PopcornFX::CInt4));

	OutValues.SetNum(expectedCount);
	for (u32 i = 0; i < expectedCount; i++)
	{
		OutValues[i].X = values[i].x();
		OutValues[i].Y = values[i].y();
		OutValues[i].Z = values[i].y();
		OutValues[i].Z = values[i].w();
	}
	return true;
}

//---------------------------------------------------------------------------
// 
//	Write functions
// 
//---------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerGrid::CanWriteToGrid(UPopcornFXAttributeSamplerGrid *Grid, EPopcornFXGridDataType::Type WantedType, const int32 InValuesCount)
{
	if (!Grid)
	{
		UE_LOG(LogPopcornFXAttributeSamplerGrid, Error, TEXT("Null grid sampler when trying to write into grid attribute sampler"));
			return false;
	}

	if (Grid->Properties.DataType != WantedType)
	{
		UE_LOG(LogPopcornFXAttributeSamplerGrid, Error, TEXT("Trying to write %s values into a %s grid attribute sampler '%s'"
			" in actor '%s'%s"),
			*DataTypeToString(WantedType), *DataTypeToString(Grid->Properties.DataType),
			*Grid->GetName(), Grid->GetAttachmentRootActor() ? *Grid->GetAttachmentRootActor()->GetName() : TEXT("null"),
			Grid->bIsInline ? *FString::Printf(TEXT(" for emitter '%s'"), *Grid->GetOuter()->GetName()) : TEXT(""));
		return false;
	}

	if (!PK_VERIFY(InValuesCount == Grid->GetCellCount()))
	{
		UE_LOG(LogPopcornFXAttributeSamplerGrid, Error, TEXT("Invalid input array size when trying to write into grid attribute sampler '%s'"
			" in actor '%s'%s. Grid size: %d (%d x %d x %d). Input array size: %d"),
			*Grid->GetName(), Grid->GetAttachmentRootActor() ? *Grid->GetAttachmentRootActor()->GetName() : TEXT("null"),
			Grid->bIsInline ? *FString::Printf(TEXT(" for emitter '%s'"), *Grid->GetOuter()->GetName()) : TEXT(""),
			Grid->GetCellCount(), Grid->Properties.SizeX, Grid->Properties.SizeY, Grid->Properties.SizeZ, InValuesCount);
		return false;
	}
	if (!Grid->m_Data || Grid->m_Data->m_Desc == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplerGrid, Error, TEXT("Null grid descriptor when trying to write into grid attribute sampler '%s'"
			" in actor '%s'%s"),
			*Grid->GetName(), Grid->GetAttachmentRootActor() ? *Grid->GetAttachmentRootActor()->GetName() : TEXT("null"),
			Grid->bIsInline ? *FString::Printf(TEXT(" for emitter '%s'"), *Grid->GetOuter()->GetName()) : TEXT(""));
			return false;
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerGrid::WriteGridFloatValues(UPopcornFXAttributeSamplerGrid *InSelf, const TArray<float> &InValues)
{
	if (!CanWriteToGrid(InSelf, EPopcornFXGridDataType::Float, InValues.Num()))
		return false;

	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->GetCellCount();

	PopcornFX::TStridedMemoryView<float> values(reinterpret_cast<float*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(float));

	for (u32 i = 0; i < expectedCount; i++)
	{
		values[i] = InValues[i];
	}
	return true;
}

bool	UPopcornFXAttributeSamplerGrid::WriteGridFloat2Values(UPopcornFXAttributeSamplerGrid *InSelf, const TArray<FVector2D> &InValues)
{
	if (!CanWriteToGrid(InSelf, EPopcornFXGridDataType::Float2, InValues.Num()))
		return false;

	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->GetCellCount();

	PopcornFX::TStridedMemoryView<PopcornFX::CFloat2> values(reinterpret_cast<PopcornFX::CFloat2*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(PopcornFX::CFloat2));

	for (u32 i = 0; i < expectedCount; i++)
	{
		values[i].x() = InValues[i].X;
		values[i].y() = InValues[i].Y;
	}
	return true;
}

bool	UPopcornFXAttributeSamplerGrid::WriteGridFloat3Values(UPopcornFXAttributeSamplerGrid *InSelf, const TArray<FVector> &InValues)
{
	if (!CanWriteToGrid(InSelf, EPopcornFXGridDataType::Float3, InValues.Num()))
		return false;

	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->GetCellCount();

	PopcornFX::TStridedMemoryView<PopcornFX::CFloat3> values(reinterpret_cast<PopcornFX::CFloat3*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(PopcornFX::CFloat3));

	for (u32 i = 0; i < expectedCount; i++)
	{
		values[i].x() = InValues[i].X;
		values[i].y() = InValues[i].Y;
		values[i].z() = InValues[i].Z;
	}
	return true;
}

bool	UPopcornFXAttributeSamplerGrid::WriteGridFloat4Values(UPopcornFXAttributeSamplerGrid *InSelf, const TArray<FVector4> &InValues)
{
	if (!CanWriteToGrid(InSelf, EPopcornFXGridDataType::Float4, InValues.Num()))
		return false;

	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->GetCellCount();

	PopcornFX::TStridedMemoryView<PopcornFX::CFloat4> values(reinterpret_cast<PopcornFX::CFloat4*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(PopcornFX::CFloat4));

	for (u32 i = 0; i < expectedCount; i++)
	{
		values[i].x() = InValues[i].X;
		values[i].y() = InValues[i].Y;
		values[i].z() = InValues[i].Z;
		values[i].w() = InValues[i].W;
	}
	return true;
}

bool	UPopcornFXAttributeSamplerGrid::WriteGridIntValues(UPopcornFXAttributeSamplerGrid *InSelf, const TArray<int> &InValues)
{
	if (!CanWriteToGrid(InSelf, EPopcornFXGridDataType::Int, InValues.Num()))
		return false;

	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->GetCellCount();

	PopcornFX::TStridedMemoryView<int> values(reinterpret_cast<int*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(int));

	for (u32 i = 0; i < expectedCount; i++)
	{
		values[i] = InValues[i];
	}
	return true;
}

bool	UPopcornFXAttributeSamplerGrid::WriteGridInt2Values(UPopcornFXAttributeSamplerGrid *InSelf, const TArray<FIntPoint> &InValues)
{
	if (!CanWriteToGrid(InSelf, EPopcornFXGridDataType::Int2, InValues.Num()))
		return false;

	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->GetCellCount();

	PopcornFX::TStridedMemoryView<PopcornFX::CInt2> values(reinterpret_cast<PopcornFX::CInt2*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(PopcornFX::CInt2));

	for (u32 i = 0; i < expectedCount; i++)
	{
		values[i].x() = InValues[i].X;
		values[i].y() = InValues[i].Y;
	}
	return true;
}

bool	UPopcornFXAttributeSamplerGrid::WriteGridInt3Values(UPopcornFXAttributeSamplerGrid *InSelf, const TArray<FIntVector> &InValues)
{
	if (!CanWriteToGrid(InSelf, EPopcornFXGridDataType::Int3, InValues.Num()))
		return false;

	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->GetCellCount();

	PopcornFX::TStridedMemoryView<PopcornFX::CInt3> values(reinterpret_cast<PopcornFX::CInt3*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(PopcornFX::CInt3));

	for (u32 i = 0; i < expectedCount; i++)
	{
		values[i].x() = InValues[i].X;
		values[i].y() = InValues[i].Y;
		values[i].z() = InValues[i].Z;
	}
	return true;
}

bool	UPopcornFXAttributeSamplerGrid::WriteGridInt4Values(UPopcornFXAttributeSamplerGrid *InSelf, const TArray<FIntVector4> &InValues)
{
	if (!CanWriteToGrid(InSelf, EPopcornFXGridDataType::Int4, InValues.Num()))
		return false;

	// CPU Grids
	// TODO: GPU Grids
	const u32	expectedCount = InSelf->GetCellCount();

	PopcornFX::TStridedMemoryView<PopcornFX::CInt4> values(reinterpret_cast<PopcornFX::CInt4*>(InSelf->m_Data->m_Desc->m_RawDataPtr),
		expectedCount, sizeof(PopcornFX::CInt4));

	for (u32 i = 0; i < expectedCount; i++)
	{
		values[i].x() = InValues[i].X;
		values[i].y() = InValues[i].Y;
		values[i].z() = InValues[i].Z;
		values[i].w() = InValues[i].W;
	}
	return true;
}

//----------------------------------------------------------------------------

int32	UPopcornFXAttributeSamplerGrid::GetCellCount() const
{
	if (Properties.bAssetGrid && Properties.RenderTarget)
	{
		UTextureRenderTarget2D *RT2D = Cast<UTextureRenderTarget2D>(Properties.RenderTarget);
		UTextureRenderTargetVolume *RTVolume = Cast<UTextureRenderTargetVolume>(Properties.RenderTarget);
		if (RT2D != null)
		{
			return RT2D->SizeX * RT2D->SizeY;
		}
		else if (RTVolume != null)
		{
			return RTVolume->SizeX * RTVolume->SizeY * RTVolume->SizeZ;
		}
	}
	// Check the order because reimported effects can still have the value of Y and Z when they lost one or more dimension(s)
	switch (Properties.Order)
	{
	case EPopcornFXGridOrder::OneD:
		return Properties.SizeX;
	case EPopcornFXGridOrder::TwoD:
		return Properties.SizeX * Properties.SizeY;
	case EPopcornFXGridOrder::ThreeD:
		return Properties.SizeX * Properties.SizeY * Properties.SizeZ;
	default:
		PK_ASSERT_NOT_REACHED();
		return 0;
	}
}

//----------------------------------------------------------------------------

FIntVector	UPopcornFXAttributeSamplerGrid::GetDimensions() const
{
	FIntVector	dimensions;
	dimensions.X = 1;
	dimensions.Y = 1;
	dimensions.Z = 1;

	if (Properties.bAssetGrid && Properties.RenderTarget)
	{
		UTextureRenderTarget2D *RT2D = Cast<UTextureRenderTarget2D>(Properties.RenderTarget);
		UTextureRenderTargetVolume *RTVolume = Cast<UTextureRenderTargetVolume>(Properties.RenderTarget);
		if (RT2D != null)
		{
			dimensions.X = RT2D->SizeX;
			dimensions.Y = RT2D->SizeY;
		}
		else if (RTVolume != null)
		{
			dimensions.X = RTVolume->SizeX;
			dimensions.Y = RTVolume->SizeY;
			dimensions.Z = RTVolume->SizeZ;
		}
	}
	else
	{
		dimensions.X = Properties.SizeX;
		// Check the order because reimported effects can still have the value of Y and Z when they lost one or more dimension(s)
		if (Properties.Order >= EPopcornFXGridOrder::ThreeD)
			dimensions.Z = Properties.SizeZ;
		if (Properties.Order >= EPopcornFXGridOrder::TwoD)
			dimensions.Y = Properties.SizeY;
	}
	return dimensions;
}

//----------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
