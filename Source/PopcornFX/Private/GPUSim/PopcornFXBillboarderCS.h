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

//----------------------------------------------------------------------------

#if (PK_HAS_GPU != 0)

//----------------------------------------------------------------------------
namespace PopcornFXBillboarder
{
	//----------------------------------------------------------------------------

#define EXEC_X_PK_BILLBOARDER_INPUT()					\
		X_PK_BILLBOARDER_INPUT(InIndices)				\
		X_PK_BILLBOARDER_INPUT(InPositions)				\
		X_PK_BILLBOARDER_INPUT(InOrientations)			\
		X_PK_BILLBOARDER_INPUT(InScales)

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

	//----------------------------------------------------------------------------

#define EXEC_X_PK_BILLBOARDER_OUTPUT()					\
		X_PK_BILLBOARDER_OUTPUT(OutMatrices)

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
				m_UAVs[output] = buffer->UAV();
			else
				m_UAVs[output] = null;
		}

		void	SetUAV(EOutput::Type output, CPooledIndexBuffer &buffer)
		{
			if (buffer.Valid())
				m_UAVs[output] = buffer->UAV();
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
				m_Outputs[output] = buffer->UAV();
			else
				m_Outputs[output] = null;
		}

		void	SetOutput(EOutput::Type output, bool condition, CPooledIndexBuffer &buffer)
		{
			if (condition && PK_VERIFY(buffer.Valid()))
				m_Outputs[output] = buffer->UAV();
			else
				m_Outputs[output] = null;
		}

		void	SetOutput(EOutput::Type output, CPooledVertexBuffer &buffer)
		{
			if (buffer.Valid())
				m_Outputs[output] = buffer->UAV();
			else
				m_Outputs[output] = null;
		}

		void	SetOutput(EOutput::Type output, CPooledIndexBuffer &buffer)
		{
			if (buffer.Valid())
				m_Outputs[output] = buffer->UAV();
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

		FUAVsClearCS(const ShaderMetaType::CompiledShaderInitializerType &Initializer);
		FUAVsClearCS() { }

		void			Dispatch(const SUERenderContext &renderContext, FRHICommandList &RHICmdList, const FClearCS_Params &params);
	public:
		LAYOUT_FIELD(FShaderParameter, ClearBufferCSParams);
		LAYOUT_FIELD(FShaderResourceParameter, UAVRaw);
		LAYOUT_FIELD(FShaderResourceParameter, UAV);
		LAYOUT_FIELD(FShaderParameter, RawUAV);
	};
#endif // (PK_GPU_D3D11 != 0) || (PK_GPU_D3D12 != 0)

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
	public:
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
	};

	//----------------------------------------------------------------------------
	//
	//	GPU renderer copy size buffer
	//
	//----------------------------------------------------------------------------

	class	FCopySizeBufferCS : public FGlobalShader
	{
		DECLARE_SHADER_TYPE(FCopySizeBufferCS, Global);

	public:
		typedef FGlobalShader	Super;

		static bool			ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters);

		FCopySizeBufferCS(const ShaderMetaType::CompiledShaderInitializerType &Initializer);
		FCopySizeBufferCS();

		void			Dispatch(FRHICommandList &RHICmdList, const FCSCopySizeBuffer_Params &params);
	public:
		LAYOUT_FIELD(FShaderParameter, InDrawIndirectArgsBufferOffset);
		LAYOUT_FIELD(FShaderParameter, IndexCountPerInstance);
		LAYOUT_FIELD(FShaderResourceParameter, DrawIndirectArgsBuffer);
		LAYOUT_FIELD(FShaderResourceParameter, LiveParticleCount);
	};

	//----------------------------------------------------------------------------

} // namespace PopcornFXBillboarder

#endif // (PK_HAS_GPU != 0)
