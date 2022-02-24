//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "PopcornFXSDK.h"
#if (PK_HAS_GPU != 0)
#	include "Render/PopcornFXBuffer.h"
#	include "GPUSim/PopcornFXGPUSim.h"
#	include "Render/RenderTypesPolicies.h"
#	include <pk_particles/include/Renderers/ps_renderer_enums.h>
#	include "GlobalShader.h"
#	include "ShaderParameterUtils.h"
#endif

//----------------------------------------------------------------------------

#define PK_BILLBOARDER_CS_OUTPUT_PACK_PTN				1
#define PK_BILLBOARDER_CS_OUTPUT_PACK_COLOR_F16			1
#define PK_BILLBOARDER_CS_OUTPUT_PACK_TEXCOORD			1

#define	PK_BILLBOARDER_CS_ASSERT_OUTPUT_STRIDE(__output, __stride)		do {																												\
		if (__output == PopcornFXBillboarder::EOutput::OutPositions || __output == PopcornFXBillboarder::EOutput::OutNormals || __output == PopcornFXBillboarder::EOutput::OutTangents)		\
			PK_ASSERT(__stride == (PK_BILLBOARDER_CS_OUTPUT_PACK_PTN ? 12 : 16));																											\
		else if (__output == PopcornFXBillboarder::EOutput::OutColors)																														\
			PK_ASSERT(__stride == (PK_BILLBOARDER_CS_OUTPUT_PACK_COLOR_F16 ? 8 : 16));																										\
		else if (__output == PopcornFXBillboarder::EOutput::OutTexcoords || __output == PopcornFXBillboarder::EOutput::OutTexcoord2s)														\
			PK_ASSERT(__stride == (PK_BILLBOARDER_CS_OUTPUT_PACK_TEXCOORD ? 4 : 8));																										\
	} while (0)

//----------------------------------------------------------------------------

#if (PK_HAS_GPU != 0)

//----------------------------------------------------------------------------
namespace PopcornFXBillboarder
{

#define EXEC_X_PK_BILLBOARDER_TYPE()			\
	X_PK_BILLBOARDER_TYPE(ScreenAligned)		\
	X_PK_BILLBOARDER_TYPE(ViewposAligned)		\
	X_PK_BILLBOARDER_TYPE(AxisAligned)			\
	X_PK_BILLBOARDER_TYPE(AxisAlignedSpheroid)	\
	X_PK_BILLBOARDER_TYPE(AxisAlignedCapsule)	\
	X_PK_BILLBOARDER_TYPE(PlaneAligned)

	namespace EBillboarder
	{
		enum Type
		{
#define X_PK_BILLBOARDER_TYPE(__type)		__type,
			EXEC_X_PK_BILLBOARDER_TYPE()
#undef X_PK_BILLBOARDER_TYPE
		};
#define X_PK_BILLBOARDER_TYPE(__type)		1 +
		enum { _Count = EXEC_X_PK_BILLBOARDER_TYPE() + 0 };
#undef X_PK_BILLBOARDER_TYPE
	}

	//----------------------------------------------------------------------------

	EBillboarder::Type		BillboarderModeToType(PopcornFX::EBillboardMode mode);

	//----------------------------------------------------------------------------

#define EXEC_X_PK_BILLBOARDER_INPUT()					\
		X_PK_BILLBOARDER_INPUT(InIndices)				\
		X_PK_BILLBOARDER_INPUT(InPositions)				\
		X_PK_BILLBOARDER_INPUT(InColors)				\
		X_PK_BILLBOARDER_INPUT(InSizes)					\
		X_PK_BILLBOARDER_INPUT(In2Sizes)				\
		X_PK_BILLBOARDER_INPUT(InRotations)				\
		X_PK_BILLBOARDER_INPUT(InAxis0s)				\
		X_PK_BILLBOARDER_INPUT(InAxis1s)				\
		X_PK_BILLBOARDER_INPUT(InTextureIds)			\
		X_PK_BILLBOARDER_INPUT(InAlphaCursors)			\
		X_PK_BILLBOARDER_INPUT(InOrientations)			\
		X_PK_BILLBOARDER_INPUT(InScales)				\
		X_PK_BILLBOARDER_INPUT(InVATCursors)			\
		X_PK_BILLBOARDER_INPUT(InDynamicParameter0s)	\
		X_PK_BILLBOARDER_INPUT(InDynamicParameter1s)	\
		X_PK_BILLBOARDER_INPUT(InDynamicParameter2s)	\
		X_PK_BILLBOARDER_INPUT(InDynamicParameter3s)

	namespace EInput
	{
		enum Type
		{
#define X_PK_BILLBOARDER_INPUT(__input) __input,
			EXEC_X_PK_BILLBOARDER_INPUT()
#undef X_PK_BILLBOARDER_INPUT
		};
#define X_PK_BILLBOARDER_INPUT(__type)		1 +
		enum { _Count = EXEC_X_PK_BILLBOARDER_INPUT() 0 };
#undef X_PK_BILLBOARDER_INPUT
	}

	u32		BillboarderTypeMaskMustHaveInputMask(u32 typeMask);

	//----------------------------------------------------------------------------

#define EXEC_X_PK_BILLBOARDER_OUTPUT()					\
		X_PK_BILLBOARDER_OUTPUT(OutIndicesRaw)			\
		X_PK_BILLBOARDER_OUTPUT(OutIndices)				\
		X_PK_BILLBOARDER_OUTPUT(OutPositions)			\
		X_PK_BILLBOARDER_OUTPUT(OutColors)				\
		X_PK_BILLBOARDER_OUTPUT(OutTexcoords)			\
		X_PK_BILLBOARDER_OUTPUT(OutTexcoord2s)			\
		X_PK_BILLBOARDER_OUTPUT(OutNormals)				\
		X_PK_BILLBOARDER_OUTPUT(OutTangents)			\
		X_PK_BILLBOARDER_OUTPUT(OutAtlasIDs)			\
		X_PK_BILLBOARDER_OUTPUT(OutAlphaCursors)		\
		X_PK_BILLBOARDER_OUTPUT(OutMatrices)			\
		X_PK_BILLBOARDER_OUTPUT(OutVATCursors)			\
		X_PK_BILLBOARDER_OUTPUT(OutDynamicParameter0s)	\
		X_PK_BILLBOARDER_OUTPUT(OutDynamicParameter1s)	\
		X_PK_BILLBOARDER_OUTPUT(OutDynamicParameter2s)	\
		X_PK_BILLBOARDER_OUTPUT(OutDynamicParameter3s)

	namespace EOutput
	{
		enum Type
		{
#define X_PK_BILLBOARDER_OUTPUT(__output) __output,
			EXEC_X_PK_BILLBOARDER_OUTPUT()
#undef X_PK_BILLBOARDER_OUTPUT
		};
#define X_PK_BILLBOARDER_OUTPUT(__type)		1 +
		enum { _Count = EXEC_X_PK_BILLBOARDER_OUTPUT() 0 };
#undef X_PK_BILLBOARDER_OUTPUT
	}

	//----------------------------------------------------------------------------

	namespace ERendererFlag
	{
		enum
		{
			FlipV,
			SoftAnimationBlending,
		};
	}

	//----------------------------------------------------------------------------

	void		AddDefinesEnumTypeAndMasks(FShaderCompilerEnvironment& OutEnvironment);

	//----------------------------------------------------------------------------

#if (PK_GPU_D3D11 != 0) || (PK_GPU_D3D12 != 0)
	struct	FClearCS_Params
	{
		FUnorderedAccessViewRHIRef	m_UAVs[EOutput::_Count];

		void	SetUAV(EOutput::Type output, std::nullptr_t _null)
		{
			m_UAVs[output] = null;
		}

		void	SetUAV(EOutput::Type output, CPooledVertexBuffer &buffer)
		{
			if (buffer.Valid())
			{
				PK_BILLBOARDER_CS_ASSERT_OUTPUT_STRIDE(output, buffer->AllocatedStride());
				m_UAVs[output] = buffer->UAV();
			}
			else
				m_UAVs[output] = null;
		}

		void	SetUAV(EOutput::Type output, CPooledIndexBuffer &buffer)
		{
			if (buffer.Valid())
			{
				PK_BILLBOARDER_CS_ASSERT_OUTPUT_STRIDE(output, buffer->AllocatedStride());
				m_UAVs[output] = buffer->UAV();
			}
			else
				m_UAVs[output] = null;
		}
	};
#endif // (PK_GPU_D3D11 != 0) || (PK_GPU_D3D12 != 0)

	struct	FCS_Params
	{
		uint32			m_ParticleCount = 0;
		uint32			m_InputOffset = 0;
		uint32			m_OutputVertexOffset = 0;

		FShaderResourceViewRHIRef			m_LiveParticleCount;
		FShaderResourceViewRHIRef			m_InIndices;
		FShaderResourceViewRHIRef			m_InSimData;

		FUnorderedAccessViewRHIRef			m_Outputs[EOutput::_Count];
		bool								m_ValidInputs[EInput::_Count];
		uint32								m_InputsOffsets[EInput::_Count];
		CFloat4								m_InputsDefault[EInput::_Count];

		FCS_Params()
		{
			PopcornFX::Mem::Clear(m_InputsOffsets);
			PopcornFX::Mem::Clear(m_InputsDefault);
		}

		void	SetOutput(EOutput::Type output, std::nullptr_t _null)
		{
			m_Outputs[output] = null;
		}

		void	SetOutput(EOutput::Type output, bool condition, std::nullptr_t _null)
		{
			PK_ASSERT(!condition);
			m_Outputs[output] = null;
		}

		void	SetOutput(EOutput::Type output, bool condition, CPooledVertexBuffer &buffer)
		{
			if (condition && PK_VERIFY(buffer.Valid()))
			{
				PK_BILLBOARDER_CS_ASSERT_OUTPUT_STRIDE(output, buffer->AllocatedStride());
				m_Outputs[output] = buffer->UAV();
			}
			else
				m_Outputs[output] = null;
		}

		void	SetOutput(EOutput::Type output, bool condition, CPooledIndexBuffer &buffer)
		{
			if (condition && PK_VERIFY(buffer.Valid()))
			{
				PK_BILLBOARDER_CS_ASSERT_OUTPUT_STRIDE(output, buffer->AllocatedStride());
				m_Outputs[output] = buffer->UAV();
			}
			else
				m_Outputs[output] = null;
		}

		void	SetOutput(EOutput::Type output, CPooledVertexBuffer &buffer)
		{
			if (buffer.Valid())
			{
				PK_BILLBOARDER_CS_ASSERT_OUTPUT_STRIDE(output, buffer->AllocatedStride());
				m_Outputs[output] = buffer->UAV();
			}
			else
				m_Outputs[output] = null;
		}

		void	SetOutput(EOutput::Type output, CPooledIndexBuffer &buffer)
		{
			if (buffer.Valid())
			{
				PK_BILLBOARDER_CS_ASSERT_OUTPUT_STRIDE(output, buffer->AllocatedStride());
				m_Outputs[output] = buffer->UAV();
			}
			else
				m_Outputs[output] = null;
		}

		void	SetInput(EInput::Type input, bool valid, uint32 inputOffset, const CFloat4 &defaultValue = CFloat4(0.f))
		{
			m_ValidInputs[input] = valid;
			m_InputsOffsets[input] = inputOffset;
			m_InputsDefault[input] = defaultValue;
		}

		void	SetInput(EInput::Type input, std::nullptr_t _null, uint32 inputOffset, const CFloat4 &defaultValue = CFloat4(0.f))
		{
			m_ValidInputs[input] = false;
			m_InputsOffsets[input] = inputOffset;
			m_InputsDefault[input] = defaultValue;
		}

		template <typename _Type, u32 _Stride>
		bool		SetInputOffsetIFP(
			EInput::Type input,
			const PopcornFX::CParticleStreamToRender_GPU &stream_GPU,
			PopcornFX::CGuid streamId,
			const CFloat4 &defaultValue)
		{
			m_InputsDefault[input] = defaultValue;
			if (streamId.Valid())
			{
				m_ValidInputs[input] = StreamBufferByteOffset(stream_GPU, streamId, m_InputsOffsets[input]);
				return true;
			}
			else
			{
				m_ValidInputs[input] = false;
				m_InputsOffsets[input] = 0;
			}
			return false;
		}
	};

	struct	FCSCopySizeBuffer_Params
	{
		FShaderResourceViewRHIRef	m_LiveParticleCount;
		FUnorderedAccessViewRHIRef	m_DrawIndirectArgsBuffer;
		uint32						m_DrawIndirectArgsOffset = 0;
		bool						m_IsCapsule;
	};

#if (PK_GPU_D3D11 != 0) || (PK_GPU_D3D12 != 0)
	//----------------------------------------------------------------------------
	//
	//	Generic UAV Clear compute shader
	//
	//----------------------------------------------------------------------------

	class	FUAVsClearCS : public FGlobalShader
	{
		DECLARE_SHADER_TYPE(FUAVsClearCS, Global);

	public:
		typedef FGlobalShader	Super;

		static bool			ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters);
		static void			ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment);

		FUAVsClearCS(const ShaderMetaType::CompiledShaderInitializerType &Initializer);
		FUAVsClearCS();

		void			Dispatch(const SRenderContext &renderContext, FRHICommandList &RHICmdList, const FClearCS_Params &params);
#if (ENGINE_MINOR_VERSION < 25)
		virtual bool	Serialize(FArchive &Ar);
#endif // (ENGINE_MINOR_VERSION < 25)
	public:
#if (ENGINE_MINOR_VERSION >= 25)
		LAYOUT_FIELD(FShaderParameter, ClearBufferCSParams);
		LAYOUT_FIELD(FShaderResourceParameter, UAVRaw);
		LAYOUT_FIELD(FShaderResourceParameter, UAV);
		LAYOUT_FIELD(FShaderParameter, RawUAV);
#else
		FShaderParameter			ClearBufferCSParams;
		FShaderResourceParameter	UAVRaw;
		FShaderResourceParameter	UAV;
		FShaderParameter			RawUAV;
#endif // (ENGINE_MINOR_VERSION >= 25)
	};
#endif // (PK_GPU_D3D11 != 0) || (PK_GPU_D3D12 != 0)

	//----------------------------------------------------------------------------
	//
	//	Billboard renderer billboarder compute shader
	//
	//----------------------------------------------------------------------------

	namespace EBillboarderCSBuild
	{
		enum Type
		{
			Std,
			VertexPP,
		};
	}

	struct	FBillboardCS_Params : public FCS_Params
	{
		uint32	m_BillboarderType = 0;
	};

	struct	FBillboarderBillboardCS_Params : public FBillboardCS_Params
	{
		uint32			m_InIndicesOffset = 0;
		uint32			m_OutputIndexOffset = 0;

		CFloat4x4		m_BillboardingMatrix;

		u32				m_RendererFlags = 0;
		float			m_RendererNormalsBendingFactor = 0.f;

		u32									m_RendererAtlasRectCount = 0;
		FShaderResourceViewRHIRef			m_RendererAtlasBuffer;

		FBillboarderBillboardCS_Params()
		{
			m_BillboardingMatrix = CFloat4x4::IDENTITY;
		}
	};

	template <EBillboarderCSBuild::Type _Build>
	class	FBillboarderBillboardCS : public FGlobalShader
	{
		DECLARE_SHADER_TYPE(FBillboarderBillboardCS, Global)

	public:
		typedef FGlobalShader	Super;

		static bool			ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters);
		static void			ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment);

		FBillboarderBillboardCS(const ShaderMetaType::CompiledShaderInitializerType &Initializer);
		FBillboarderBillboardCS();

		void			Dispatch(FRHICommandList &RHICmdList, const FBillboarderBillboardCS_Params &params);
#if (ENGINE_MINOR_VERSION < 25)
		virtual bool	Serialize(FArchive &Ar);
#endif // (ENGINE_MINOR_VERSION < 25)

	public:
#if (ENGINE_MINOR_VERSION >= 25)
		LAYOUT_FIELD(FShaderParameter, BillboarderType);
		LAYOUT_FIELD(FShaderParameter, OutputMask);
		LAYOUT_FIELD(FShaderParameter, InputMask);
		LAYOUT_FIELD(FShaderParameter, InIndicesOffset);
		LAYOUT_FIELD(FShaderParameter, InputOffset);
		LAYOUT_FIELD(FShaderParameter, OutputVertexOffset);
		LAYOUT_FIELD(FShaderParameter, OutputIndexOffset);
		LAYOUT_FIELD(FShaderParameter, BillboardingMatrix);
		LAYOUT_FIELD(FShaderParameter, RendererFlags);
		LAYOUT_FIELD(FShaderParameter, RendererNormalsBendingFactor);
		LAYOUT_FIELD(FShaderParameter, RendererAtlasRectCount);
		LAYOUT_FIELD(FShaderResourceParameter, RendererAtlasBuffer);
		LAYOUT_ARRAY(FShaderResourceParameter, Outputs, EOutput::_Count);
		LAYOUT_FIELD(FShaderResourceParameter, InIndices);
		LAYOUT_FIELD(FShaderResourceParameter, InSimData);
		LAYOUT_ARRAY(FShaderParameter, InputsOffsets, EInput::_Count);
		LAYOUT_ARRAY(FShaderParameter, InputsDefault, EInput::_Count);
		LAYOUT_FIELD(FShaderParameter, HasLiveParticleCount);
		LAYOUT_FIELD(FShaderResourceParameter, LiveParticleCount);
#else
		FShaderParameter				BillboarderType;
		FShaderParameter				OutputMask;
		FShaderParameter				InputMask;
		FShaderParameter				InIndicesOffset;
		FShaderParameter				InputOffset;
		FShaderParameter				OutputVertexOffset;
		FShaderParameter				OutputIndexOffset;
		FShaderParameter				BillboardingMatrix;
		FShaderParameter				RendererFlags;
		FShaderParameter				RendererNormalsBendingFactor;
		FShaderParameter				RendererAtlasRectCount;
		FShaderResourceParameter		RendererAtlasBuffer;
		FShaderResourceParameter		Outputs[EOutput::_Count];
		FShaderResourceParameter		InIndices;
		FShaderResourceParameter		InSimData;
		FShaderParameter				InputsOffsets[EInput::_Count];
		FShaderParameter				InputsDefault[EInput::_Count];
		FShaderParameter				HasLiveParticleCount;
		FShaderResourceParameter		LiveParticleCount;
#endif // (ENGINE_MINOR_VERSION >= 25)
	};

	extern template class FBillboarderBillboardCS<EBillboarderCSBuild::Std>;
	extern template class FBillboarderBillboardCS<EBillboarderCSBuild::VertexPP>;

	typedef FBillboarderBillboardCS<EBillboarderCSBuild::Std>			FBillboarderBillboardCS_Std;
	typedef FBillboarderBillboardCS<EBillboarderCSBuild::VertexPP>		FBillboarderBillboardCS_VPP;

	//----------------------------------------------------------------------------
	//
	//	Billboard renderer copy streams compute shader
	//
	//----------------------------------------------------------------------------

	class	FCopyStreamCS : public FGlobalShader
	{
		DECLARE_SHADER_TYPE(FCopyStreamCS, Global);

	public:
		typedef FGlobalShader	Super;

		static bool			ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters);
		static void			ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment);

		FCopyStreamCS(const ShaderMetaType::CompiledShaderInitializerType &Initializer);
		FCopyStreamCS();

		void			Dispatch(FRHICommandList &RHICmdList, const FBillboardCS_Params &params);
#if (ENGINE_MINOR_VERSION < 25)
		virtual bool	Serialize(FArchive &Ar);
#endif // (ENGINE_MINOR_VERSION < 25)

	public:
#if (ENGINE_MINOR_VERSION >= 25)
		LAYOUT_FIELD(FShaderParameter, OutputMask);
		LAYOUT_FIELD(FShaderParameter, InputMask);
		LAYOUT_FIELD(FShaderParameter, InputOffset);
		LAYOUT_FIELD(FShaderParameter, OutputVertexOffset);
		LAYOUT_FIELD(FShaderParameter, IsCapsule);
		LAYOUT_ARRAY(FShaderResourceParameter, Outputs, EOutput::_Count);
		LAYOUT_FIELD(FShaderResourceParameter, InSimData);
		LAYOUT_ARRAY(FShaderParameter, InputsOffsets, EInput::_Count);
		LAYOUT_ARRAY(FShaderParameter, InputsDefault, EInput::_Count);
		LAYOUT_FIELD(FShaderParameter, HasLiveParticleCount);
		LAYOUT_FIELD(FShaderResourceParameter, LiveParticleCount);
#else
		FShaderParameter				OutputMask;
		FShaderParameter				InputMask;
		FShaderParameter				InputOffset;
		FShaderParameter				OutputVertexOffset;
		FShaderParameter				IsCapsule;
		FShaderResourceParameter		Outputs[EOutput::_Count];
		FShaderResourceParameter		InSimData;
		FShaderParameter				InputsOffsets[EInput::_Count];
		FShaderParameter				InputsDefault[EInput::_Count];
		FShaderParameter				HasLiveParticleCount;
		FShaderResourceParameter		LiveParticleCount;
#endif // (ENGINE_MINOR_VERSION >= 25)
	};

	//----------------------------------------------------------------------------
	//
	//	Mesh renderer billboarding compute shader (Build transforms)
	//
	//----------------------------------------------------------------------------

	struct	FMeshCS_Params : public FCS_Params
	{
		float	m_PositionsScale = 1.0f;
	};

	class	FBillboarderMeshCS : public FGlobalShader
	{
		DECLARE_SHADER_TYPE(FBillboarderMeshCS, Global);

	public:
		typedef FGlobalShader	Super;

		static bool			ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters);
		static void			ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment);

		FBillboarderMeshCS(const ShaderMetaType::CompiledShaderInitializerType &Initializer);
		FBillboarderMeshCS();

		void			Dispatch(FRHICommandList& RHICmdList, const FMeshCS_Params &params);
#if (ENGINE_MINOR_VERSION < 25)
		virtual bool	Serialize(FArchive& Ar);
#endif // (ENGINE_MINOR_VERSION < 25)
	public:
#if (ENGINE_MINOR_VERSION >= 25)
		LAYOUT_FIELD(FShaderParameter, OutputMask);
		LAYOUT_FIELD(FShaderParameter, InputMask);
		LAYOUT_FIELD(FShaderParameter, InputOffset);
		LAYOUT_FIELD(FShaderParameter, OutputVertexOffset);
		LAYOUT_FIELD(FShaderParameter, PositionsScale);
		LAYOUT_ARRAY(FShaderResourceParameter, Outputs, EOutput::_Count);
		LAYOUT_FIELD(FShaderResourceParameter, InSimData);
		LAYOUT_ARRAY(FShaderParameter, InputsOffsets, EInput::_Count);
		LAYOUT_ARRAY(FShaderParameter, InputsDefault, EInput::_Count);
		LAYOUT_FIELD(FShaderParameter, HasLiveParticleCount);
		LAYOUT_FIELD(FShaderResourceParameter, LiveParticleCount);
#else
		FShaderParameter				OutputMask;
		FShaderParameter				InputMask;
		FShaderParameter				InputOffset;
		FShaderParameter				OutputVertexOffset;
		FShaderParameter				PositionsScale;
		FShaderResourceParameter		Outputs[EOutput::_Count];
		FShaderResourceParameter		InSimData;
		FShaderParameter				InputsOffsets[EInput::_Count];
		FShaderParameter				InputsDefault[EInput::_Count];
		FShaderParameter				HasLiveParticleCount;
		FShaderResourceParameter		LiveParticleCount;
#endif // (ENGINE_MINOR_VERSION >= 25)
	};

	//----------------------------------------------------------------------------
	//
	//	Billboard renderer copy size buffer
	//
	//----------------------------------------------------------------------------

	class	FCopySizeBufferCS : public FGlobalShader
	{
		DECLARE_SHADER_TYPE(FCopySizeBufferCS, Global);

	public:
		typedef FGlobalShader	Super;

		static bool			ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters);
		static void			ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment);

		FCopySizeBufferCS(const ShaderMetaType::CompiledShaderInitializerType &Initializer);
		FCopySizeBufferCS();

		void			Dispatch(FRHICommandList &RHICmdList, const FCSCopySizeBuffer_Params &params);
#if (ENGINE_MINOR_VERSION < 25)
		virtual bool	Serialize(FArchive &Ar);
#endif // (ENGINE_MINOR_VERSION < 25)
	public:
#if (ENGINE_MINOR_VERSION >= 25)
		LAYOUT_FIELD(FShaderParameter, InDrawIndirectArgsBufferOffset);
		LAYOUT_FIELD(FShaderParameter, IndexCountPerInstance);
		LAYOUT_FIELD(FShaderResourceParameter, DrawIndirectArgsBuffer);
		LAYOUT_FIELD(FShaderResourceParameter, LiveParticleCount);
#else
		FShaderParameter			InDrawIndirectArgsBufferOffset;
		FShaderParameter			IndexCountPerInstance;
		FShaderResourceParameter	DrawIndirectArgsBuffer;
		FShaderResourceParameter	LiveParticleCount;
#endif // (ENGINE_MINOR_VERSION >= 25)
	};

	//----------------------------------------------------------------------------

} // namespace PopcornFXBillboarder

#endif // (PK_HAS_GPU != 0)
