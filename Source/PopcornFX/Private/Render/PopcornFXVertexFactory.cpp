//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXVertexFactory.h"

#include "PopcornFXVertexFactoryShaderParameters.h"

#include "Render/PopcornFXShaderUtils.h"

#include "MaterialShared.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"

#if (ENGINE_MINOR_VERSION >= 25)
#	include "MeshMaterialShader.h"
#endif // (ENGINE_MINOR_VERSION >= 25)

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FPopcornFXBillboardVSUniforms, "PopcornFXBillboardVSUniforms");
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FPopcornFXBillboardCommonUniforms, "PopcornFXBillboardCommonUniforms");

//----------------------------------------------------------------------------

void	FPopcornFXVertexFactory::_SetupStream(	u32								attributeIndex,
												FPopcornFXVertexBufferView		&buffer,
												EVertexElementType				type,
												FVertexDeclarationElementList	&decl)
{
	if (!buffer.Valid())
	{
		FVertexStreamComponent NullColorComponent(&GNullColorVertexBuffer, 0, 0, VET_Color);
		decl.Add(AccessStreamComponent(NullColorComponent, attributeIndex));
		return;
	}

	const u32			streamIndex = Streams.Num();
	FVertexStream		*stream = new (Streams) FVertexStream();
	stream->Offset = buffer.Offset();
	stream->Stride = buffer.Stride();
	stream->VertexBuffer = buffer.VertexBufferPtr();

	decl.Add(FVertexElement(streamIndex, 0, type, attributeIndex, buffer.Stride(), false));
}

//----------------------------------------------------------------------------
//
// FPopcornFXVertexFactory
//
//----------------------------------------------------------------------------

IMPLEMENT_VERTEX_FACTORY_TYPE(FPopcornFXVertexFactory, PKUE_SHADER_PATH("PopcornFXVertexFactory"), true, false, true, false, false);

#if (ENGINE_MINOR_VERSION >= 25)
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXVertexFactory, SF_Vertex, FPopcornFXVertexFactoryShaderParametersVertex);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FPopcornFXVertexFactory, SF_Pixel, FPopcornFXVertexFactoryShaderParametersPixel);
#endif // (ENGINE_MINOR_VERSION >= 25)

//----------------------------------------------------------------------------

FPopcornFXVertexFactory::FPopcornFXVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
:	FVertexFactory(InFeatureLevel)
{
	check(IsInRenderingThread());
}

//----------------------------------------------------------------------------

FPopcornFXVertexFactory::~FPopcornFXVertexFactory()
{
	check(IsInRenderingThread());

}

//----------------------------------------------------------------------------

#if (ENGINE_MINOR_VERSION >= 25)
void	FPopcornFXVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);
#else
void	FPopcornFXVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryType* Type, EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
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

#if (ENGINE_MINOR_VERSION >= 25)
bool	FPopcornFXVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	return (Parameters.MaterialParameters.bIsUsedWithParticleSprites || Parameters.MaterialParameters.bIsUsedWithNiagaraSprites || Parameters.MaterialParameters.bIsSpecialEngineMaterial) &&
			Parameters.MaterialParameters.MaterialDomain == EMaterialDomain::MD_Surface;
}
#else
bool	FPopcornFXVertexFactory::ShouldCompilePermutation(EShaderPlatform platform, const class FMaterial *material, const class FShaderType *shaderType)
{
	return	(material->IsUsedWithNiagaraSprites() || material->IsUsedWithParticleSprites() || material->IsSpecialEngineMaterial()) &&
			material->GetMaterialDomain() == EMaterialDomain::MD_Surface;
}
#endif // (ENGINE_MINOR_VERSION >= 25)

//----------------------------------------------------------------------------

bool	FPopcornFXVertexFactory::IsCompatible(UMaterialInterface *material)
{
	if (!material->CheckMaterialUsage_Concurrent(MATUSAGE_ParticleSprites) &&
		!material->CheckMaterialUsage_Concurrent(MATUSAGE_NiagaraSprites) &&
		!material->CheckMaterialUsage_Concurrent(MATUSAGE_NiagaraRibbons))
		return false;

	UMaterial	*mat = material->GetMaterial();
	if (mat != null)
		return PK_VERIFY(mat->MaterialDomain == MD_Surface); // If this triggers, it's a miss earlier (Render data factory ?)
	return true;
}

//----------------------------------------------------------------------------

void	FPopcornFXVertexFactory::InitRHI()
{
	FVertexDeclarationElementList	vDeclElements;
	Streams.Empty();

	_SetupStream(0, m_Positions, VET_Float3, vDeclElements);

	_SetupStream(1, m_Normals, VET_Float3, vDeclElements);
	_SetupStream(2, m_Tangents, VET_Float4, vDeclElements);

	PK_RELEASE_ASSERT(m_Texcoords.Stride() == 0 || m_Texcoords.Stride() == 8 || m_Texcoords.Stride() == 4);
	_SetupStream(3, m_Texcoords, m_Texcoords.Stride() == 4 ? VET_UShort2N : VET_Float2, vDeclElements);
	PK_RELEASE_ASSERT(m_Texcoord2s.Stride() == 0 || m_Texcoord2s.Stride() == 8 || m_Texcoord2s.Stride() == 4);
	_SetupStream(4, m_Texcoord2s, m_Texcoord2s.Stride() == 4 ? VET_UShort2N : VET_Float2, vDeclElements);

	_SetupStream(5, m_UVFactors, VET_Float4, vDeclElements);
	_SetupStream(6, m_UVScalesAndOffsets, VET_Float4, vDeclElements);
	_SetupStream(7, m_AtlasIDs, VET_Float1, vDeclElements); // This should be an additional input

	InitDeclaration(vDeclElements);
}

//----------------------------------------------------------------------------

#if (ENGINE_MINOR_VERSION < 25)
FVertexFactoryShaderParameters	*FPopcornFXVertexFactory::ConstructShaderParameters(EShaderFrequency shaderFrequency)
{
	// Create the corresponding shader parameters, depending on the shader type

	if (shaderFrequency == SF_Vertex)
	{
		return new FPopcornFXVertexFactoryShaderParametersVertex();
	}
	else if (shaderFrequency == SF_Pixel)
	{
		return new FPopcornFXVertexFactoryShaderParametersPixel();
	}
	return null;
}
#endif // (ENGINE_MINOR_VERSION < 25)
