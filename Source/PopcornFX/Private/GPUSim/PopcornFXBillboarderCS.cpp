//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXBillboarderCS.h"

#if (PK_HAS_GPU != 0)

#include "Render/PopcornFXShaderUtils.h"

#if (PK_GPU_D3D11 == 1)
#	if defined(PK_DURANGO)
#		include <d3d11_x.h>
#	else
#		include <d3d11.h>
#	endif // defined(PK_DURANGO)
#	include "D3D11RHI.h"
#   if PLATFORM_WINDOWS
#		ifdef WINDOWS_PLATFORM_TYPES_GUARD
#			include "Windows/HideWindowsPlatformTypes.h"
#		endif
#	endif
#	include "D3D11State.h"
#	include "D3D11Resources.h"
#endif // (PK_GPU_D3D11 == 1)
#if (PK_GPU_D3D12 == 1)
#	if PLATFORM_WINDOWS
#		ifdef WINDOWS_PLATFORM_TYPES_GUARD
#			include "Windows/HideWindowsPlatformTypes.h"
#		endif
#	endif
#	include "D3D12RHIPrivate.h"
//#	include "D3D12View.h"
#endif // (PK_GPU_D3D12 == 1)

#include "SceneUtils.h"

#define CS_BB_THREADGROUP_SIZE		128

//----------------------------------------------------------------------------
namespace PopcornFXBillboarder
{
	//----------------------------------------------------------------------------

#if (PK_GPU_D3D11 != 0) || (PK_GPU_D3D12 != 0)
	// static
	bool	FUAVsClearCS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
	{
		return _IsGpuSupportedOnPlatform(Parameters.Platform);
	}

	//----------------------------------------------------------------------------

	FUAVsClearCS::FUAVsClearCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	:	FGlobalShader(Initializer)
	{
		ClearBufferCSParams.Bind(Initializer.ParameterMap, TEXT("ClearBufferCSParams"), SPF_Mandatory);
		UAVRaw.Bind(Initializer.ParameterMap, TEXT("UAVRaw"));
		UAV.Bind(Initializer.ParameterMap, TEXT("UAV"));
		RawUAV.Bind(Initializer.ParameterMap, TEXT("RawUAV"), SPF_Mandatory);
	}

	//----------------------------------------------------------------------------

	void	FUAVsClearCS::Dispatch(const SUERenderContext &renderContext, FRHICommandList& RHICmdList, const FClearCS_Params &params)
	{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		FRHIBatchedShaderParameters	&batchedParameters = RHICmdList.GetScratchShaderParameters();
#endif

		// Right now, one dispatch per UAV to clear
		// But we could do a two dispatchs for all UAVs -- Vertex declaration doesn't change
		// -- We know the max UAV count
		for (u32 i = 0; i < EOutput::_Count; ++i)
		{
			if (IsValidRef(params.m_UAVs[i]))
			{
				FCSRHIParamRef	shader = RHICmdList.GetBoundComputeShader();

				// TODO: We can have the clear count without doing this
				u32	byteSize = 0;
#if (PK_GPU_D3D11 != 0)
				if (renderContext.m_API == SUERenderContext::D3D11)
				{
					const FD3D11UnorderedAccessView		*view = (FD3D11UnorderedAccessView*)params.m_UAVs[i].GetReference();
					D3D11_UNORDERED_ACCESS_VIEW_DESC	desc;
					view->View->GetDesc(&desc);
					byteSize = desc.Buffer.NumElements * 4;

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
					SetUAVParameter(batchedParameters, UAV, params.m_UAVs[i]);
					SetUAVParameter(batchedParameters, UAVRaw, null);
					SetShaderValue(batchedParameters, RawUAV, 0);
#else
					RHICmdList.SetUAVParameter(shader, UAV.GetBaseIndex(), params.m_UAVs[i]);
					RHICmdList.SetUAVParameter(shader, UAVRaw.GetBaseIndex(), null);
					SetShaderValue(RHICmdList, shader, RawUAV, 0);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
				}
#endif // (PK_GPU_D3D11 != 0)
#if (PK_GPU_D3D12 != 0)
				if (renderContext.m_API == SUERenderContext::D3D12)
				{
					const FD3D12UnorderedAccessView			*view = (FD3D12UnorderedAccessView*)params.m_UAVs[i].GetReference();
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
					const D3D12_UNORDERED_ACCESS_VIEW_DESC	&desc = view->GetD3DDesc();
#else
					const D3D12_UNORDERED_ACCESS_VIEW_DESC	&desc = view->GetDesc();
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
					byteSize = desc.Buffer.NumElements * 4;

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
					SetUAVParameter(batchedParameters, UAV, null);
					SetUAVParameter(batchedParameters, UAVRaw, params.m_UAVs[i]);
					SetShaderValue(batchedParameters, RawUAV, 1);
#else
					RHICmdList.SetUAVParameter(shader, UAV.GetBaseIndex(), null);
					RHICmdList.SetUAVParameter(shader, UAVRaw.GetBaseIndex(), params.m_UAVs[i]);
					SetShaderValue(RHICmdList, shader, RawUAV, 1);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
				}
#endif // (PK_GPU_D3D12 != 0)

				const u32	numDWordsToClear = (byteSize + 3) / 4;
				const u32	numThreadGroupsX = (numDWordsToClear + 63) / 64;

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
				SetShaderValue(batchedParameters, ClearBufferCSParams, FUintVector4(0/*Clear value*/, numDWordsToClear, 0, 0));
				RHICmdList.SetBatchedShaderParameters(shader, batchedParameters);
#else
				SetShaderValue(RHICmdList, shader, ClearBufferCSParams, FUintVector4(0/*Clear value*/, numDWordsToClear, 0, 0));
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
				RHICmdList.DispatchComputeShader(numThreadGroupsX, 1, 1);

#if (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)
				SetUAVParameter(RHICmdList, shader, UAVRaw, FUnorderedAccessViewRHIRef());
				SetUAVParameter(RHICmdList, shader, UAV, FUnorderedAccessViewRHIRef());
#endif // (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)
			}
		}
	}
#endif // (PK_GPU_D3D11 != 0) || (PK_GPU_D3D12 != 0)

	//----------------------------------------------------------------------------

	// static
	bool	FBillboarderMeshCS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
	{
		return _IsGpuSupportedOnPlatform(Parameters.Platform);
	}

	//----------------------------------------------------------------------------

	//static
	void	FBillboarderMeshCS::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment)
	{
		Super::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("PK_GPU_THREADGROUP_SIZE"), CS_BB_THREADGROUP_SIZE);
		OutEnvironment.SetDefine(TEXT("PK_BILLBOARDER_CS_OUTPUT_PACK_COLOR_F16"), uint32(PK_BILLBOARDER_CS_OUTPUT_PACK_COLOR_F16));
	}

	//----------------------------------------------------------------------------

	FBillboarderMeshCS::FBillboarderMeshCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	:	FGlobalShader(Initializer)
	{
		OutputMask.Bind(Initializer.ParameterMap, TEXT("OutputMask"));
		InputMask.Bind(Initializer.ParameterMap, TEXT("InputMask"));
		InputOffset.Bind(Initializer.ParameterMap, TEXT("InputOffset"));
		OutputVertexOffset.Bind(Initializer.ParameterMap, TEXT("OutputVertexOffset"));
		PositionsScale.Bind(Initializer.ParameterMap, TEXT("PositionsScale"));

#define X_PK_BILLBOARDER_OUTPUT(__output) Outputs[EOutput :: __output].Bind(Initializer.ParameterMap, TEXT(#__output));
		EXEC_X_PK_BILLBOARDER_OUTPUT()
#undef X_PK_BILLBOARDER_OUTPUT
		InSimData.Bind(Initializer.ParameterMap, TEXT("InSimData"));
#define X_PK_BILLBOARDER_INPUT(__input)																		\
			InputsOffsets[EInput::__input].Bind(Initializer.ParameterMap, TEXT(#__input "Offset"));			\
			InputsDefault[EInput::__input].Bind(Initializer.ParameterMap, TEXT("Default") TEXT(#__input));
			EXEC_X_PK_BILLBOARDER_INPUT()
#undef X_PK_BILLBOARDER_INPUT

		HasLiveParticleCount.Bind(Initializer.ParameterMap, TEXT("HasLiveParticleCount"));
		LiveParticleCount.Bind(Initializer.ParameterMap, TEXT("LiveParticleCount"));
	}

	//----------------------------------------------------------------------------

	FBillboarderMeshCS::FBillboarderMeshCS()
	{
	}

	//----------------------------------------------------------------------------

	void	FBillboarderMeshCS::Dispatch(FRHICommandList& RHICmdList, const FMeshCS_Params &params)
	{
		SCOPED_DRAW_EVENT(RHICmdList, PopcornFXBillboarderMesh_Dispatch);

		FCSRHIParamRef	shader = RHICmdList.GetBoundComputeShader();

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		FRHIBatchedShaderParameters	&batchedParameters = RHICmdList.GetScratchShaderParameters();
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)

		uint32	outputMask = 0;
		uint32	inputMask = 0;

		for (u32 i = 0; i < EOutput::_Count; ++i)
		{
			if (IsValidRef(params.m_Outputs[i]))
			{
				outputMask |= (1 << i); // if buffer is provided, always set the mask, even if not used (not bound)
				if (Outputs[i].IsBound())
				{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
					SetUAVParameter(batchedParameters, Outputs[i], params.m_Outputs[i]);
#else
					RHICmdList.SetUAVParameter(shader, Outputs[i].GetBaseIndex(), params.m_Outputs[i]);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
				}
			}
			else if (Outputs[i].IsBound())
			{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
				SetUAVParameter(batchedParameters, Outputs[i], null); // avoids D3D11 warnings !?
#else
				RHICmdList.SetUAVParameter(shader, Outputs[i].GetBaseIndex(), null); // avoids D3D11 warnings !?
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
			}
		}

		for (u32 i = 0; i < EInput::_Count; ++i)
		{
			if (params.m_ValidInputs[i])
				inputMask |= (1 << i); // if buffer is provided, always set the mask, even if not used (not bound)

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
			SetShaderValue(batchedParameters, InputsOffsets[i], params.m_InputsOffsets[i]);
			SetShaderValue(batchedParameters, InputsDefault[i], _Reinterpret<FVector4f>(params.m_InputsDefault[i]));
#else
			SetShaderValue(RHICmdList, shader, InputsOffsets[i], params.m_InputsOffsets[i]);
			SetShaderValue(RHICmdList, shader, InputsDefault[i], _Reinterpret<FVector4f>(params.m_InputsDefault[i]));
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		}

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		SetSRVParameter(batchedParameters, InSimData, IsValidRef(params.m_InSimData) ? params.m_InSimData : null);

		SetShaderValue(batchedParameters, OutputMask, outputMask);
		SetShaderValue(batchedParameters, InputMask, inputMask);
		SetShaderValue(batchedParameters, InputOffset, params.m_InputOffset);
		SetShaderValue(batchedParameters, OutputVertexOffset, params.m_OutputVertexOffset);
		SetShaderValue(batchedParameters, PositionsScale, params.m_PositionsScale);
#else
		RHICmdList.SetShaderResourceViewParameter(shader, InSimData.GetBaseIndex(), IsValidRef(params.m_InSimData) ? params.m_InSimData : null);

		SetShaderValue(RHICmdList, shader, OutputMask, outputMask);
		SetShaderValue(RHICmdList, shader, InputMask, inputMask);
		SetShaderValue(RHICmdList, shader, InputOffset, params.m_InputOffset);
		SetShaderValue(RHICmdList, shader, OutputVertexOffset, params.m_OutputVertexOffset);
		SetShaderValue(RHICmdList, shader, PositionsScale, params.m_PositionsScale);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)

		uint32		hasLiveParticleCount = 0;
		if (IsValidRef(params.m_LiveParticleCount))
			hasLiveParticleCount = 1;

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		SetShaderValue(batchedParameters, HasLiveParticleCount, hasLiveParticleCount);
#else
		SetShaderValue(RHICmdList, shader, HasLiveParticleCount, hasLiveParticleCount);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		if (IsValidRef(params.m_LiveParticleCount) && LiveParticleCount.IsBound())
		{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
			SetSRVParameter(batchedParameters, LiveParticleCount, params.m_LiveParticleCount);
#else
			RHICmdList.SetShaderResourceViewParameter(shader, LiveParticleCount.GetBaseIndex(), params.m_LiveParticleCount);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		}

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		RHICmdList.SetBatchedShaderParameters(shader, batchedParameters);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)

		{
			PK_NAMEDSCOPEDPROFILE("FBillboarderMeshCS::Dispatch");
			const uint32	threadGroupCount = PopcornFX::Mem::Align(params.m_ParticleCount, CS_BB_THREADGROUP_SIZE) / CS_BB_THREADGROUP_SIZE;
			RHICmdList.DispatchComputeShader(threadGroupCount, 1, 1);
		}

#if (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)
		FUAVRHIParamRef			nullUAV = FUAVRHIParamRef();
		FSRVRHIParamRef			nullSRV = FSRVRHIParamRef();
		for (u32 i = 0; i < EOutput::_Count; ++i)
		{
			if (IsValidRef(params.m_Outputs[i]) && Outputs[i].IsBound())
				RHICmdList.SetUAVParameter(shader, Outputs[i].GetBaseIndex(), nullUAV);
		}
		RHICmdList.SetShaderResourceViewParameter(shader, InSimData.GetBaseIndex(), nullSRV);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	}

	//----------------------------------------------------------------------------

	// static
	bool	FCopySizeBufferCS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
	{
		return _IsGpuSupportedOnPlatform(Parameters.Platform);
	}

	//----------------------------------------------------------------------------

	FCopySizeBufferCS::FCopySizeBufferCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		InDrawIndirectArgsBufferOffset.Bind(Initializer.ParameterMap, TEXT("InDrawIndirectArgsBufferOffset"));
		IndexCountPerInstance.Bind(Initializer.ParameterMap, TEXT("IndexCountPerInstance"));

		DrawIndirectArgsBuffer.Bind(Initializer.ParameterMap, TEXT("DrawIndirectArgsBuffer"));
		LiveParticleCount.Bind(Initializer.ParameterMap, TEXT("LiveParticleCount"));
	}

	//----------------------------------------------------------------------------

	FCopySizeBufferCS::FCopySizeBufferCS()
	{
	}

	//----------------------------------------------------------------------------

	void	FCopySizeBufferCS::Dispatch(FRHICommandList& RHICmdList, const FCSCopySizeBuffer_Params &params)
	{
		FCSRHIParamRef	shader = RHICmdList.GetBoundComputeShader();

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		FRHIBatchedShaderParameters	&batchedParameters = RHICmdList.GetScratchShaderParameters();

		SetShaderValue(batchedParameters, InDrawIndirectArgsBufferOffset, params.m_DrawIndirectArgsOffset);
		SetShaderValue(batchedParameters, IndexCountPerInstance, params.m_IsCapsule ? 12 : 6);

		SetSRVParameter(batchedParameters, LiveParticleCount, params.m_LiveParticleCount);
		SetUAVParameter(batchedParameters, DrawIndirectArgsBuffer, params.m_DrawIndirectArgsBuffer);

		RHICmdList.SetBatchedShaderParameters(shader, batchedParameters);
#else
		SetShaderValue(RHICmdList, shader, InDrawIndirectArgsBufferOffset, params.m_DrawIndirectArgsOffset);
		SetShaderValue(RHICmdList, shader, IndexCountPerInstance, params.m_IsCapsule ? 12 : 6);

		RHICmdList.SetShaderResourceViewParameter(shader, LiveParticleCount.GetBaseIndex(), params.m_LiveParticleCount);
		RHICmdList.SetUAVParameter(shader, DrawIndirectArgsBuffer.GetBaseIndex(), params.m_DrawIndirectArgsBuffer);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)


		{
			PK_NAMEDSCOPEDPROFILE("FCopySizeBufferCS::Dispatch");
			RHICmdList.DispatchComputeShader(1, 1, 1);
		}

#if (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)
		FSRVRHIParamRef		nullSRV = FSRVRHIParamRef();
		FUAVRHIParamRef		nullUAV = FUAVRHIParamRef();
		RHICmdList.SetUAVParameter(shader, DrawIndirectArgsBuffer.GetBaseIndex(), nullUAV);
		RHICmdList.SetShaderResourceViewParameter(shader, LiveParticleCount.GetBaseIndex(), nullSRV);
#endif // (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)
	}

} // namespace PopcornFXBillboarder

//----------------------------------------------------------------------------

IMPLEMENT_SHADER_TYPE(, PopcornFXBillboarder::FCopySizeBufferCS, TEXT(PKUE_GLOBAL_SHADER_PATH("PopcornFXCopySizeBufferComputeShader")), TEXT("Copy"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, PopcornFXBillboarder::FBillboarderMeshCS, TEXT(PKUE_GLOBAL_SHADER_PATH("PopcornFXBillboarderMeshComputeShader")), TEXT("Billboard"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, PopcornFXBillboarder::FUAVsClearCS, TEXT(PKUE_GLOBAL_SHADER_PATH("PopcornFXUAVsClearComputeShader")), TEXT("Clear"), SF_Compute);

//----------------------------------------------------------------------------

#endif // (PK_HAS_GPU != 0)
