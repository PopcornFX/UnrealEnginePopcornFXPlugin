//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXGPUSimInterfaces.h"
#include "PopcornFXPlugin.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "RHI.h"
#include "RHIResources.h"

#include <pk_compiler/include/cp_backend.h>

#if (PK_GPU_D3D11 != 0)
#	include <pk_particles/include/Kernels/D3D11/kernel_d3d11.h> // PopcornFX::SBindingContextD3D11
#	if defined(PK_DURANGO)
#		include <d3d11_x.h>
#	else
#		include <d3d11.h>
#	endif // PK_DURANGO
#endif // (PK_GPU_D3D11 != 0)

#if (PK_HAS_GPU != 0)

#	include <pk_particles/include/Updaters/GPU/updater_gpu.h>

namespace	PopcornFXGPU
{
#if (PK_GPU_D3D != 0)

	bool	_LoadSimInterfaceSource(const FString &kernelPath, PopcornFX::CString &outKernelSource)
	{
		const FString	pluginSimInterfacesDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("PopcornFX"))->GetBaseDir(), TEXT("GPUSimInterfaces"));
		const FString	simInterfacePath = pluginSimInterfacesDir / kernelPath;

		FString	simInterfaceSource;
		if (!FFileHelper::LoadFileToString(simInterfaceSource, *simInterfacePath))
		{
			PopcornFX::CLog::Log(PK_ERROR, "Couldn't load sim interface source ('%s')", TCHAR_TO_ANSI(*simInterfacePath));
			return false;
		}
		outKernelSource += TCHAR_TO_ANSI(*simInterfaceSource);
		return true;
	}

	namespace	D3D
	{
#if WITH_EDITOR
		bool	_Declare_Texture2D(PopcornFX::CStringId name, u32 regslot, PopcornFX::CString &out)
		{
			out += PopcornFX::CString::Format("Texture2D			%s : register(t%d);\n", name.ToStringData(), regslot);
			return true;
		}

		bool	_Declare_ViewConstantBuffer(PopcornFX::CStringId name, u32 regslot, PopcornFX::CString &out)
		{
			out += PopcornFX::CString::Format(	"cbuffer		%s : register(b%d)\n"
												"{\n"
												"	float4		InvDeviceZToWorldZTransform;\n"
												"	float4x4	TranslatedWorldToClip;\n"
												"	float4x4	ScreenToWorld;\n"
												"	float4		PreViewTranslation;\n"
												"	float4		ScreenPositionScaleBias;\n"
												"};\n", name.ToStringData(), regslot);
			return true;
		}

		bool	_Declare_Point_Sampler(PopcornFX::CStringId name, u32 regslot, PopcornFX::CString &out)
		{
			out += PopcornFX::CString::Format(	"SamplerState		%s : register(s%d)\n"
												"{\n"
												"	Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;\n"
												"};\n", name.ToStringData(), regslot);
			return true;
		}

		// Scene intersect with scene depth map
		bool	EmitBuiltin_IntersectScene(PopcornFX::CLinkerGPU::SExternalFunctionEmitArgs &args)
		{
			PopcornFX::CLinkerGPU::SExternalFunctionEmitArgs::SDeclInput	inputSceneDepth(PopcornFX::CStringId("UE_SceneDepth"), PopcornFX::CLinkerGPU::SExternalFunctionEmitArgs::SDeclInput::ShaderResource, _Declare_Texture2D);
			PopcornFX::CLinkerGPU::SExternalFunctionEmitArgs::SDeclInput	inputSceneNormal(PopcornFX::CStringId("UE_SceneNormal"), PopcornFX::CLinkerGPU::SExternalFunctionEmitArgs::SDeclInput::ShaderResource, _Declare_Texture2D);
			PopcornFX::CLinkerGPU::SExternalFunctionEmitArgs::SDeclInput	inputViewConstantBuffer(PopcornFX::CStringId("UE_ViewConstantBuffer"), PopcornFX::CLinkerGPU::SExternalFunctionEmitArgs::SDeclInput::ConstantBuffer, _Declare_ViewConstantBuffer);
			PopcornFX::CLinkerGPU::SExternalFunctionEmitArgs::SDeclInput	inputSampler(PopcornFX::CStringId("UE_SceneTexturesSampler"), PopcornFX::CLinkerGPU::SExternalFunctionEmitArgs::SDeclInput::Sampler, _Declare_Point_Sampler);

			if (!PK_VERIFY(args.m_ExternalShaderInput.PushBack(inputSceneDepth).Valid()) ||
				!PK_VERIFY(args.m_ExternalShaderInput.PushBack(inputSceneNormal).Valid()) ||
				!PK_VERIFY(args.m_ExternalShaderInput.PushBack(inputViewConstantBuffer).Valid()) ||
				!PK_VERIFY(args.m_ExternalShaderInput.PushBack(inputSampler).Valid()))
				return false;

			return _LoadSimInterfaceSource("PopcornFXGPUSimInterface_IntersectScene.d3d", args.m_DstKernelSource);
		}
#endif // WITH_EDITOR

		bool	_Bind_RHITexture(FRHITexture2D *texRHI, const PopcornFX::SLinkGPUContext &context)
		{
			if (texRHI == null)
				return false;

			if (context.m_ContextType == PopcornFX::EGPUContext::ContextD3D12)
			{
#if	(PK_GPU_D3D12 != 0)
#endif	// (PK_GPU_D3D12 != 0)
			}
			else if (context.m_ContextType == PopcornFX::EGPUContext::ContextD3D11)
			{
#if	(PK_GPU_D3D11 != 0)
				const PopcornFX::SBindingContextD3D11	&d3d11Context = context.ToD3D11();
				ID3D11ShaderResourceView				*gpuTextureSRV = static_cast<ID3D11ShaderResourceView*>(texRHI->GetNativeShaderResourceView());

				d3d11Context.m_DeviceContext->CSSetShaderResources(context.m_Location, 1, &gpuTextureSRV);
#endif	// (PK_GPU_D3D11 != 0)
			}
			return true;
		}

		bool	Bind_IntersectScene_SceneDepth(PopcornFX::CStringId mangledName, const PopcornFX::SLinkGPUContext &context)
		{
			PK_ASSERT(mangledName.ToStringView() == "UE_SceneDepth");
			(void)mangledName;

#if 0
			return _Bind_RHITexture(FPopcornFXPlugin::Get().SceneDepthTexture(), context);
#else
			return false;
#endif
		}

		bool	Bind_IntersectScene_SceneNormal(PopcornFX::CStringId mangledName, const PopcornFX::SLinkGPUContext &context)
		{
			PK_ASSERT(mangledName.ToStringView() == "UE_SceneNormal");
			(void)mangledName;

#if 0
			return _Bind_RHITexture(FPopcornFXPlugin::Get().SceneNormalTexture(), context);
#else
			return false;
#endif
		}

		bool	Bind_IntersectScene_ViewUniformBuffer(PopcornFX::CStringId mangledName, const PopcornFX::SLinkGPUContext &context)
		{
			PK_ASSERT(mangledName.ToStringView() == "UE_ViewConstantBuffer");
			(void)mangledName;

#if 0
			const SUEViewUniformData	&viewUniformData = FPopcornFXPlugin::Get().ViewUniformData();

			if (context.m_ContextType == PopcornFX::EGPUContext::ContextD3D12)
			{
#if	(PK_GPU_D3D12 != 0)
#endif	// (PK_GPU_D3D12 != 0)
			}
			else if (context.m_ContextType == PopcornFX::EGPUContext::ContextD3D11)
			{
#if	(PK_GPU_D3D11 != 0)
				const PopcornFX::SBindingContextD3D11	&d3d11Context = context.ToD3D11();

				PopcornFX::SParticleStreamBuffer_D3D11	&viewConstantBuffer = FPopcornFXPlugin::Get().ViewConstantBuffer_D3D11();
				if (viewConstantBuffer.Empty())
				{
					ID3D11Device	*device = null;
					d3d11Context.m_DeviceContext->GetDevice(&device);

					D3D11_BUFFER_DESC	bufDesc = {};
					bufDesc.Usage = D3D11_USAGE_DEFAULT;
					bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
					bufDesc.CPUAccessFlags = 0;
					bufDesc.MiscFlags = 0;
					bufDesc.ByteWidth = sizeof(SUEViewUniformData);

					D3D11_SUBRESOURCE_DATA	initialData = {};
					initialData.pSysMem = &viewUniformData;

					return device->CreateBuffer(&bufDesc, &initialData, &viewConstantBuffer.m_Buffer) == S_OK;
				}
				else
					d3d11Context.m_DeviceContext->UpdateSubresource(viewConstantBuffer.m_Buffer, 0, null, &viewUniformData, 0, 0);

				d3d11Context.m_DeviceContext->CSSetConstantBuffers(context.m_Location, 1, &viewConstantBuffer.m_Buffer);
#endif	// (PK_GPU_D3D11 != 0)
			}
			return true;
#else
			return false;
#endif
		}

	} // namespace D3D

#endif // (PK_GPU_D3D != 0)

} // namespace PopcornFXGPU

#endif // (PK_HAS_GPU != 0)
