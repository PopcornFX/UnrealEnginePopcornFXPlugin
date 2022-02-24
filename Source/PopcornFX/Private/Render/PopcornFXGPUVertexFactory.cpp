//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXGPUVertexFactory.h"

#include "PopcornFXVertexFactoryShaderParameters.h"

#include "Render/PopcornFXShaderUtils.h"
#include "MeshMaterialShader.h"
#include "ParticleResources.h"
#include "PipelineStateCache.h"

#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FPopcornFXGPUBillboardVSUniforms, "PopcornFXVSUniforms");

//----------------------------------------------------------------------------
//	Base
//----------------------------------------------------------------------------

class	FPopcornFXGPUVertexFactoryShaderParametersBase : public FVertexFactoryShaderParameters
{
public:
#if (ENGINE_MINOR_VERSION >= 25)
	void	Bind(const FShaderParameterMap& ParameterMap)
	{
	}
#else
	virtual void	Bind(const FShaderParameterMap& ParameterMap) override
	{
	}

	virtual void	Serialize(FArchive& Ar) override
	{
	}
#endif // (ENGINE_MINOR_VERSION >= 25)
};

//----------------------------------------------------------------------------
//	Vertex
//----------------------------------------------------------------------------

class	FPopcornFXGPUVertexFactoryShaderParametersVertex : public FPopcornFXGPUVertexFactoryShaderParametersBase
{
public:
#if (ENGINE_MINOR_VERSION < 25)
virtual
#endif // (ENGINE_MINOR_VERSION < 25)
	void					GetElementShaderBindings(	const FSceneInterface *scene,
														const FSceneView *view,
														const FMeshMaterialShader *shader,
														const EVertexInputStreamType inputStreamType,
														ERHIFeatureLevel::Type featureLevel,
														const FVertexFactory *vertexFactory,
														const FMeshBatchElement &batchElement,
														class FMeshDrawSingleShaderBindings &shaderBindings,
														FVertexInputStreamArray &vertexStreams) const
#if (ENGINE_MINOR_VERSION < 25)
														override
#endif // (ENGINE_MINOR_VERSION < 25)
	{
		FPopcornFXGPUVertexFactory	*_vertexFactory = (FPopcornFXGPUVertexFactory*)vertexFactory;
		shaderBindings.Add(shader->GetUniformBufferParameter<FPopcornFXGPUBillboardVSUniforms>(), _vertexFactory->GetVSUniformBuffer());
	}
};

//----------------------------------------------------------------------------
//	Fragment
//----------------------------------------------------------------------------

class	FPopcornFXGPUVertexFactoryShaderParametersFragment : public FPopcornFXGPUVertexFactoryShaderParametersBase
{
public:
#if (ENGINE_MINOR_VERSION < 25)
	virtual
#	endif // (ENGINE_MINOR_VERSION < 25)
	void					GetElementShaderBindings(	const FSceneInterface *scene,
														const FSceneView *view,
														const FMeshMaterialShader *shader,
														const EVertexInputStreamType inputStreamType,
														ERHIFeatureLevel::Type featureLevel,
														const FVertexFactory *vertexFactory,
														const FMeshBatchElement &batchElement,
														class FMeshDrawSingleShaderBindings &shaderBindings,
														FVertexInputStreamArray &vertexStreams) const
#if (ENGINE_MINOR_VERSION < 25)
	override
#endif // (ENGINE_MINOR_VERSION < 25)
	{
		FPopcornFXGPUVertexFactory	*_vertexFactory = (FPopcornFXGPUVertexFactory*)vertexFactory;
		shaderBindings.Add(shader->GetUniformBufferParameter<FPopcornFXGPUBillboardVSUniforms>(), _vertexFactory->GetVSUniformBuffer());
	}
};

//----------------------------------------------------------------------------
//
// FPopcornFXGPUVertexFactory
//
//----------------------------------------------------------------------------

IMPLEMENT_VERTEX_FACTORY_TYPE(FPopcornFXGPUVertexFactory, PKUE_SHADER_PATH("PopcornFXGPUVertexFactory"), true, false, true, false, false);

#if (ENGINE_MINOR_VERSION >= 25)
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXGPUVertexFactory, SF_Vertex, FPopcornFXGPUVertexFactoryShaderParametersVertex);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXGPUVertexFactory, SF_Compute, FPopcornFXGPUVertexFactoryShaderParametersVertex);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXGPUVertexFactory, SF_RayHitGroup, FPopcornFXGPUVertexFactoryShaderParametersVertex);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXGPUVertexFactory, SF_Pixel, FPopcornFXGPUVertexFactoryShaderParametersFragment);
#endif // (ENGINE_MINOR_VERSION >= 25)

//----------------------------------------------------------------------------

FPopcornFXGPUVertexFactory::FPopcornFXGPUVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
:	FVertexFactory(InFeatureLevel)
{
	check(IsInRenderingThread());
}

//----------------------------------------------------------------------------

FPopcornFXGPUVertexFactory::~FPopcornFXGPUVertexFactory()
{
	check(IsInRenderingThread());

}

//----------------------------------------------------------------------------

#if (ENGINE_MINOR_VERSION >= 25)
void	FPopcornFXGPUVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);
#else
void	FPopcornFXGPUVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryType* Type, EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
{
	FVertexFactory::ModifyCompilationEnvironment(Type, Platform, Material, OutEnvironment);
#endif // (ENGINE_MINOR_VERSION >= 25)

	// @FIXME: it wont trigger recompilation !!
	OutEnvironment.SetDefine(TEXT("ENGINE_MINOR_VERSION"), ENGINE_MINOR_VERSION);

	// 4.9: Needed to have DynamicParameters
	OutEnvironment.SetDefine(TEXT("PARTICLE_FACTORY"),TEXT("1"));

	// Dynamic parameters..
	OutEnvironment.SetDefine(TEXT("NIAGARA_PARTICLE_FACTORY"), TEXT("1"));
}

//----------------------------------------------------------------------------

bool	FPopcornFXGPUVertexFactory::PlatformIsSupported(EShaderPlatform platform)
{
	const bool	platformSupportsByteAddressBuffers = IsFeatureLevelSupported(platform, ERHIFeatureLevel::SM5) ||
													IsFeatureLevelSupported(platform, ERHIFeatureLevel::ES3_1);

	return platformSupportsByteAddressBuffers;
}

//----------------------------------------------------------------------------

#if (ENGINE_MINOR_VERSION >= 25)
bool	FPopcornFXGPUVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	return	PlatformIsSupported(Parameters.Platform) &&
			(Parameters.MaterialParameters.bIsUsedWithParticleSprites || Parameters.MaterialParameters.bIsUsedWithNiagaraSprites || Parameters.MaterialParameters.bIsSpecialEngineMaterial) &&
			(Parameters.MaterialParameters.MaterialDomain == EMaterialDomain::MD_Surface || Parameters.MaterialParameters.MaterialDomain == EMaterialDomain::MD_Volume);
}
#else
bool	FPopcornFXGPUVertexFactory::ShouldCompilePermutation(EShaderPlatform platform, const class FMaterial *material, const class FShaderType *shaderType)
{
	return	PlatformIsSupported(platform) &&
			(material->IsUsedWithNiagaraSprites() || material->IsUsedWithParticleSprites() || material->IsSpecialEngineMaterial()) &&
			(material->GetMaterialDomain() == EMaterialDomain::MD_Surface || material->GetMaterialDomain() == EMaterialDomain::MD_Volume);
}
#endif // (ENGINE_MINOR_VERSION >= 25)

//----------------------------------------------------------------------------

bool	FPopcornFXGPUVertexFactory::IsCompatible(UMaterialInterface *material)
{
	if (!material->CheckMaterialUsage_Concurrent(MATUSAGE_ParticleSprites) &&
		!material->CheckMaterialUsage_Concurrent(MATUSAGE_NiagaraSprites))
		return false;

	UMaterial	*mat = material->GetMaterial();
	if (mat != null)
	{
		return PK_VERIFY(mat->MaterialDomain == MD_Surface ||
						 mat->MaterialDomain == MD_Volume); // If this triggers, it's a miss earlier (Render data factory ?)
	}
	return true;
}

//----------------------------------------------------------------------------

class	FPopcornFXVertexDeclaration : public FRenderResource
{
public:
	FPopcornFXVertexDeclaration() {}
	virtual		~FPopcornFXVertexDeclaration() {}

	virtual void	InitDynamicRHI() override
	{
		FVertexDeclarationElementList	vDecl;

		vDecl.Add(FVertexElement(0, 0, VET_Float2, 0, sizeof(CFloat2), false));
		VertexDeclarationRHI = PipelineStateCache::GetOrCreateVertexDeclaration(vDecl);
	}

	virtual void	ReleaseDynamicRHI() override
	{
		VertexDeclarationRHI.SafeRelease();
	}

	FVertexDeclarationRHIRef	VertexDeclarationRHI;
};

/** The simple element vertex declaration. */
static TGlobalResource<FPopcornFXVertexDeclaration>		GPopcornFXBillboardsParticleDeclaration;

//----------------------------------------------------------------------------

class	FPopcornFXGPUParticlesTexCoordVertexBuffer : public FVertexBuffer
{
public:
	virtual void	InitRHI() override
	{
		const u32				sizeInBytes = sizeof(CFloat2) * 6;
		FRHIResourceCreateInfo	info;

		void	*data = null;
		VertexBufferRHI = RHICreateAndLockVertexBuffer(sizeInBytes, BUF_Static, info, data);

		CFloat2	*vertices = (CFloat2*)data;

		vertices[0] = CFloat2(-1.0f, -1.0f); // Lower left corner
		vertices[1] = CFloat2(-1.0f, 1.0f); // Upper left corner
		vertices[2] = CFloat2(1.0f, 1.0f); // Upper right corner
		vertices[3] = CFloat2(1.0f, -1.0f); // Lower right corner
		vertices[4] = CFloat2(0.0f, 2.0f); // Capsule up
		vertices[5] = CFloat2(0.0f, -2.0f); // Capsule down

		RHIUnlockVertexBuffer(VertexBufferRHI);
	}
};

TGlobalResource<FPopcornFXGPUParticlesTexCoordVertexBuffer>	GPopcornFXGPUParticlesTexCoordVertexBuffer;

//----------------------------------------------------------------------------

void	FPopcornFXGPUVertexFactory::InitRHI()
{
#if (ENGINE_MINOR_VERSION >= 25)
	const bool	bInstanced = true; // Supported everyone since 4.25
#else
	const bool	bInstanced = GRHISupportsInstancing;
#endif // (ENGINE_MINOR_VERSION >= 25)

	Streams.Empty();
	if (bInstanced)
	{
		FVertexStream	*texCoordStream = new(Streams) FVertexStream;
		texCoordStream->VertexBuffer = &GPopcornFXGPUParticlesTexCoordVertexBuffer;
		texCoordStream->Stride = sizeof(CFloat2);
		texCoordStream->Offset = 0;
	}
	SetDeclaration(GPopcornFXBillboardsParticleDeclaration.VertexDeclarationRHI);
}

//----------------------------------------------------------------------------

#if (ENGINE_MINOR_VERSION < 25)
FVertexFactoryShaderParameters	*FPopcornFXGPUVertexFactory::ConstructShaderParameters(EShaderFrequency shaderFrequency)
{
	// Create the corresponding shader parameters, depending on the shader type

	if (shaderFrequency == SF_Vertex)
	{
		return new FPopcornFXGPUVertexFactoryShaderParametersVertex();
	}
	else if (shaderFrequency == SF_Pixel)
	{
		return new FPopcornFXGPUVertexFactoryShaderParametersFragment();
	}
#if RHI_RAYTRACING
	else if (shaderFrequency == SF_Compute)
	{
		return new FPopcornFXGPUVertexFactoryShaderParametersVertex();
	}
	else if (shaderFrequency == SF_RayHitGroup)
	{
		return new FPopcornFXGPUVertexFactoryShaderParametersVertex();
	}
#endif // (RHI_RAYTRACING)
	return null;
}
#endif //(ENGINE_MINOR_VERSION < 25)
