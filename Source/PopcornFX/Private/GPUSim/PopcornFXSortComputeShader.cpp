//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXSortComputeShader.h"

#include "GPUSim/PopcornFXGPUSim.h"
#include "Render/PopcornFXShaderUtils.h"

#include "ShaderParameterUtils.h"
#include "SceneUtils.h"

#if (PK_HAS_GPU != 0)

//----------------------------------------------------------------------------

#define CS_SORT_THREADGROUP_SIZE	256

//----------------------------------------------------------------------------
//
// FPopcornFXSortComputeShader_GenKeys
//
//----------------------------------------------------------------------------

class FPopcornFXSortComputeShader_GenKeys : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPopcornFXSortComputeShader_GenKeys, Global)

public:
	typedef FGlobalShader		Super;

	static bool				ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
	{
		return _IsGpuSupportedOnPlatform(Parameters.Platform);
	}
	static void				ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment)
	{
		Super::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("DO_GenKeysCS"), 1);
		OutEnvironment.SetDefine(TEXT("PK_GPU_THREADGROUP_SIZE"), CS_SORT_THREADGROUP_SIZE);
		//OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	FPopcornFXSortComputeShader_GenKeys(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: Super(Initializer)
	{
		TotalCount.Bind(Initializer.ParameterMap, TEXT("TotalCount"));
		SortOrigin.Bind(Initializer.ParameterMap, TEXT("SortOrigin"));
		IndexStart.Bind(Initializer.ParameterMap, TEXT("IndexStart"));
		IndexStep.Bind(Initializer.ParameterMap, TEXT("IndexStep"));
		InputOffset.Bind(Initializer.ParameterMap, TEXT("InputOffset"));
		OutputOffset.Bind(Initializer.ParameterMap, TEXT("OutputOffset"));
		InPositions.Bind(Initializer.ParameterMap, TEXT("InPositions"));
		InPositionsOffset.Bind(Initializer.ParameterMap, TEXT("InPositionsOffset"));
		OutKeys.Bind(Initializer.ParameterMap, TEXT("OutKeys"));
		OutValues.Bind(Initializer.ParameterMap, TEXT("OutValues"));
	}

	FPopcornFXSortComputeShader_GenKeys() { }

public:
	LAYOUT_FIELD(FShaderParameter, TotalCount);
	LAYOUT_FIELD(FShaderParameter, SortOrigin);
	LAYOUT_FIELD(FShaderParameter, IndexStart);
	LAYOUT_FIELD(FShaderParameter, IndexStep);
	LAYOUT_FIELD(FShaderParameter, InputOffset);
	LAYOUT_FIELD(FShaderParameter, OutputOffset);
	LAYOUT_FIELD(FShaderResourceParameter, InPositions);
	LAYOUT_FIELD(FShaderParameter, InPositionsOffset);
	LAYOUT_FIELD(FShaderResourceParameter, OutKeys);
	LAYOUT_FIELD(FShaderResourceParameter, OutValues);
};


IMPLEMENT_SHADER_TYPE(, FPopcornFXSortComputeShader_GenKeys, TEXT(PKUE_GLOBAL_SHADER_PATH("PopcornFXSortComputeShader")), TEXT("GenKeysCS"), SF_Compute);

//----------------------------------------------------------------------------
//
// FPopcornFXSortComputeShader_Sort_UpSweep
//
//----------------------------------------------------------------------------

class FPopcornFXSortComputeShader_Sort_UpSweep : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPopcornFXSortComputeShader_Sort_UpSweep, Global)
public:
	typedef FGlobalShader	Super;

	static bool				ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
	{
		return _IsGpuSupportedOnPlatform(Parameters.Platform);
	}
	static void				ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment)
	{
		Super::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("DO_SortUpSweep"), 1);
		OutEnvironment.SetDefine(TEXT("PK_GPU_THREADGROUP_SIZE"), CS_SORT_THREADGROUP_SIZE);
		//OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	FPopcornFXSortComputeShader_Sort_UpSweep(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: Super(Initializer)
	{
		KeyBitOffset.Bind(Initializer.ParameterMap, TEXT("KeyBitOffset"));
		InKeys.Bind(Initializer.ParameterMap, TEXT("InKeys"));
		OutOffsets.Bind(Initializer.ParameterMap, TEXT("OutOffsets"));
	}
	FPopcornFXSortComputeShader_Sort_UpSweep() {}
public:
	LAYOUT_FIELD(FShaderParameter, KeyBitOffset);
	LAYOUT_FIELD(FShaderResourceParameter, InKeys);
	LAYOUT_FIELD(FShaderResourceParameter, OutOffsets);
};

IMPLEMENT_SHADER_TYPE(, FPopcornFXSortComputeShader_Sort_UpSweep, TEXT(PKUE_GLOBAL_SHADER_PATH("PopcornFXSortComputeShader")), TEXT("SortUpSweep"), SF_Compute);

//----------------------------------------------------------------------------
//
// FPopcornFXSortComputeShader_Sort_UpSweepOffsets
//
//----------------------------------------------------------------------------

class FPopcornFXSortComputeShader_Sort_UpSweepOffsets : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPopcornFXSortComputeShader_Sort_UpSweepOffsets, Global)
public:
	typedef FGlobalShader	Super;

	static bool				ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
	{
		return _IsGpuSupportedOnPlatform(Parameters.Platform);
	}
	static void				ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment)
	{
		Super::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("DO_SortUpSweepOffsets"), 1);
		OutEnvironment.SetDefine(TEXT("PK_GPU_THREADGROUP_SIZE"), CS_SORT_THREADGROUP_SIZE);
		//OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	FPopcornFXSortComputeShader_Sort_UpSweepOffsets(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: Super(Initializer)
	{
		GroupCount.Bind(Initializer.ParameterMap, TEXT("GroupCount"));
		InOutOffsets.Bind(Initializer.ParameterMap, TEXT("InOutOffsets"));
	}
	FPopcornFXSortComputeShader_Sort_UpSweepOffsets() {}
public:
	LAYOUT_FIELD(FShaderParameter, GroupCount);
	LAYOUT_FIELD(FShaderResourceParameter, InOutOffsets);
};

IMPLEMENT_SHADER_TYPE(, FPopcornFXSortComputeShader_Sort_UpSweepOffsets, TEXT(PKUE_GLOBAL_SHADER_PATH("PopcornFXSortComputeShader")), TEXT("SortUpSweepOffsets"), SF_Compute);

//----------------------------------------------------------------------------
//
// FPopcornFXSortComputeShader_Sort_DownSweep
//
//----------------------------------------------------------------------------

class FPopcornFXSortComputeShader_Sort_DownSweep : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPopcornFXSortComputeShader_Sort_DownSweep, Global)
public:
	typedef FGlobalShader	Super;

	static bool				ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters)
	{
		return _IsGpuSupportedOnPlatform(Parameters.Platform);
	}
	static void				ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters &Parameters, FShaderCompilerEnvironment &OutEnvironment)
	{
		Super::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("DO_SortDownSweep"), 1);
		OutEnvironment.SetDefine(TEXT("PK_GPU_THREADGROUP_SIZE"), CS_SORT_THREADGROUP_SIZE);
	}

	FPopcornFXSortComputeShader_Sort_DownSweep(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: Super(Initializer)
	{
		KeyBitOffset.Bind(Initializer.ParameterMap, TEXT("KeyBitOffset"));
		GroupCount.Bind(Initializer.ParameterMap, TEXT("GroupCount"));
		InOffsets.Bind(Initializer.ParameterMap, TEXT("InOffsets"));
		InKeys.Bind(Initializer.ParameterMap, TEXT("InKeys"));
		InValues.Bind(Initializer.ParameterMap, TEXT("InValues"));
		OutKeys.Bind(Initializer.ParameterMap, TEXT("OutKeys"));
		OutValues.Bind(Initializer.ParameterMap, TEXT("OutValues"));
	}
	FPopcornFXSortComputeShader_Sort_DownSweep() {}
public:
	LAYOUT_FIELD(FShaderParameter, KeyBitOffset);
	LAYOUT_FIELD(FShaderParameter, GroupCount);
	LAYOUT_FIELD(FShaderResourceParameter, InOffsets);
	LAYOUT_FIELD(FShaderResourceParameter, InKeys);
	LAYOUT_FIELD(FShaderResourceParameter, InValues);
	LAYOUT_FIELD(FShaderResourceParameter, OutKeys);
	LAYOUT_FIELD(FShaderResourceParameter, OutValues);
};

IMPLEMENT_SHADER_TYPE(, FPopcornFXSortComputeShader_Sort_DownSweep, TEXT(PKUE_GLOBAL_SHADER_PATH("PopcornFXSortComputeShader")), TEXT("SortDownSweep"), SF_Compute);

//----------------------------------------------------------------------------
//
// FPopcornFXSortComputeShader_Sorter
//
//----------------------------------------------------------------------------

void	FPopcornFXSortComputeShader_Sorter::Clear()
{
	m_TotalCount = 0;
	m_CurrBuff = 0;
	m_CurrGenOutputOffset = 0;
	for (u32 i = 0; i < 2; ++i)
	{
		m_Keys[i].Clear();
		m_Values[i].Clear();
	}
	m_Cache.Clear();
}

//----------------------------------------------------------------------------

bool	FPopcornFXSortComputeShader_Sorter::Prepare(CVertexBufferPool &vbpool, u32 totalCount)
{
	m_CurrBuff = 0;
	m_CurrGenOutputOffset = 0;

	m_TotalCount = 0;
	const u32		alignedCount = PopcornFX::Mem::Align<CS_SORT_THREADGROUP_SIZE>(totalCount);
	const u32		cacheCount = PopcornFX::Mem::Align<CS_SORT_THREADGROUP_SIZE>(alignedCount / CS_SORT_THREADGROUP_SIZE);

	for (u32 i = 0; i < 2; ++i)
	{
		if (!vbpool.AllocateIf(true, m_Keys[i], alignedCount, sizeof(u32)))
			return false;
		if (!vbpool.AllocateIf(true, m_Values[i], alignedCount, sizeof(u32)))
			return false;
	}
	if (!vbpool.AllocateIf(true, m_Cache, cacheCount, sizeof(u32)))
		return false;
	m_TotalCount = totalCount;
	return true;
}

//----------------------------------------------------------------------------

void	FPopcornFXSortComputeShader_Sorter::DispatchGenIndiceBatch(FRHICommandList& RHICmdList, const FPopcornFXSortComputeShader_GenKeys_Params &params)
{
	PK_ASSERT(Ready());

	SCOPED_DRAW_EVENT(RHICmdList, PopcornFXSortComputeShader_Sorter_GenIndices);

	const ERHIFeatureLevel::Type							featureLevel = ERHIFeatureLevel::SM5; // @FIXME, true feature level ?
	TShaderMapRef< FPopcornFXSortComputeShader_GenKeys >	genKeys(GetGlobalShaderMap(featureLevel));

	FCSRHIParamRef			shader = genKeys.GetComputeShader();

	PK_ASSERT(params.m_Count > 0);

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3)
	SetComputePipelineState(RHICmdList, shader);
#else
	RHICmdList.SetComputeShader(shader);
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3)

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	FRHIBatchedShaderParameters	&batchedParameters = RHICmdList.GetScratchShaderParameters();
#endif

	if (PK_VERIFY(genKeys->InPositions.IsBound()) &&
		PK_VERIFY(IsValidRef(params.m_InputPositions)))
	{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		SetSRVParameter(batchedParameters, genKeys->InPositions, params.m_InputPositions);
#else
		RHICmdList.SetShaderResourceViewParameter(shader, genKeys->InPositions.GetBaseIndex(), params.m_InputPositions);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	}

	if (PK_VERIFY(genKeys->OutKeys.IsBound()))
	{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		SetUAVParameter(batchedParameters, genKeys->OutKeys, m_Keys[m_CurrBuff]->UAV());
#else
		RHICmdList.SetUAVParameter(shader, genKeys->OutKeys.GetBaseIndex(), m_Keys[m_CurrBuff]->UAV());
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	}

	if (PK_VERIFY(genKeys->OutValues.IsBound()))
	{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		SetUAVParameter(batchedParameters, genKeys->OutValues, m_Values[m_CurrBuff]->UAV());
#else
		RHICmdList.SetUAVParameter(shader, genKeys->OutValues.GetBaseIndex(), m_Values[m_CurrBuff]->UAV());
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	}

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	SetShaderValue(batchedParameters, genKeys->InPositionsOffset, params.m_InputPositionsOffset);

	SetShaderValue(batchedParameters, genKeys->TotalCount, params.m_Count);
	SetShaderValue(batchedParameters, genKeys->SortOrigin, ToUE(params.m_SortOrigin));
	SetShaderValue(batchedParameters, genKeys->IndexStart, params.m_IndexStart);
	SetShaderValue(batchedParameters, genKeys->IndexStep, params.m_IndexStep);
	SetShaderValue(batchedParameters, genKeys->InputOffset, params.m_InputOffset);

	SetShaderValue(batchedParameters, genKeys->OutputOffset, m_CurrGenOutputOffset);
#else
	SetShaderValue(RHICmdList, shader, genKeys->InPositionsOffset, params.m_InputPositionsOffset);

	SetShaderValue(RHICmdList, shader, genKeys->TotalCount, params.m_Count);
	SetShaderValue(RHICmdList, shader, genKeys->SortOrigin, ToUE(params.m_SortOrigin));
	SetShaderValue(RHICmdList, shader, genKeys->IndexStart, params.m_IndexStart);
	SetShaderValue(RHICmdList, shader, genKeys->IndexStep, params.m_IndexStep);
	SetShaderValue(RHICmdList, shader, genKeys->InputOffset, params.m_InputOffset);

	SetShaderValue(RHICmdList, shader, genKeys->OutputOffset, m_CurrGenOutputOffset);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	m_CurrGenOutputOffset += params.m_Count;
	PK_ASSERT(m_CurrGenOutputOffset <= m_TotalCount);

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	RHICmdList.SetBatchedShaderParameters(shader, batchedParameters);
#endif

	{
		const uint32	threadGroupCount = PopcornFX::Mem::Align(params.m_Count, CS_SORT_THREADGROUP_SIZE) / CS_SORT_THREADGROUP_SIZE;
		RHICmdList.DispatchComputeShader(threadGroupCount, 1, 1);
	}

	FUAVRHIParamRef		nullUAV = FUAVRHIParamRef();
	FSRVRHIParamRef		nullSRV = FSRVRHIParamRef();

	if (genKeys->OutValues.IsBound())
	{
#if (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)
		RHICmdList.SetUAVParameter(shader, genKeys->OutValues.GetBaseIndex(), nullUAV);
#endif // (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)
	}

	if (genKeys->OutKeys.IsBound())
	{
#if (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)
		RHICmdList.SetUAVParameter(shader, genKeys->OutKeys.GetBaseIndex(), nullUAV);
#endif // (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)
	}

	if (genKeys->InPositions.IsBound())
	{
#if (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)
		RHICmdList.SetShaderResourceViewParameter(shader, genKeys->InPositions.GetBaseIndex(), nullSRV);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXSortComputeShader_Sorter::DispatchSort(FRHICommandList& RHICmdList)
{
	PK_ASSERT(Ready());

	SCOPED_DRAW_EVENT(RHICmdList, PopcornFXSortComputeShader_Sorter_Sort);

	PK_ASSERT(m_CurrGenOutputOffset == m_TotalCount);

	const u32		totalAlignedCount = PopcornFX::Mem::Align<CS_SORT_THREADGROUP_SIZE>(m_TotalCount);

	const ERHIFeatureLevel::Type	featureLevel = ERHIFeatureLevel::SM5; // @FIXME, true feature level ?

	TShaderMapRef< FPopcornFXSortComputeShader_Sort_UpSweep >			upSweep(GetGlobalShaderMap(featureLevel));
	TShaderMapRef< FPopcornFXSortComputeShader_Sort_UpSweepOffsets >	upSweepOffsets(GetGlobalShaderMap(featureLevel));
	TShaderMapRef< FPopcornFXSortComputeShader_Sort_DownSweep >			downSweep(GetGlobalShaderMap(featureLevel));

	FCSRHIParamRef	upSweepShader = upSweep.GetComputeShader();
	FCSRHIParamRef	upSweepOffsetsShader = upSweepOffsets.GetComputeShader();
	FCSRHIParamRef	downSweepShader = downSweep.GetComputeShader();

	const uint32		threadGroupCount = totalAlignedCount / CS_SORT_THREADGROUP_SIZE;

	FUAVRHIParamRef		nullUAV = FUAVRHIParamRef();
	FSRVRHIParamRef		nullSRV = FSRVRHIParamRef();

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	FRHIBatchedShaderParameters	&batchedParameters = RHICmdList.GetScratchShaderParameters();
#endif

	for (u32 bitOffset = 0; bitOffset < 16; bitOffset += 1)
	{
		//----------------------------------------------------------------------------
		// Up Sweep

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		SetComputePipelineState(RHICmdList, upSweepShader);
#else
		RHICmdList.SetComputeShader(upSweepShader);
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)

		if (PK_VERIFY(upSweep->InKeys.IsBound()))
		{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
			SetSRVParameter(batchedParameters, upSweep->InKeys, m_Keys[m_CurrBuff]->SRV());
#else
			RHICmdList.SetShaderResourceViewParameter(upSweepShader, upSweep->InKeys.GetBaseIndex(), m_Keys[m_CurrBuff]->SRV());
#endif (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		}
		if (PK_VERIFY(upSweep->OutOffsets.IsBound()))
		{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
			SetUAVParameter(batchedParameters, upSweep->OutOffsets, m_Cache->UAV());
#else
			RHICmdList.SetUAVParameter(upSweepShader, upSweep->OutOffsets.GetBaseIndex(), m_Cache->UAV());
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		}

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		SetShaderValue(batchedParameters, upSweep->KeyBitOffset, bitOffset);
#else
		SetShaderValue(RHICmdList, upSweepShader, upSweep->KeyBitOffset, bitOffset);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		RHICmdList.SetBatchedShaderParameters(upSweepShader, batchedParameters);
#endif

		{
			SCOPED_DRAW_EVENT(RHICmdList, PopcornFXSortComputeShader_Sorter_Sort_UpSweep);
			RHICmdList.DispatchComputeShader(threadGroupCount, 1, 1);
		}

#if (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)
		if (upSweep->InKeys.IsBound())
			RHICmdList.SetShaderResourceViewParameter(upSweepShader, upSweep->InKeys.GetBaseIndex(), nullSRV);
		if (upSweep->OutOffsets.IsBound())
		{
			//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, m_Cache->UAV());
			RHICmdList.SetUAVParameter(upSweepShader, upSweep->OutOffsets.GetBaseIndex(), nullUAV);
		}
#endif // (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)

		//----------------------------------------------------------------------------
		// Up Sweep Offsets
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		SetComputePipelineState(RHICmdList, upSweepOffsetsShader);
#else
		RHICmdList.SetComputeShader(upSweepOffsetsShader);
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)

		if (PK_VERIFY(upSweepOffsets->InOutOffsets.IsBound()))
		{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
			SetUAVParameter(batchedParameters, upSweepOffsets->InOutOffsets, m_Cache->UAV());
#else
			RHICmdList.SetUAVParameter(upSweepOffsetsShader, upSweepOffsets->InOutOffsets.GetBaseIndex(), m_Cache->UAV());
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		}

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		SetShaderValue(batchedParameters, upSweepOffsets->GroupCount, threadGroupCount);
#else
		SetShaderValue(RHICmdList, upSweepOffsetsShader, upSweepOffsets->GroupCount, threadGroupCount);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		RHICmdList.SetBatchedShaderParameters(upSweepOffsetsShader, batchedParameters);
#endif

		{
			SCOPED_DRAW_EVENT(RHICmdList, PopcornFXSortComputeShader_Sorter_Sort_UpSweepOffsets);
			RHICmdList.DispatchComputeShader(1, 1, 1);
		}

#if (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)
		if (upSweepOffsets->InOutOffsets.IsBound())
		{
			//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, m_Cache->UAV());
			RHICmdList.SetUAVParameter(upSweepOffsetsShader, upSweepOffsets->InOutOffsets.GetBaseIndex(), nullUAV);
		}
#endif // (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)

		//----------------------------------------------------------------------------
		// Down Sweep
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		SetComputePipelineState(RHICmdList, downSweepShader);
#else
		RHICmdList.SetComputeShader(downSweepShader);
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)

		if (PK_VERIFY(downSweep->InOffsets.IsBound()))
		{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
			SetSRVParameter(batchedParameters, downSweep->InOffsets, m_Cache->SRV());
#else
			RHICmdList.SetShaderResourceViewParameter(downSweepShader, downSweep->InOffsets.GetBaseIndex(), m_Cache->SRV());
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		}

		if (PK_VERIFY(downSweep->InKeys.IsBound()))
		{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
			SetSRVParameter(batchedParameters, downSweep->InKeys, m_Keys[m_CurrBuff]->SRV());
#else
			RHICmdList.SetShaderResourceViewParameter(downSweepShader, downSweep->InKeys.GetBaseIndex(), m_Keys[m_CurrBuff]->SRV());
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		}
		if (PK_VERIFY(downSweep->InValues.IsBound()))
		{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
			SetSRVParameter(batchedParameters, downSweep->InValues, m_Values[m_CurrBuff]->SRV());
#else
			RHICmdList.SetShaderResourceViewParameter(downSweepShader, downSweep->InValues.GetBaseIndex(), m_Values[m_CurrBuff]->SRV());
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		}

		m_CurrBuff = (m_CurrBuff + 1) % PK_ARRAY_COUNT(m_Keys);

		if (PK_VERIFY(downSweep->OutKeys.IsBound()))
		{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
			SetUAVParameter(batchedParameters, downSweep->OutKeys, m_Keys[m_CurrBuff]->UAV());
#else
			RHICmdList.SetUAVParameter(downSweepShader, downSweep->OutKeys.GetBaseIndex(), m_Keys[m_CurrBuff]->UAV());
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		}
		if (PK_VERIFY(downSweep->OutValues.IsBound()))
		{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
			SetUAVParameter(batchedParameters, downSweep->OutValues, m_Values[m_CurrBuff]->UAV());
#else
			RHICmdList.SetUAVParameter(downSweepShader, downSweep->OutValues.GetBaseIndex(), m_Values[m_CurrBuff]->UAV());
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		}

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		SetShaderValue(batchedParameters, downSweep->KeyBitOffset, bitOffset);
		SetShaderValue(batchedParameters, downSweep->GroupCount, threadGroupCount);
#else
		SetShaderValue(RHICmdList, downSweepShader, downSweep->KeyBitOffset, bitOffset);
		SetShaderValue(RHICmdList, downSweepShader, downSweep->GroupCount, threadGroupCount);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		RHICmdList.SetBatchedShaderParameters(downSweepShader, batchedParameters);
#endif

		{
			SCOPED_DRAW_EVENT(RHICmdList, PopcornFXSortComputeShader_Sorter_Sort_DownSweep);
			RHICmdList.DispatchComputeShader(threadGroupCount, 1, 1);
		}

#if (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)
		if (downSweep->InOffsets.IsBound())
			RHICmdList.SetShaderResourceViewParameter(downSweepShader, downSweep->InOffsets.GetBaseIndex(), nullSRV);
		if (downSweep->InKeys.IsBound())
			RHICmdList.SetShaderResourceViewParameter(downSweepShader, downSweep->InKeys.GetBaseIndex(), nullSRV);
		if (downSweep->InValues.IsBound())
			RHICmdList.SetShaderResourceViewParameter(downSweepShader, downSweep->InValues.GetBaseIndex(), nullSRV);
		if (downSweep->OutKeys.IsBound())
		{
			//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, m_Keys[m_CurrBuff]->UAV());
			RHICmdList.SetUAVParameter(downSweepShader, downSweep->OutKeys.GetBaseIndex(), nullUAV);
		}
		if (downSweep->OutValues.IsBound())
		{
			//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, m_Values[m_CurrBuff]->UAV());
			RHICmdList.SetUAVParameter(downSweepShader, downSweep->OutValues.GetBaseIndex(), nullUAV);
		}
#endif // (ENGINE_MAJOR_VERSION != 5) || (ENGINE_MINOR_VERSION < 3)
	}
}

//----------------------------------------------------------------------------

const FShaderResourceViewRHIRef		&FPopcornFXSortComputeShader_Sorter::SortedValuesSRV()
{
	return m_Values[m_CurrBuff]->SRV();
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

#endif //(PK_HAS_GPU != 0)
