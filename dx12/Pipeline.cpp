#include "Pipeline.hpp"
#include "helpers/Resources.hpp"
#include "helpers/Log.hpp"
#include <vector>

void Pipeline::configureGraphics(const std::string& name, const Settings& settings, ID3D12Device* device)
{
	const bool needsPixelShader = settings.rtFormat != DXGI_FORMAT_UNKNOWN;

	size_t vertexShaderSize = 0;
	char* vertexShaderData = Resources::loadRawDataFromExternalFile( name + "VertexShader.cso", vertexShaderSize );
	size_t pixelShaderSize = 0;
	char* pixelShaderData = nullptr;
	if ( needsPixelShader ) {
		pixelShaderData = Resources::loadRawDataFromExternalFile(name + "PixelShader.cso", pixelShaderSize);
	}

	{
		D3D12_ROOT_SIGNATURE_DESC1 rootDesc{};
		rootDesc.Flags = (D3D12_ROOT_SIGNATURE_FLAGS)(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
			| ( needsPixelShader ? 0 : D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS) | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS);

		// General layout:
		std::vector<D3D12_ROOT_PARAMETER1> params;
		D3D12_DESCRIPTOR_RANGE1 ranges[3]; //temp storage

		UINT currentCBVRegister = 0;
		UINT currentSRVRegister = 0;

		if (settings.resources & Settings::OBJECT_CBV) {
			params.emplace_back();
			D3D12_ROOT_PARAMETER1& cbv = params.back();
			cbv.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			cbv.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			cbv.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
			cbv.Descriptor.RegisterSpace = 0;
			cbv.Descriptor.ShaderRegister = currentCBVRegister++;
		}
		if (settings.resources & Settings::OBJECT_TABLE) {
			ranges[0].BaseShaderRegister = currentSRVRegister;
			ranges[0].NumDescriptors = settings.resourcesCount;
			ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
			ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			ranges[0].RegisterSpace = 0;
			currentSRVRegister += ranges[0].NumDescriptors;

			params.emplace_back();
			D3D12_ROOT_PARAMETER1& table = params.back();
			table.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			table.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			table.DescriptorTable.NumDescriptorRanges = 1;
			table.DescriptorTable.pDescriptorRanges = &ranges[0];
		}

		if (settings.resources & Settings::ENGINE_CBV) {
			params.emplace_back();
			D3D12_ROOT_PARAMETER1& cbv = params.back();
			cbv.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			cbv.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			cbv.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
			cbv.Descriptor.RegisterSpace = 0;
			cbv.Descriptor.ShaderRegister = currentCBVRegister++;
		}
		
		if (settings.resources & Settings::ENGINE_TABLE) {
			ranges[1].BaseShaderRegister = currentSRVRegister;
			ranges[1].NumDescriptors = 1;
			ranges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
			ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			ranges[1].RegisterSpace = 0;
			currentSRVRegister += ranges[1].NumDescriptors;

			params.emplace_back();
			D3D12_ROOT_PARAMETER1& table = params.back();
			table.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			table.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			table.DescriptorTable.NumDescriptorRanges = 1;
			table.DescriptorTable.pDescriptorRanges = &ranges[1];
		}
		if (settings.resources & (Settings::OBJECT_TABLE | Settings::ENGINE_TABLE)) {
			ranges[2].BaseShaderRegister = 0;
			ranges[2].NumDescriptors = 2;
			ranges[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
			ranges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			ranges[2].RegisterSpace = 0;

			params.emplace_back();
			D3D12_ROOT_PARAMETER1& table = params.back();
			table.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			table.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			table.DescriptorTable.NumDescriptorRanges = 1;
			table.DescriptorTable.pDescriptorRanges = &ranges[2];
		}

		rootDesc.NumParameters = (UINT)params.size();
		rootDesc.pParameters = params.data();
		rootDesc.NumStaticSamplers = 0;
		rootDesc.pStaticSamplers = nullptr;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC vRootDesc{ };
		vRootDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		vRootDesc.Desc_1_1 = rootDesc;

		ID3DBlob* blob{ nullptr };
		ID3DBlob* errorBlob{ nullptr };
		DX_RET(D3D12SerializeVersionedRootSignature(&vRootDesc, &blob, &errorBlob));
		DX_RET(device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&signature)));
	}

	D3D12_INPUT_ELEMENT_DESC attribs[5];
	{
		attribs[0].SemanticName = "POSITION";
		attribs[0].SemanticIndex = 0;
		attribs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attribs[0].InputSlot = 0;
		attribs[0].AlignedByteOffset = offsetof(Vertex, pos);
		attribs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		attribs[0].InstanceDataStepRate = 0;

		if (settings.fullLayout) {
			attribs[1].SemanticName = "NORMAL";
			attribs[1].SemanticIndex = 0;
			attribs[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			attribs[1].InputSlot = 0;
			attribs[1].AlignedByteOffset = offsetof(Vertex, normal);
			attribs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			attribs[1].InstanceDataStepRate = 0;

			attribs[2].SemanticName = "TANGENT";
			attribs[2].SemanticIndex = 0;
			attribs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			attribs[2].InputSlot = 0;
			attribs[2].AlignedByteOffset = offsetof(Vertex, tangent);
			attribs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			attribs[2].InstanceDataStepRate = 0;

			attribs[3].SemanticName = "BINORMAL";
			attribs[3].SemanticIndex = 0;
			attribs[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			attribs[3].InputSlot = 0;
			attribs[3].AlignedByteOffset = offsetof(Vertex, binormal);
			attribs[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			attribs[3].InstanceDataStepRate = 0;

			attribs[4].SemanticName = "TEXCOORD";
			attribs[4].SemanticIndex = 0;
			attribs[4].Format = DXGI_FORMAT_R32G32_FLOAT;
			attribs[4].InputSlot = 0;
			attribs[4].AlignedByteOffset = offsetof(Vertex, texCoord);
			attribs[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			attribs[4].InstanceDataStepRate = 0;
		}
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{ };
	desc.pRootSignature = signature;
	desc.VS.BytecodeLength = vertexShaderSize;
	desc.VS.pShaderBytecode = vertexShaderData;
	desc.PS.BytecodeLength = pixelShaderSize;
	desc.PS.pShaderBytecode = pixelShaderData;
	desc.HS = { nullptr, 0 };
	desc.DS = { nullptr, 0 };
	desc.GS = { nullptr, 0 };
	desc.StreamOutput.NumEntries = 0;

	desc.BlendState.AlphaToCoverageEnable = FALSE;
	desc.BlendState.IndependentBlendEnable = FALSE;
	desc.BlendState.RenderTarget[0].BlendEnable = FALSE;
	desc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
	desc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0b1111;
	desc.SampleMask = 0xffffffff;

	desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	desc.RasterizerState.FrontCounterClockwise = TRUE;
	desc.RasterizerState.DepthBias = 0;
	desc.RasterizerState.DepthBiasClamp = 0.f;
	desc.RasterizerState.SlopeScaledDepthBias = 0.f;
	desc.RasterizerState.DepthClipEnable = TRUE;
	desc.RasterizerState.MultisampleEnable = FALSE;
	desc.RasterizerState.AntialiasedLineEnable = FALSE;
	desc.RasterizerState.ForcedSampleCount = 0;
	desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	desc.DepthStencilState.DepthEnable = TRUE;
	desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	desc.DepthStencilState.StencilEnable = FALSE;

	desc.InputLayout.NumElements = settings.fullLayout ? 5 : 1;
	desc.InputLayout.pInputElementDescs = attribs;

	desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = needsPixelShader ? 1 : 0;
	desc.RTVFormats[0] = settings.rtFormat;
	for (int i = desc.NumRenderTargets; i < 8; ++i) {
		desc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.NodeMask = 0;
	desc.CachedPSO.CachedBlobSizeInBytes = 0;
	desc.CachedPSO.pCachedBlob = nullptr;
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	DX_RET(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline)));

	delete[] vertexShaderData;

	if(pixelShaderData)
		delete[] pixelShaderData;
}

void Pipeline::configureCompute(const std::string& name, ID3D12Device* device)
{
	size_t computeShaderSize = 0;
	char* computeShaderData = Resources::loadRawDataFromExternalFile(name + "ComputeShader.cso", computeShaderSize);
	
	{
		D3D12_ROOT_SIGNATURE_DESC1 rootDesc{};
		rootDesc.Flags = (D3D12_ROOT_SIGNATURE_FLAGS)(D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS);

		// Basic processing layout: one resource table containing one input SRV, one output UAV, no CBV

		std::vector<D3D12_ROOT_PARAMETER1> params;
		D3D12_DESCRIPTOR_RANGE1 ranges[2]; //temp storage

		{
			ranges[0].BaseShaderRegister = 0;
			ranges[0].NumDescriptors = 1;
			ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
			ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			ranges[0].RegisterSpace = 0;

			ranges[1].BaseShaderRegister = 0;
			ranges[1].NumDescriptors = 1;
			ranges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
			ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			ranges[1].RegisterSpace = 0;
	
			params.emplace_back();
			D3D12_ROOT_PARAMETER1& table = params.back();
			table.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			table.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			table.DescriptorTable.NumDescriptorRanges = 2;
			table.DescriptorTable.pDescriptorRanges = &ranges[0];
		}

		rootDesc.NumParameters = (UINT)params.size();
		rootDesc.pParameters = params.data();
		rootDesc.NumStaticSamplers = 0;
		rootDesc.pStaticSamplers = nullptr;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC vRootDesc{ };
		vRootDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		vRootDesc.Desc_1_1 = rootDesc;

		ID3DBlob* blob{ nullptr };
		ID3DBlob* errorBlob{ nullptr };
		DX_RET(D3D12SerializeVersionedRootSignature(&vRootDesc, &blob, &errorBlob));
		DX_RET(device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&signature)));
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC desc{ };
	desc.pRootSignature = signature;
	desc.CS.BytecodeLength = computeShaderSize;
	desc.CS.pShaderBytecode = computeShaderData;
	desc.NodeMask = 0;
	desc.CachedPSO.CachedBlobSizeInBytes = 0;
	desc.CachedPSO.pCachedBlob = nullptr;
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	DX_RET(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline)));

	delete[] computeShaderData;
}
