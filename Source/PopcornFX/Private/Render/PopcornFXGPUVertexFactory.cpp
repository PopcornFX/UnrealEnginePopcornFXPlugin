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
	void	Bind(const FShaderParameterMap& ParameterMap)
	{
	}
};

//----------------------------------------------------------------------------
//	Vertex
//----------------------------------------------------------------------------

class	FPopcornFXGPUVertexFactoryShaderParametersVertex : public FPopcornFXGPUVertexFactoryShaderParametersBase
{
public:
	void					GetElementShaderBindings(	const FSceneInterface *scene,
														const FSceneView *view,
														const FMeshMaterialShader *shader,
														const EVertexInputStreamType inputStreamType,
														ERHIFeatureLevel::Type featureLevel,
														const FVertexFactory *vertexFactory,
														const FMeshBatchElement &batchElement,
														class FMeshDrawSingleShaderBindings &shaderBindings,
														FVertexInputStreamArray &vertexStreams) const
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
	void					GetElementShaderBindings(	const FSceneInterface *scene,
														const FSceneView *view,
														const FMeshMaterialShader *shader,
														const EVertexInputStreamType inputStreamType,
														ERHIFeatureLevel::Type featureLevel,
														const FVertexFactory *vertexFactory,
														const FMeshBatchElement &batchElement,
														class FMeshDrawSingleShaderBindings &shaderBindings,
														FVertexInputStreamArray &vertexStreams) const
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

#if (ENGINE_MAJOR_VERSION == 5)
IMPLEMENT_VERTEX_FACTORY_TYPE(FPopcornFXGPUVertexFactory, PKUE_SHADER_PATH("PopcornFXGPUVertexFactory"),
	EVertexFactoryFlags::UsedWithMaterials | EVertexFactoryFlags::SupportsDynamicLighting); // TODO
#else
IMPLEMENT_VERTEX_FACTORY_TYPE(FPopcornFXGPUVertexFactory, PKUE_SHADER_PATH("PopcornFXGPUVertexFactory"), true, false, true, false, false);
#endif // (ENGINE_MAJOR_VERSION == 5)

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXGPUVertexFactory, SF_Vertex, FPopcornFXGPUVertexFactoryShaderParametersVertex);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXGPUVertexFactory, SF_Compute, FPopcornFXGPUVertexFactoryShaderParametersVertex);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXGPUVertexFactory, SF_RayHitGroup, FPopcornFXGPUVertexFactoryShaderParametersVertex);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXGPUVertexFactory, SF_Pixel, FPopcornFXGPUVertexFactoryShaderParametersFragment);

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

void	FPopcornFXGPUVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);

	// @FIXME: it wont trigger recompilation !!
	OutEnvironment.SetDefine(TEXT("ENGINE_MINOR_VERSION"), ENGINE_MINOR_VERSION);
	OutEnvironment.SetDefine(TEXT("ENGINE_MAJOR_VERSION"), ENGINE_MAJOR_VERSION);

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

bool	FPopcornFXGPUVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	return	PlatformIsSupported(Parameters.Platform) &&
			(Parameters.MaterialParameters.bIsUsedWithParticleSprites || Parameters.MaterialParameters.bIsUsedWithNiagaraSprites || Parameters.MaterialParameters.bIsSpecialEngineMaterial) &&
			(Parameters.MaterialParameters.MaterialDomain == EMaterialDomain::MD_Surface || Parameters.MaterialParameters.MaterialDomain == EMaterialDomain::MD_Volume);
}

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
		FRHIResourceCreateInfo	info(TEXT("PopcornFX Texcoords buffer"));

		void	*data = null;
#if (ENGINE_MAJOR_VERSION == 5)
		VertexBufferRHI = RHICreateBuffer(sizeInBytes, BUF_Static | BUF_VertexBuffer, sizeof(CFloat2), ERHIAccess::VertexOrIndexBuffer, info);
		data = RHILockBuffer(VertexBufferRHI, 0, sizeInBytes, RLM_WriteOnly);
#else
		VertexBufferRHI = RHICreateAndLockVertexBuffer(sizeInBytes, BUF_Static, info, data);
#endif // (ENGINE_MAJOR_VERSION == 5)

		CFloat2	*vertices = (CFloat2*)data;

		vertices[0] = CFloat2(-1.0f, -1.0f); // Lower left corner
		vertices[1] = CFloat2(-1.0f, 1.0f); // Upper left corner
		vertices[2] = CFloat2(1.0f, 1.0f); // Upper right corner
		vertices[3] = CFloat2(1.0f, -1.0f); // Lower right corner
		vertices[4] = CFloat2(0.0f, 2.0f); // Capsule up
		vertices[5] = CFloat2(0.0f, -2.0f); // Capsule down

#if (ENGINE_MAJOR_VERSION == 5)
		RHIUnlockBuffer(VertexBufferRHI);
#else
		RHIUnlockVertexBuffer(VertexBufferRHI);
#endif // (ENGINE_MAJOR_VERSION == 5)
	}
};

TGlobalResource<FPopcornFXGPUParticlesTexCoordVertexBuffer>	GPopcornFXGPUParticlesTexCoordVertexBuffer;

//----------------------------------------------------------------------------

void	FPopcornFXGPUVertexFactory::InitRHI()
{
	const bool	bInstanced = true; // Supported everyone since 4.25

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
