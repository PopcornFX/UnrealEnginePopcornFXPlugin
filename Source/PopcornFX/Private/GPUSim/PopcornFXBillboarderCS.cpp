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

	EBillboarder::Type		BillboarderModeToType(PopcornFX::EBillboardMode mode)
	{
		EBillboarder::Type		bbType = (EBillboarder::Type)~0;
		switch (mode)
		{
		case	PopcornFX::EBillboardMode::BillboardMode_ScreenAligned:
			bbType = EBillboarder::ScreenAligned;
			break;
		case	PopcornFX::EBillboardMode::BillboardMode_ViewposAligned:
			bbType = EBillboarder::ViewposAligned;
			break;
		case	PopcornFX::EBillboardMode::BillboardMode_AxisAligned:
			bbType = EBillboarder::AxisAligned;
			break;
		case	PopcornFX::EBillboardMode::BillboardMode_AxisAlignedSpheroid:
			bbType = EBillboarder::AxisAlignedSpheroid;
			break;
		case	PopcornFX::EBillboardMode::BillboardMode_AxisAlignedCapsule:
			bbType = EBillboarder::AxisAlignedCapsule;
			break;
		case	PopcornFX::EBillboardMode::BillboardMode_PlaneAligned:
			bbType = EBillboarder::PlaneAligned;
			break;
		default:
			PK_ASSERT_NOT_REACHED();
			break;
		}
		return bbType;
	}

	//----------------------------------------------------------------------------

	u32		BillboarderTypeMaskMustHaveInputMask(u32 typeMask)
	{
		PK_STATIC_ASSERT(EBillboarder::_Count < sizeof(typeMask) * 8);
		PK_STATIC_ASSERT(EInput::_Count < sizeof(typeMask) * 8);

		u32		inputMask = 0;
		inputMask |= (1 << EInput::InPositions);
		if (typeMask & (1 << EBillboarder::ScreenAligned))
		{
		}
		if (typeMask & (1 << EBillboarder::ViewposAligned))
		{
		}
		if (typeMask & (1 << EBillboarder::AxisAligned))
		{
			inputMask |= (1 << EInput::InAxis0s);
		}
		if (typeMask & (1 << EBillboarder::AxisAlignedCapsule))
		{
			inputMask |= (1 << EInput::InAxis0s);
		}
		if (typeMask & (1 << EBillboarder::AxisAlignedSpheroid))
		{
			inputMask |= (1 << EInput::InAxis0s);
		}
		if (typeMask & (1 << EBillboarder::PlaneAligned))
		{
			inputMask |= (1 << EInput::InAxis0s);
			inputMask |= (1 << EInput::InAxis1s);
		}
		PK_STATIC_ASSERT(EBillboarder::_Count == 6); // check above
		return inputMask;
	}

	//----------------------------------------------------------------------------

	void		AddDefinesEnumTypeAndMasks(FShaderCompilerEnvironment& OutEnvironment)
	{
#define X_PK_BILLBOARDER_TYPE(__type)		\
	PK_ASSERT(!OutEnvironment.GetDefinitions().Contains(TEXT("BILLBOARD_") TEXT(#__type))); \
	OutEnvironment.SetDefine(TEXT("BILLBOARD_") TEXT(#__type), uint32(EBillboarder::__type));
		EXEC_X_PK_BILLBOARDER_TYPE()
#undef X_PK_BILLBOARDER_TYPE

#define X_PK_BILLBOARDER_TYPE(__type) \
	PK_ASSERT(!OutEnvironment.GetDefinitions().Contains(TEXT("BBMASK_") TEXT(#__type))); \
	OutEnvironment.SetDefine(TEXT("BBMASK_") TEXT(#__type), 1 << uint32(EBillboarder::__type));
		EXEC_X_PK_BILLBOARDER_TYPE()
#undef X_PK_BILLBOARDER_TYPE

#define X_PK_BILLBOARDER_OUTPUT(__output) \
	PK_ASSERT(!OutEnvironment.GetDefinitions().Contains(TEXT("MASK_") TEXT(#__output))); \
	OutEnvironment.SetDefine(TEXT("MASK_") TEXT(#__output), uint32(1 << (EOutput:: __output)));
		EXEC_X_PK_BILLBOARDER_OUTPUT()
#undef X_PK_BILLBOARDER_OUTPUT

#define X_PK_BILLBOARDER_INPUT(__input) \
	PK_ASSERT(!OutEnvironment.GetDefinitions().Contains(TEXT("MASK_") TEXT(#__input))); \
	OutEnvironment.SetDefine(TEXT("MASK_") TEXT(#__input), uint32(1 << (EInput:: __input)));
		EXEC_X_PK_BILLBOARDER_INPUT()
#undef X_PK_BILLBOARDER_INPUT
	}


	//----------------------------------------------------------------------------
	//
	//
	//
	//----------------------------------------------------------------------------

	// static
	template <EBillboarderCSBuild::Type _Build>
	bool	FBillboarderBillboardCS<_Build>::ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
	{
		return _IsGpuSupportedOnPlatform(Parameters.Platform);
	}

	//----------------------------------------------------------------------------

	//static
	template <EBillboarderCSBuild::Type _Build>
	void	FBillboarderBillboardCS<_Build>::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment)
	{
		Super::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("PK_GPU_THREADGROUP_SIZE"), CS_BB_THREADGROUP_SIZE);

		AddDefinesEnumTypeAndMasks(OutEnvironment);

		OutEnvironment.SetDefine(TEXT("RENDERERFLAG_FlipV"), 1 << ERendererFlag::FlipV);
		OutEnvironment.SetDefine(TEXT("RENDERERFLAG_SoftAnimationBlending"), 1 << ERendererFlag::SoftAnimationBlending);

		OutEnvironment.SetDefine(TEXT("PK_BILLBOARDER_CS_OUTPUT_PACK_PTN"), uint32(PK_BILLBOARDER_CS_OUTPUT_PACK_PTN));
		OutEnvironment.SetDefine(TEXT("PK_BILLBOARDER_CS_OUTPUT_PACK_TEXCOORD"), uint32(PK_BILLBOARDER_CS_OUTPUT_PACK_TEXCOORD));

		switch (_Build)
		{
		case EBillboarderCSBuild::Std:
			break;
		case EBillboarderCSBuild::VertexPP:
			OutEnvironment.SetDefine(TEXT("VERTEX_PER_PARTICLE"), 1);
			break;
		}

		//OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);

	}

	//----------------------------------------------------------------------------

	template <EBillboarderCSBuild::Type _Build>
	FBillboarderBillboardCS<_Build>::FBillboarderBillboardCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		BillboarderType.Bind(Initializer.ParameterMap, TEXT("BillboarderType"));
		OutputMask.Bind(Initializer.ParameterMap, TEXT("OutputMask"));
		InputMask.Bind(Initializer.ParameterMap, TEXT("InputMask"));
		InIndicesOffset.Bind(Initializer.ParameterMap, TEXT("InIndicesOffset"));
		InputOffset.Bind(Initializer.ParameterMap, TEXT("InputOffset"));
		OutputVertexOffset.Bind(Initializer.ParameterMap, TEXT("OutputVertexOffset"));
		OutputIndexOffset.Bind(Initializer.ParameterMap, TEXT("OutputIndexOffset"));
		BillboardingMatrix.Bind(Initializer.ParameterMap, TEXT("BillboardingMatrix"));
		RendererFlags.Bind(Initializer.ParameterMap, TEXT("RendererFlags"));
		RendererNormalsBendingFactor.Bind(Initializer.ParameterMap, TEXT("RendererNormalsBendingFactor"));

		RendererAtlasRectCount.Bind(Initializer.ParameterMap, TEXT("RendererAtlasRectCount"));
		RendererAtlasBuffer.Bind(Initializer.ParameterMap, TEXT("RendererAtlasBuffer"));

#define X_PK_BILLBOARDER_OUTPUT(__output) Outputs[EOutput :: __output].Bind(Initializer.ParameterMap, TEXT(#__output));
		EXEC_X_PK_BILLBOARDER_OUTPUT()
#undef X_PK_BILLBOARDER_OUTPUT

		InIndices.Bind(Initializer.ParameterMap, TEXT("InIndices"));
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

	template <EBillboarderCSBuild::Type _Build>
	FBillboarderBillboardCS<_Build>::FBillboarderBillboardCS()
	{
	}

	//----------------------------------------------------------------------------

	template <EBillboarderCSBuild::Type _Build>
	void	FBillboarderBillboardCS<_Build>::Dispatch(FRHICommandList& RHICmdList, const FBillboarderBillboardCS_Params &params)
	{
		const bool		isVPP = (_Build == EBillboarderCSBuild::VertexPP);

#if (ENGINE_MINOR_VERSION >= 25)
		FCSRHIParamRef	shader = RHICmdList.GetBoundComputeShader();
#else
		FCSRHIParamRef	shader = GetComputeShader();
		RHICmdList.SetComputeShader(shader);
#endif //(ENGINE_MINOR_VERSION >= 25)

		uint32				outputMask = 0;
		uint32				inputMask = 0;

		if (isVPP)
		{
			PK_ASSERT(!IsValidRef(params.m_Outputs[EOutput::OutTexcoords]));
			PK_ASSERT(!IsValidRef(params.m_Outputs[EOutput::OutTexcoord2s]));
			PK_ASSERT(!IsValidRef(params.m_Outputs[EOutput::OutTangents]));
		}

		for (u32 i = 0; i < EOutput::_Count; ++i)
		{
			if (IsValidRef(params.m_Outputs[i]))
			{
				outputMask |= (1 << i); // if buffer is provided, always set the mask, even if not used (not bound)
				if (Outputs[i].IsBound())
				{
					// D3D12: Force ERWBarrier right now
					// TODO: Investigate into output vertex offset to aligned stride (cache lines ?)
					// if (params.m_OutputVertexOffset == 0)
					{
#if (ENGINE_MINOR_VERSION >= 26)
						RHICmdList.Transition(FRHITransitionInfo(params.m_Outputs[i], ERHIAccess::VertexOrIndexBuffer, ERHIAccess::UAVCompute));
#else
						RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, params.m_Outputs[i]);
#endif // (ENGINE_MINOR_VERSION >= 26)
						RHICmdList.SetUAVParameter(shader, Outputs[i].GetBaseIndex(), params.m_Outputs[i]);
					}
				}
			}
			else if (Outputs[i].IsBound())
				RHICmdList.SetUAVParameter(shader, Outputs[i].GetBaseIndex(), null); // avoids D3D11 warnings !?
		}

		RHICmdList.SetShaderResourceViewParameter(shader, InIndices.GetBaseIndex(), IsValidRef(params.m_InIndices) ? params.m_InIndices : null);
		RHICmdList.SetShaderResourceViewParameter(shader, InSimData.GetBaseIndex(), IsValidRef(params.m_InSimData) ? params.m_InSimData : null);
		for (u32 i = 0; i < EInput::_Count; ++i)
		{
			if (params.m_ValidInputs[i])
				inputMask |= (1 << i); // if buffer is provided, always set the mask, even if not used (not bound)

			SetShaderValue(RHICmdList, shader, InputsOffsets[i], params.m_InputsOffsets[i]);
			SetShaderValue(RHICmdList, shader, InputsDefault[i], _Reinterpret<FVector4>(params.m_InputsDefault[i]));
		}

		PK_ONLY_IF_ASSERTS(const u32		mustHaveInputMask = BillboarderTypeMaskMustHaveInputMask(1 << params.m_BillboarderType));
		PK_ASSERT((inputMask & mustHaveInputMask) == mustHaveInputMask);

		SetShaderValue(RHICmdList, shader, OutputMask, outputMask);
		SetShaderValue(RHICmdList, shader, InputMask, inputMask);
		SetShaderValue(RHICmdList, shader, BillboarderType, params.m_BillboarderType);
		SetShaderValue(RHICmdList, shader, InIndicesOffset, params.m_InIndicesOffset);
		SetShaderValue(RHICmdList, shader, InputOffset, params.m_InputOffset);
		SetShaderValue(RHICmdList, shader, OutputVertexOffset, params.m_OutputVertexOffset);
		SetShaderValue(RHICmdList, shader, OutputIndexOffset, params.m_OutputIndexOffset);
		SetShaderValue(RHICmdList, shader, BillboardingMatrix, ToUE(params.m_BillboardingMatrix));
		SetShaderValue(RHICmdList, shader, RendererFlags, params.m_RendererFlags);
		SetShaderValue(RHICmdList, shader, RendererNormalsBendingFactor, params.m_RendererNormalsBendingFactor);

		if (IsValidRef(params.m_RendererAtlasBuffer) &&
			RendererAtlasBuffer.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(shader, RendererAtlasBuffer.GetBaseIndex(), params.m_RendererAtlasBuffer);
			SetShaderValue(RHICmdList, shader, RendererAtlasRectCount, params.m_RendererAtlasRectCount);
		}
		else
		{
			if (RendererAtlasBuffer.IsBound())
				RHICmdList.SetShaderResourceViewParameter(shader, RendererAtlasBuffer.GetBaseIndex(), null); // avoids D3D11 warnings !?
			SetShaderValue(RHICmdList, shader, RendererAtlasRectCount, u32(0));
		}

		uint32		hasLiveParticleCount = 0;
		if (IsValidRef(params.m_LiveParticleCount))
			hasLiveParticleCount = 1;
		SetShaderValue(RHICmdList, shader, HasLiveParticleCount, hasLiveParticleCount);
		if (IsValidRef(params.m_LiveParticleCount) && LiveParticleCount.IsBound())
			RHICmdList.SetShaderResourceViewParameter(shader, LiveParticleCount.GetBaseIndex(), params.m_LiveParticleCount);

		{
			PK_NAMEDSCOPEDPROFILE("FBillboarderBillboardCS::Dispatch");
			const uint32	threadGroupCount = PopcornFX::Mem::Align(params.m_ParticleCount, CS_BB_THREADGROUP_SIZE) / CS_BB_THREADGROUP_SIZE;
			RHICmdList.DispatchComputeShader(threadGroupCount, 1, 1);
		}

		FUAVRHIParamRef		nullUAV = FUAVRHIParamRef();
		FSRVRHIParamRef		nullSRV = FSRVRHIParamRef();
		for (u32 i = 0; i < EOutput::_Count; ++i)
		{
			if (IsValidRef(params.m_Outputs[i]) && Outputs[i].IsBound())
			{
#if (ENGINE_MINOR_VERSION >= 26)
				RHICmdList.Transition(FRHITransitionInfo(params.m_Outputs[i], ERHIAccess::UAVCompute, ERHIAccess::VertexOrIndexBuffer));
#else
				//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, params.m_Outputs[i]);
#endif // (ENGINE_MINOR_VERSION >= 26)
				RHICmdList.SetUAVParameter(shader, Outputs[i].GetBaseIndex(), nullUAV);
			}
		}
		RHICmdList.SetShaderResourceViewParameter(shader, InIndices.GetBaseIndex(), nullSRV);
		RHICmdList.SetShaderResourceViewParameter(shader, InSimData.GetBaseIndex(), nullSRV);
	}

	//----------------------------------------------------------------------------

#if (ENGINE_MINOR_VERSION < 25)
	template <EBillboarderCSBuild::Type _Build>
	bool	FBillboarderBillboardCS<_Build>::Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << BillboarderType;
		Ar << OutputMask;
		Ar << InputMask;
		Ar << InIndicesOffset;
		Ar << InputOffset;
		Ar << OutputVertexOffset;
		Ar << OutputIndexOffset;
		Ar << BillboardingMatrix;
		Ar << RendererFlags;
		Ar << RendererNormalsBendingFactor;
		Ar << RendererAtlasRectCount;
		Ar << RendererAtlasBuffer;
		for (u32 i = 0; i < EOutput::_Count; ++i)
			Ar << Outputs[i];
		Ar << InIndices;
		Ar << InSimData;
		for (u32 i = 0; i < EInput::_Count; ++i)
		{
			Ar << InputsOffsets[i];
			Ar << InputsDefault[i];
		}
		Ar << HasLiveParticleCount;
		Ar << LiveParticleCount;
		return bShaderHasOutdatedParameters;
	}
#endif // (ENGINE_MINOR_VERSION < 25)

	//----------------------------------------------------------------------------

	template class FBillboarderBillboardCS<EBillboarderCSBuild::Std>;
	template class FBillboarderBillboardCS<EBillboarderCSBuild::VertexPP>;

	//----------------------------------------------------------------------------

	// static
	bool	FCopyStreamCS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
	{
		return _IsGpuSupportedOnPlatform(Parameters.Platform);
	}

	//----------------------------------------------------------------------------

	//static
	void	FCopyStreamCS::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment)
	{
		Super::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("PK_GPU_THREADGROUP_SIZE"), CS_BB_THREADGROUP_SIZE);
		AddDefinesEnumTypeAndMasks(OutEnvironment);
		OutEnvironment.SetDefine(TEXT("PK_BILLBOARDER_CS_OUTPUT_PACK_COLOR_F16"), uint32(PK_BILLBOARDER_CS_OUTPUT_PACK_COLOR_F16));
	}

	//----------------------------------------------------------------------------

	FCopyStreamCS::FCopyStreamCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		OutputMask.Bind(Initializer.ParameterMap, TEXT("OutputMask"));
		InputMask.Bind(Initializer.ParameterMap, TEXT("InputMask"));
		InputOffset.Bind(Initializer.ParameterMap, TEXT("InputOffset"));
		OutputVertexOffset.Bind(Initializer.ParameterMap, TEXT("OutputVertexOffset"));
		IsCapsule.Bind(Initializer.ParameterMap, TEXT("IsCapsule"));

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

	FCopyStreamCS::FCopyStreamCS()
	{
	}

	//----------------------------------------------------------------------------

	void	FCopyStreamCS::Dispatch(FRHICommandList& RHICmdList, const FBillboardCS_Params &params)
	{
#if (ENGINE_MINOR_VERSION >= 25)
		FCSRHIParamRef	shader = RHICmdList.GetBoundComputeShader();
#else
		FCSRHIParamRef	shader = GetComputeShader();
		RHICmdList.SetComputeShader(shader);
#endif //(ENGINE_MINOR_VERSION >= 25)

		uint32	outputMask = 0;
		uint32	inputMask = 0;

		for (u32 i = 0; i < EOutput::_Count; ++i)
		{
			if (IsValidRef(params.m_Outputs[i]))
			{
				outputMask |= (1 << i); // if buffer is provided, always set the mask, even if not used (not bound)
				if (Outputs[i].IsBound())
				{
					// D3D12: Force ERWBarrier right now
					// TODO: Investigate into output vertex offset to aligned stride (cache lines ?)
					// if (params.m_OutputVertexOffset == 0)
					{
#if (ENGINE_MINOR_VERSION >= 26)
						RHICmdList.Transition(FRHITransitionInfo(params.m_Outputs[i], ERHIAccess::VertexOrIndexBuffer, ERHIAccess::UAVCompute));
#else
						RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, params.m_Outputs[i]);
#endif // (ENGINE_MINOR_VERSION >= 26)
						RHICmdList.SetUAVParameter(shader, Outputs[i].GetBaseIndex(), params.m_Outputs[i]);
					}
				}
			}
			else if (Outputs[i].IsBound())
				RHICmdList.SetUAVParameter(shader, Outputs[i].GetBaseIndex(), null); // avoids D3D11 warnings !?
		}

		RHICmdList.SetShaderResourceViewParameter(shader, InSimData.GetBaseIndex(), IsValidRef(params.m_InSimData) ? params.m_InSimData : null);
		for (u32 i = 0; i < EInput::_Count; ++i)
		{
			if (params.m_ValidInputs[i])
				inputMask |= (1 << i); // if buffer is provided, always set the mask, even if not used (not bound)

			SetShaderValue(RHICmdList, shader, InputsOffsets[i], params.m_InputsOffsets[i]);
			SetShaderValue(RHICmdList, shader, InputsDefault[i], _Reinterpret<FVector4>(params.m_InputsDefault[i]));
		}

		const bool	isCapsule = params.m_BillboarderType == EBillboarder::AxisAlignedCapsule;
		SetShaderValue(RHICmdList, shader, OutputMask, outputMask);
		SetShaderValue(RHICmdList, shader, InputMask, inputMask);
		SetShaderValue(RHICmdList, shader, InputOffset, params.m_InputOffset);
		SetShaderValue(RHICmdList, shader, OutputVertexOffset, params.m_OutputVertexOffset);
		SetShaderValue(RHICmdList, shader, IsCapsule, isCapsule ? 1 : 0);

		uint32		hasLiveParticleCount = 0;
		if (IsValidRef(params.m_LiveParticleCount))
			hasLiveParticleCount = 1;
		SetShaderValue(RHICmdList, shader, HasLiveParticleCount, hasLiveParticleCount);
		if (IsValidRef(params.m_LiveParticleCount) && LiveParticleCount.IsBound())
			RHICmdList.SetShaderResourceViewParameter(shader, LiveParticleCount.GetBaseIndex(), params.m_LiveParticleCount);

		{
			PK_NAMEDSCOPEDPROFILE("FCopyStreamCS::Dispatch");
			const uint32	threadGroupCount = PopcornFX::Mem::Align(params.m_ParticleCount, CS_BB_THREADGROUP_SIZE) / CS_BB_THREADGROUP_SIZE;
			RHICmdList.DispatchComputeShader(threadGroupCount, 1, 1);
		}

		FUAVRHIParamRef		nullUAV = FUAVRHIParamRef();
		FSRVRHIParamRef		nullSRV = FSRVRHIParamRef();
		for (u32 i = 0; i < EOutput::_Count; ++i)
		{
			if (IsValidRef(params.m_Outputs[i]) && Outputs[i].IsBound())
			{
#if (ENGINE_MINOR_VERSION >= 26)
				RHICmdList.Transition(FRHITransitionInfo(params.m_Outputs[i], ERHIAccess::UAVCompute, ERHIAccess::VertexOrIndexBuffer));
#else
				//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, params.m_Outputs[i]);
#endif // (ENGINE_MINOR_VERSION >= 26)

				RHICmdList.SetUAVParameter(shader, Outputs[i].GetBaseIndex(), nullUAV);
			}
		}
		RHICmdList.SetShaderResourceViewParameter(shader, InSimData.GetBaseIndex(), nullSRV);
	}

	//----------------------------------------------------------------------------

#if (ENGINE_MINOR_VERSION < 25)
	bool	FCopyStreamCS::Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << OutputMask;
		Ar << InputMask;
		Ar << InputOffset;
		Ar << OutputVertexOffset;
		Ar << IsCapsule;
		for (u32 i = 0; i < EOutput::_Count; ++i)
			Ar << Outputs[i];
		Ar << InSimData;
		for (u32 i = 0; i < EInput::_Count; ++i)
		{
			Ar << InputsOffsets[i];
			Ar << InputsDefault[i];
		}
		Ar << HasLiveParticleCount;
		Ar << LiveParticleCount;
		return bShaderHasOutdatedParameters;
	}
#endif // (ENGINE_MINOR_VERSION < 25)

	//----------------------------------------------------------------------------

#if (PK_GPU_D3D11 != 0) || (PK_GPU_D3D12 != 0)
	// static
	bool	FUAVsClearCS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
	{
		return _IsGpuSupportedOnPlatform(Parameters.Platform);
	}

	//----------------------------------------------------------------------------

	//static
	void	FUAVsClearCS::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment)
	{
		Super::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		AddDefinesEnumTypeAndMasks(OutEnvironment);
	}

	//----------------------------------------------------------------------------

	FUAVsClearCS::FUAVsClearCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		ClearBufferCSParams.Bind(Initializer.ParameterMap, TEXT("ClearBufferCSParams"), SPF_Mandatory);
		UAVRaw.Bind(Initializer.ParameterMap, TEXT("UAVRaw"));
		UAV.Bind(Initializer.ParameterMap, TEXT("UAV"));
		RawUAV.Bind(Initializer.ParameterMap, TEXT("RawUAV"), SPF_Mandatory);
	}

	//----------------------------------------------------------------------------

	FUAVsClearCS::FUAVsClearCS()
	{
	}

	//----------------------------------------------------------------------------

	void	FUAVsClearCS::Dispatch(const SRenderContext &renderContext, FRHICommandList& RHICmdList, const FClearCS_Params &params)
	{
		// Right now, one dispatch per UAV to clear
		// But we could do a two dispatchs for all UAVs -- Vertex declaration doesn't change
		// -- We know the max UAV count
		for (u32 i = 0; i < EOutput::_Count; ++i)
		{
			if (IsValidRef(params.m_UAVs[i]))
			{
#if (ENGINE_MINOR_VERSION >= 25)
				FCSRHIParamRef	shader = RHICmdList.GetBoundComputeShader();
#else
				FCSRHIParamRef	shader = GetComputeShader();
				RHICmdList.SetComputeShader(shader);
#endif //(ENGINE_MINOR_VERSION >= 25)

				// TODO: We can have the clear count without doing this
				u32	byteSize = 0;
#if (PK_GPU_D3D11 != 0)
				if (renderContext.m_API == SRenderContext::D3D11)
				{
					const FD3D11UnorderedAccessView		*view = (FD3D11UnorderedAccessView*)params.m_UAVs[i].GetReference();
					D3D11_UNORDERED_ACCESS_VIEW_DESC	desc;
					view->View->GetDesc(&desc);
					byteSize = desc.Buffer.NumElements * 4;

					RHICmdList.SetUAVParameter(shader, UAV.GetBaseIndex(), params.m_UAVs[i]);
					RHICmdList.SetUAVParameter(shader, UAVRaw.GetBaseIndex(), null);
					SetShaderValue(RHICmdList, shader, RawUAV, 0);
				}
#endif // (PK_GPU_D3D11 != 0)
#if (PK_GPU_D3D12 != 0)
				if (renderContext.m_API == SRenderContext::D3D12)
				{
					const FD3D12UnorderedAccessView			*view = (FD3D12UnorderedAccessView*)params.m_UAVs[i].GetReference();
					const D3D12_UNORDERED_ACCESS_VIEW_DESC	&desc = view->GetDesc();
					byteSize = desc.Buffer.NumElements * 4;

					RHICmdList.SetUAVParameter(shader, UAV.GetBaseIndex(), null);
					RHICmdList.SetUAVParameter(shader, UAVRaw.GetBaseIndex(), params.m_UAVs[i]);
					SetShaderValue(RHICmdList, shader, RawUAV, 1);
				}
#endif // (PK_GPU_D3D12 != 0)

				const u32	numDWordsToClear = (byteSize + 3) / 4;
				const u32	numThreadGroupsX = (numDWordsToClear + 63) / 64;

				SetShaderValue(RHICmdList, shader, ClearBufferCSParams, FUintVector4(0/*Clear value*/, numDWordsToClear, 0, 0));
				//RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, params.m_UAVs[i]);
				RHICmdList.DispatchComputeShader(numThreadGroupsX, 1, 1);
				//RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, params.m_UAVs[i]);

				SetUAVParameter(RHICmdList, shader, UAVRaw, FUnorderedAccessViewRHIRef());
				SetUAVParameter(RHICmdList, shader, UAV, FUnorderedAccessViewRHIRef());
			}
		}
	}

	//----------------------------------------------------------------------------

#	if (ENGINE_MINOR_VERSION < 25)
	bool	FUAVsClearCS::Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ClearBufferCSParams;
		Ar << UAVRaw;
		Ar << UAV;
		Ar << RawUAV;
		return bShaderHasOutdatedParameters;
	}
#	endif //(ENGINE_MINOR_VERSION < 25)
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
		AddDefinesEnumTypeAndMasks(OutEnvironment);
		OutEnvironment.SetDefine(TEXT("PK_BILLBOARDER_CS_OUTPUT_PACK_COLOR_F16"), uint32(PK_BILLBOARDER_CS_OUTPUT_PACK_COLOR_F16));
	}

	//----------------------------------------------------------------------------

	FBillboarderMeshCS::FBillboarderMeshCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
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

#if (ENGINE_MINOR_VERSION >= 25)
		FCSRHIParamRef	shader = RHICmdList.GetBoundComputeShader();
#else
		FCSRHIParamRef	shader = GetComputeShader();
		RHICmdList.SetComputeShader(shader);
#endif //(ENGINE_MINOR_VERSION >= 25)

		uint32	outputMask = 0;
		uint32	inputMask = 0;

		for (u32 i = 0; i < EOutput::_Count; ++i)
		{
			if (IsValidRef(params.m_Outputs[i]))
			{
				outputMask |= (1 << i); // if buffer is provided, always set the mask, even if not used (not bound)
				if (Outputs[i].IsBound())
					RHICmdList.SetUAVParameter(shader, Outputs[i].GetBaseIndex(), params.m_Outputs[i]);
			}
			else if (Outputs[i].IsBound())
				RHICmdList.SetUAVParameter(shader, Outputs[i].GetBaseIndex(), null); // avoids D3D11 warnings !?
		}

		RHICmdList.SetShaderResourceViewParameter(shader, InSimData.GetBaseIndex(), IsValidRef(params.m_InSimData) ? params.m_InSimData : null);
		for (u32 i = 0; i < EInput::_Count; ++i)
		{
			if (params.m_ValidInputs[i])
				inputMask |= (1 << i); // if buffer is provided, always set the mask, even if not used (not bound)

			SetShaderValue(RHICmdList, shader, InputsOffsets[i], params.m_InputsOffsets[i]);
			SetShaderValue(RHICmdList, shader, InputsDefault[i], _Reinterpret<FVector4>(params.m_InputsDefault[i]));
		}

		SetShaderValue(RHICmdList, shader, OutputMask, outputMask);
		SetShaderValue(RHICmdList, shader, InputMask, inputMask);
		SetShaderValue(RHICmdList, shader, InputOffset, params.m_InputOffset);
		SetShaderValue(RHICmdList, shader, OutputVertexOffset, params.m_OutputVertexOffset);
		SetShaderValue(RHICmdList, shader, PositionsScale, params.m_PositionsScale);

		uint32		hasLiveParticleCount = 0;
		if (IsValidRef(params.m_LiveParticleCount))
			hasLiveParticleCount = 1;
		SetShaderValue(RHICmdList, shader, HasLiveParticleCount, hasLiveParticleCount);
		if (IsValidRef(params.m_LiveParticleCount) && LiveParticleCount.IsBound())
			RHICmdList.SetShaderResourceViewParameter(shader, LiveParticleCount.GetBaseIndex(), params.m_LiveParticleCount);

		{
			PK_NAMEDSCOPEDPROFILE("FBillboarderMeshCS::Dispatch");
			const uint32	threadGroupCount = PopcornFX::Mem::Align(params.m_ParticleCount, CS_BB_THREADGROUP_SIZE) / CS_BB_THREADGROUP_SIZE;
			RHICmdList.DispatchComputeShader(threadGroupCount, 1, 1);
		}

		FUAVRHIParamRef			nullUAV = FUAVRHIParamRef();
		FSRVRHIParamRef			nullSRV = FSRVRHIParamRef();
		for (u32 i = 0; i < EOutput::_Count; ++i)
		{
			if (IsValidRef(params.m_Outputs[i]) && Outputs[i].IsBound())
				RHICmdList.SetUAVParameter(shader, Outputs[i].GetBaseIndex(), nullUAV);
		}
		RHICmdList.SetShaderResourceViewParameter(shader, InSimData.GetBaseIndex(), nullSRV);
	}

	//----------------------------------------------------------------------------

#if (ENGINE_MINOR_VERSION < 25)
	bool	FBillboarderMeshCS::Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << OutputMask;
		Ar << InputMask;
		Ar << InputOffset;
		Ar << OutputVertexOffset;
		Ar << PositionsScale;
		for (u32 i = 0; i < EOutput::_Count; ++i)
			Ar << Outputs[i];
		Ar << InSimData;
		for (u32 i = 0; i < EInput::_Count; ++i)
		{
			Ar << InputsOffsets[i];
			Ar << InputsDefault[i];
		}
		Ar << HasLiveParticleCount;
		Ar << LiveParticleCount;
		return bShaderHasOutdatedParameters;
	}
#endif // (ENGINE_MINOR_VERSION < 25)

	//----------------------------------------------------------------------------

	// static
	bool	FCopySizeBufferCS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
	{
		return _IsGpuSupportedOnPlatform(Parameters.Platform);
	}

	//----------------------------------------------------------------------------

	//static
	void	FCopySizeBufferCS::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment)
	{
		Super::ModifyCompilationEnvironment(Parameters, OutEnvironment);
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
#if (ENGINE_MINOR_VERSION >= 25)
		FCSRHIParamRef	shader = RHICmdList.GetBoundComputeShader();
#else
		FCSRHIParamRef	shader = GetComputeShader();
		RHICmdList.SetComputeShader(shader);
#endif //(ENGINE_MINOR_VERSION >= 25)

		SetShaderValue(RHICmdList, shader, InDrawIndirectArgsBufferOffset, params.m_DrawIndirectArgsOffset);
		SetShaderValue(RHICmdList, shader, IndexCountPerInstance, params.m_IsCapsule ? 12 : 6);

		RHICmdList.SetShaderResourceViewParameter(shader, LiveParticleCount.GetBaseIndex(), params.m_LiveParticleCount);
		RHICmdList.SetUAVParameter(shader, DrawIndirectArgsBuffer.GetBaseIndex(), params.m_DrawIndirectArgsBuffer);

		{
			PK_NAMEDSCOPEDPROFILE("FCopySizeBufferCS::Dispatch");
			RHICmdList.DispatchComputeShader(1, 1, 1);
		}

		FSRVRHIParamRef		nullSRV = FSRVRHIParamRef();
		FUAVRHIParamRef		nullUAV = FUAVRHIParamRef();
		RHICmdList.SetUAVParameter(shader, DrawIndirectArgsBuffer.GetBaseIndex(), nullUAV);
		RHICmdList.SetShaderResourceViewParameter(shader, LiveParticleCount.GetBaseIndex(), nullSRV);
	}

	//----------------------------------------------------------------------------

#if (ENGINE_MINOR_VERSION < 25)
	bool	FCopySizeBufferCS::Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << InDrawIndirectArgsBufferOffset;
		Ar << IndexCountPerInstance;
		Ar << DrawIndirectArgsBuffer;
		Ar << LiveParticleCount;
		return bShaderHasOutdatedParameters;
	}
#endif // (ENGINE_MINOR_VERSION < 25)

} // namespace PopcornFXBillboarder

//----------------------------------------------------------------------------

IMPLEMENT_SHADER_TYPE(template<>, PopcornFXBillboarder::FBillboarderBillboardCS<PopcornFXBillboarder::EBillboarderCSBuild::Std>, TEXT(PKUE_GLOBAL_SHADER_PATH("PopcornFXBillboarderBillboardComputeShader")), TEXT("Billboard"), SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>, PopcornFXBillboarder::FBillboarderBillboardCS<PopcornFXBillboarder::EBillboarderCSBuild::VertexPP>, TEXT(PKUE_GLOBAL_SHADER_PATH("PopcornFXBillboarderBillboardComputeShader")), TEXT("Billboard"), SF_Compute);

IMPLEMENT_SHADER_TYPE(, PopcornFXBillboarder::FCopySizeBufferCS, TEXT(PKUE_GLOBAL_SHADER_PATH("PopcornFXCopySizeBufferComputeShader")), TEXT("Copy"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, PopcornFXBillboarder::FCopyStreamCS, TEXT(PKUE_GLOBAL_SHADER_PATH("PopcornFXCopyStreamComputeShader")), TEXT("Copy"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, PopcornFXBillboarder::FBillboarderMeshCS, TEXT(PKUE_GLOBAL_SHADER_PATH("PopcornFXBillboarderMeshComputeShader")), TEXT("Billboard"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, PopcornFXBillboarder::FUAVsClearCS, TEXT(PKUE_GLOBAL_SHADER_PATH("PopcornFXUAVsClearComputeShader")), TEXT("Clear"), SF_Compute);

//----------------------------------------------------------------------------

#endif // (PK_HAS_GPU != 0)
