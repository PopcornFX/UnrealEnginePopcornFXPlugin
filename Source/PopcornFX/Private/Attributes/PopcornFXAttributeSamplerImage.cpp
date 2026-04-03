//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeSamplerImage.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXStats.h"
#include "PopcornFXHelper.h"
#include "PopcornFXAttributeList.h"
#include "GPUSim/PopcornFXGPUSim.h"
#include "Assets/PopcornFXEffect.h"
#include "Assets/PopcornFXEffectPriv.h"
#include "Assets/PopcornFXTextureAtlas.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_samplers_classes.h>

#if (PK_GPU_D3D12 != 0)
#	include <pk_particles/include/Samplers/D3D12/image_gpu_d3d12.h>
#	include <pk_particles/include/Samplers/D3D12/rectangle_list_gpu_d3d12.h>

#	include "Windows/HideWindowsPlatformTypes.h"
#	include "D3D12RHIPrivate.h"
#	include "D3D12Util.h"
#endif // (PK_GPU_D3D12 != 0)

#include "Engine/Texture.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSamplerImage"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSamplerImage, Log, All);

//----------------------------------------------------------------------------
//
// UPopcornFXAttributeSamplerImage
//
//----------------------------------------------------------------------------

struct FAttributeSamplerImageData
{
	PopcornFX::PParticleSamplerDescriptor_Image_Default			m_Desc;
	PopcornFX::CImageSampler *m_ImageSampler = null;
	PopcornFX::TResourcePtr<PopcornFX::CImage>					m_TextureResource;
	PopcornFX::TResourcePtr<PopcornFX::CRectangleList>			m_TextureAtlasResource;
#if (PK_GPU_D3D12 != 0)
	PopcornFX::TResourcePtr<PopcornFX::CImageGPU_D3D12>			m_TextureResource_D3D12;
	PopcornFX::TResourcePtr<PopcornFX::CRectangleListGPU_D3D12>	m_TextureAtlasResource_D3D12;
#endif // (PK_GPU_D3D12 != 0)
	PopcornFX::SDensitySamplerData *m_DensityData = null;

	bool		m_ReloadTexture = true;
	bool		m_ReloadTextureAtlas = true;
	bool		m_RebuildPDF = true;

	void		Clear()
	{
		m_Desc = null;
		m_TextureResource.Clear();
#if (PK_GPU_D3D12 != 0)
		m_TextureResource_D3D12.Clear();
		m_TextureAtlasResource_D3D12.Clear();
#endif // (PK_GPU_D3D12 != 0)
		m_TextureAtlasResource.Clear();
		if (m_DensityData != null)
			m_DensityData->Clear();
		if (m_Desc != null)
			m_Desc->ClearDensity();
	}

	FAttributeSamplerImageData()
	{
	}

	~FAttributeSamplerImageData()
	{
		if (m_DensityData != null)
			PK_SAFE_DELETE(m_DensityData);
		if (m_ImageSampler != null)
			PK_SAFE_DELETE(m_ImageSampler);
	}

};

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerImage::SetTexture(class UTexture *InTexture)
{
	Properties.Texture = InTexture;
	m_Data->m_ReloadTexture = true;
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerImage::UPopcornFXAttributeSamplerImage(const FObjectInitializer &PCIP)
	: Super(PCIP)
{
	bAutoActivate = true;

	Properties.bAllowTextureConversionAtRuntime = false;

	Properties.SamplingMode = EPopcornFXImageSamplingMode::Regular;
	Properties.DensitySource = EPopcornFXImageDensitySource::RGBA_Average;
	Properties.DensityPower = 1.0f;

	Properties.Texture = null;
	Properties.TextureAtlas = null;
	// UPopcornFXAttributeSampler override:
	m_SamplerType = EPopcornFXAttributeSamplerType::Image;

	m_Data = new FAttributeSamplerImageData();
	check(m_Data != null);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerImage::OnUnregister()
{
	if (m_Data != null)
	{
		// Unregister the component during OnUnregister instead of BeginDestroy.
		// In editor mode, BeginDestroy is only called when saving a level:
		// Components ReregisterComponent() do not have a matching BeginDestroy call in editor
		m_Data->m_Desc = null;
	}
	Super::OnUnregister();
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerImage::BeginDestroy()
{
	if (m_Data != null)
	{
		delete m_Data;
		m_Data = null;
	}
	Super::BeginDestroy();
}

//----------------------------------------------------------------------------

#if WITH_EDITOR

void	UPopcornFXAttributeSamplerImage::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
	if (propertyChangedEvent.Property != NULL)
	{
		const FString	propertyName = propertyChangedEvent.Property->GetName();

		if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, Texture) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, bAllowTextureConversionAtRuntime))
		{
			m_Data->m_ReloadTexture = true;
			m_Data->m_RebuildPDF = true;
		}
		else if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, TextureAtlas))
		{
			m_Data->m_ReloadTextureAtlas = true;
			m_Data->m_RebuildPDF = true;
		}
		else if (propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, SamplingMode) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, DensitySource) ||
			propertyName == GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXAttributeSamplerPropertiesImage, DensityPower))
		{
			m_Data->m_RebuildPDF = true;
		}
	}

	Super::PostEditChangeProperty(propertyChangedEvent);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerImage::CopyPropertiesFrom(const UPopcornFXAttributeSampler *other)
{
	const FPopcornFXAttributeSamplerPropertiesImage *newImageProperties = static_cast<const FPopcornFXAttributeSamplerPropertiesImage *>(other->GetProperties());
	if (!PK_VERIFY(newImageProperties != null))
	{
		UE_LOG(LogPopcornFXAttributeSamplerImage, Error, TEXT("New properties are null or not image properties"));
		return;
	}

	Super::CopyPropertiesFrom(other);

	if (newImageProperties->Texture != Properties.Texture ||
		newImageProperties->bAllowTextureConversionAtRuntime != Properties.bAllowTextureConversionAtRuntime)
	{
		m_Data->m_ReloadTexture = true;
		m_Data->m_RebuildPDF = true;
	}
	else if (newImageProperties->TextureAtlas != Properties.TextureAtlas)
	{
		m_Data->m_ReloadTextureAtlas = true;
		m_Data->m_RebuildPDF = true;
	}
	else if (newImageProperties->SamplingMode != Properties.SamplingMode ||
		newImageProperties->DensitySource != Properties.DensitySource ||
		newImageProperties->DensityPower != Properties.DensityPower)
	{
		m_Data->m_RebuildPDF = true;
	}

	Properties = *newImageProperties;
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplerImage::SetupDefaults(UPopcornFXEffect *effect, const uint32 samplerIdx, bool updateUnlockedValues)
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
	const PopcornFX::CParticleAttributeSamplerDeclaration *const	samplerDesc = attrListPtr->UniqueSamplerList()[samplerIdx];
	PK_ASSERT(samplerDesc != null);
	if (samplerDesc == null)
	{
		return;
	}
	const PopcornFX::PResourceDescriptor		defaultSampler = samplerDesc->AttribSamplerDefaultValue();

	const PopcornFX::CResourceDescriptor_Image *image = PopcornFX::HBO::Cast<PopcornFX::CResourceDescriptor_Image>(defaultSampler.Get());
	if (image != null)
	{
		if (updateUnlockedValues)
		{
			FAssetRegistryModule &AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

			// Try to fetch the texture
			FString	uePath = TEXT("/Game/");
			uePath.Append(ToUE(image->TextureResource()));
			FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(uePath);
			UTexture *texture = Cast<UTexture>(AssetData.GetAsset());
			if (texture)
			{
				Properties.Texture = texture;
			}

			// Try to fetch the atlas
			uePath = TEXT("/Game/");
			uePath.Append(ToUE(image->AtlasDefinition()));
			AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(uePath);
			UPopcornFXTextureAtlas *atlas = Cast<UPopcornFXTextureAtlas>(AssetData.GetAsset());
			if (atlas)
			{
				Properties.TextureAtlas = atlas;
			}

			if (image->SampleRawValues())
			{
				Properties.SamplingMode = EPopcornFXImageSamplingMode::Regular;
			}
			else
			{
				Properties.DensityPower = image->DensityPower();
				Properties.DensitySource = static_cast<EPopcornFXImageDensitySource::Type>(image->DensitySrc());
			}
		}
	}
}

#endif // WITH_EDITOR

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerImage::ArePropertiesSupported()
{
	if (!Properties.Texture)
	{
		FString errorMsg = TEXT("Null texture");
#if WITH_EDITOR
		m_UnsupportedProperties.FindOrAdd(TEXT("Texture"), *errorMsg);
#endif
		UE_LOG_UNSUPPORTED_SAMPLER(LogPopcornFXAttributeSamplerImage, Error, grid, this, errorMsg);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerImage::ArePropertiesCompatible(UPopcornFXEmitterComponent *emitter, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	return true;
}

//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor *UPopcornFXAttributeSamplerImage::_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	LLM_SCOPE(ELLMTag::Particles);
	const PopcornFX::CResourceDescriptor_Image *defaultImageSampler = PopcornFX::HBO::Cast<const PopcornFX::CResourceDescriptor_Image>(defaultSampler);
	if (!PK_VERIFY(defaultImageSampler != null))
		return null;
	if (!/*PK_VERIFY*/(RebuildImageSampler()))
	{
		const FString	imageName = Properties.Texture != null ? Properties.Texture->GetName() : FString(TEXT("null"));
		const FString	atlasName = Properties.TextureAtlas != null ? Properties.TextureAtlas->GetName() : FString(TEXT("null"));
		// Do we really want to warn if the texture is just not set -> it's just going to use the default one?
		UE_LOG(LogPopcornFXAttributeSamplerImage, Warning, TEXT("AttrSamplerImage: Failed to setup texture '%s' with atlas '%s' in '%s'"), *imageName, *atlasName, *GetPathName());
		return null;
	}
	return m_Data->m_Desc.Get();
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerImage::RebuildImageSampler()
{
	if (!_RebuildImageSampler())
	{
		m_Data->Clear();
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerImage::_RebuildImageSampler()
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerImage::Build image sampler", POPCORNFX_UE_PROFILER_COLOR);

	if (Properties.Texture == null)
		return false;

#if (PK_HAS_GPU != 0)
	bool	updateGPUTextureBinding = false;
#endif // (PK_HAS_GPU != 0)

	const bool	rebuildImage = m_Data->m_ReloadTexture;
	if (rebuildImage)
	{
		const PopcornFX::CString	fullPath = ToPk(Properties.Texture->GetPathName());
		bool						success = false;
		m_Data->m_TextureResource = PopcornFX::Resource::DefaultManager()->Load<PopcornFX::CImage>(fullPath, true);
		success |= (m_Data->m_TextureResource != null && !m_Data->m_TextureResource->Empty());
		if (success)
		{
			if (!PK_VERIFY(!m_Data->m_TextureResource->m_Frames.Empty()) ||
				!PK_VERIFY(!m_Data->m_TextureResource->m_Frames[0].m_Mipmaps.Empty()))
				return false;
		}

		// Try loading the image with GPU sim resource handlers:
		// There is no way currently to know if the current sampler descriptor will be used by the CPU or GPU sim, so we'll have to load both, if possible
		// GPU sim image load is trivial: it will just grab the ref to the native resource
#if (PK_GPU_D3D12 != 0)
		if (g_PopcornFXRHIAPI == SUERenderContext::D3D12)
		{
			m_Data->m_TextureResource_D3D12 = PopcornFX::Resource::DefaultManager()->Load<PopcornFX::CImageGPU_D3D12>(fullPath, true);
			success |= (m_Data->m_TextureResource_D3D12 != null && !m_Data->m_TextureResource_D3D12->Empty());
		}
#endif // (PK_GPU_D3D12 != 0)

		if (!success) // Couldn't load any of the resources (CPU/GPU)
		{
			UE_LOG(LogPopcornFXAttributeSamplerImage, Warning, TEXT("AttrSamplerImage: couldn't load texture '%s' for CPU/GPU sim sampling"), *ToUE(fullPath));
			return false;
		}
		m_Data->m_ReloadTexture = false;
	}
	const bool	reloadImageAtlas = m_Data->m_ReloadTextureAtlas;
	if (reloadImageAtlas)
	{
		if (Properties.TextureAtlas != null)
		{
			bool						success = false;
			const PopcornFX::CString	fullPath = ToPk(Properties.TextureAtlas->GetPathName());
			m_Data->m_TextureAtlasResource = PopcornFX::Resource::DefaultManager()->Load<PopcornFX::CRectangleList>(fullPath, true);
			success |= (m_Data->m_TextureAtlasResource != null && !m_Data->m_TextureAtlasResource->Empty());

			// Try loading the image atlas with GPU sim resource handlers:
			// There is no way currently to know if the current sampler descriptor will be used by the CPU or GPU sim, so we'll have to load both, if possible
#if (PK_GPU_D3D12 != 0)
			if (g_PopcornFXRHIAPI == SUERenderContext::D3D12)
			{
				m_Data->m_TextureAtlasResource_D3D12 = PopcornFX::Resource::DefaultManager()->Load<PopcornFX::CRectangleListGPU_D3D12>(fullPath, true);
				success |= (m_Data->m_TextureAtlasResource_D3D12 != null && !m_Data->m_TextureAtlasResource_D3D12->Empty());
			}
#endif // (PK_GPU_D3D12 != 0)

			if (!success) // Couldn't load any of the resources (CPU/GPU)
			{
				UE_LOG(LogPopcornFXAttributeSamplerImage, Warning, TEXT("AttrSamplerImage: couldn't load texture atlas '%s' for CPU/GPU sim sampling"), *ToUE(fullPath));
				return false;
			}
		}
		else
			m_Data->m_TextureAtlasResource.Clear();

		m_Data->m_ReloadTextureAtlas = false;
	}
	if (m_Data->m_Desc == null)
	{
		m_Data->m_Desc = PK_NEW(PopcornFX::CParticleSamplerDescriptor_Image_Default()); // Full override
		if (!PK_VERIFY(m_Data->m_Desc != null))
			return false;
	}

#if (PK_GPU_D3D12 != 0)
	if (rebuildImage || reloadImageAtlas) // We only want to reload bindings when m_Data->m_ReloadTexture is true (or if m_Data->m_ReloadTextureAtlas is true)
	{
		if (m_Data->m_TextureResource_D3D12 != null && !m_Data->m_TextureResource_D3D12->Empty()) // Don't bind the atlas by itself, clear everything if the texture is invalid
		{
			ID3D12Resource *imageResource = m_Data->m_TextureResource_D3D12->Texture();

			ID3D12Resource *imageAtlasResource = null;
			u32				atlasRectCount = 0;
			if (m_Data->m_TextureAtlasResource != null)
			{
				if (m_Data->m_TextureAtlasResource_D3D12 != null && !m_Data->m_TextureAtlasResource_D3D12->Empty())
				{
					imageAtlasResource = m_Data->m_TextureAtlasResource_D3D12->Buffer();
					atlasRectCount = m_Data->m_TextureAtlasResource->AtlasRectCount();
				}
			}
			const bool		sRGB = PopcornFX::CImage::IsFormatGammaCorrected(m_Data->m_TextureResource_D3D12->StorageFormat());

			PK_ASSERT(imageResource != null);

			m_Data->m_Desc->SetupD3D12Resources(imageResource,
				m_Data->m_TextureResource_D3D12->Dimensions(),
				UE::DXGIUtilities::FindShaderResourceFormat(imageResource->GetDesc().Format, sRGB),
				imageAtlasResource,
				atlasRectCount);
		}
		else
			m_Data->m_Desc->ClearD3D12Resources();
	}
#endif // (PK_GPU_D3D12 != 0)

	if (m_Data->m_TextureResource != null) // CPU image
	{
		PopcornFX::CImageSurface	dstSurface(m_Data->m_TextureResource->m_Frames[0].m_Mipmaps[0], m_Data->m_TextureResource->m_Format);

		const bool	both = Properties.SamplingMode == EPopcornFXImageSamplingMode::Both;
		if (Properties.SamplingMode == EPopcornFXImageSamplingMode::Density || both)
		{
			if (m_Data->m_DensityData == null)
			{
				m_Data->m_DensityData = PK_NEW(PopcornFX::SDensitySamplerData);
				if (!PK_VERIFY(m_Data->m_DensityData != null))
					return false;
			}
			if (!_BuildPDFs(dstSurface))
				return false;
			if (!PK_VERIFY(m_Data->m_Desc->SetupDensity(m_Data->m_DensityData)))
				return false;
			if (!both)
				m_Data->m_Desc->m_Sampler = null;
		}

		if (Properties.SamplingMode == EPopcornFXImageSamplingMode::Regular || both)
		{
			if (!_BuildRegularImage(dstSurface, rebuildImage))
				return false;
			m_Data->m_Desc->m_Sampler = m_Data->m_ImageSampler;
			if (!both && m_Data->m_DensityData != null)
			{
				m_Data->m_DensityData->Clear();
				m_Data->m_Desc->ClearDensity();
			}
		}
	}

	if (m_Data->m_TextureAtlasResource != null)
		m_Data->m_Desc->m_AtlasRectangleList = m_Data->m_TextureAtlasResource->m_RectsFp32;
	else
		m_Data->m_Desc->m_AtlasRectangleList.Clear();
	PK_ASSERT(m_Data->m_Desc->m_Sampler == m_Data->m_ImageSampler);
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerImage::_BuildPDFs(PopcornFX::CImageSurface &dstSurface)
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerImage::Build PDF", POPCORNFX_UE_PROFILER_COLOR);
	if (!m_Data->m_RebuildPDF)
		return true;
	m_Data->m_RebuildPDF = false;

	PK_STATIC_ASSERT(EPopcornFXImageDensitySource::Red == (u32)PopcornFX::SImageConvertSettings::LumMode_Red);
	PK_STATIC_ASSERT(EPopcornFXImageDensitySource::Green == (u32)PopcornFX::SImageConvertSettings::LumMode_Green);
	PK_STATIC_ASSERT(EPopcornFXImageDensitySource::Blue == (u32)PopcornFX::SImageConvertSettings::LumMode_Blue);
	PK_STATIC_ASSERT(EPopcornFXImageDensitySource::Alpha == (u32)PopcornFX::SImageConvertSettings::LumMode_Alpha);
	PK_STATIC_ASSERT(EPopcornFXImageDensitySource::RGBA_Average == (u32)PopcornFX::SImageConvertSettings::LumMode_RGBA_Avg);

	PopcornFX::SDensitySamplerBuildSettings	settings;
	settings.m_DensityPower = Properties.DensityPower;
	settings.m_RGBAToLumMode = static_cast<PopcornFX::SImageConvertSettings::ELumMode>(Properties.DensitySource.GetValue());
	//settings.m_SampleRawValues = ;
	if (m_Data->m_TextureAtlasResource != null)
		settings.m_AtlasRectangleList = m_Data->m_TextureAtlasResource->m_RectsFp32;
	m_Data->m_DensityData->Clear();
	if (!PK_VERIFY(m_Data->m_DensityData->Build(dstSurface, settings)))
		return false;
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplerImage::_BuildRegularImage(PopcornFX::CImageSurface &dstSurface, bool rebuild)
{
	PK_NAMEDSCOPEDPROFILE_C("UPopcornFXAttributeSamplerImage::Build Image", POPCORNFX_UE_PROFILER_COLOR);

	if ((m_Data->m_ImageSampler != null) &&
		!rebuild)
		return true;
	if (m_Data->m_ImageSampler != null)
		PK_SAFE_DELETE(m_Data->m_ImageSampler);

	m_Data->m_ImageSampler = PK_NEW(PopcornFX::CImageSamplerBilinear);
	if (!PK_VERIFY(m_Data->m_ImageSampler != null))
		return false;

	if (!/*PK_VERIFY*/(m_Data->m_ImageSampler->SetupFromSurface(dstSurface)))
	{
		if (Properties.bAllowTextureConversionAtRuntime)
		{
			const PopcornFX::CImage::EFormat		dstFormat = PopcornFX::CImage::Format_BGRA8;
			UE_LOG(LogPopcornFXAttributeSamplerImage, Log,
				TEXT("AttrSamplerImage: texture '%s' format %s not supported for sampling, converting to %s (because AllowTextureConvertionAtRuntime) in '%s'"),
				*Properties.Texture->GetName(),
				UTF8_TO_TCHAR(PopcornFX::CImage::GetFormatName(m_Data->m_TextureResource->m_Format)),
				UTF8_TO_TCHAR(PopcornFX::CImage::GetFormatName(dstFormat)),
				*GetPathName());

			PopcornFX::CImageSurface	newSurface;
			newSurface.m_Format = dstFormat;
			if (!newSurface.CopyAndConvertIFN(dstSurface))
			{
				UE_LOG(LogPopcornFXAttributeSamplerImage, Warning,
					TEXT("AttrSamplerImage: could not convert texture '%s' from %s to %s in %s"),
					*Properties.Texture->GetName(),
					UTF8_TO_TCHAR(PopcornFX::CImage::GetFormatName(m_Data->m_TextureResource->m_Format)),
					UTF8_TO_TCHAR(PopcornFX::CImage::GetFormatName(dstFormat)),
					*GetPathName());
				return false;
			}
			if (!PK_VERIFY(m_Data->m_ImageSampler->SetupFromSurface(newSurface)))
			{
				return false;
			}
		}
		else
		{
			UE_LOG(LogPopcornFXAttributeSamplerImage, Warning,
				TEXT("AttrSamplerImage: texture '%s' format %s not supported for sampling (and AllowTextureConvertionAtRuntime not enabled) in %s"),
				*Properties.Texture->GetName(),
				UTF8_TO_TCHAR(PopcornFX::CImage::GetFormatName(m_Data->m_TextureResource->m_Format)),
				*GetPathName());
			return false;
		}
	}
	return true;
}

#undef LOCTEXT_NAMESPACE
