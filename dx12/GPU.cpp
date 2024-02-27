#include "GPU.hpp"
#include "helpers/Log.hpp"

#include <vector>
#include <algorithm>


IDXGIAdapter1* GPU::retrieveBestAdapter(IDXGIFactory2* factory) {
	
	struct CandidateAdapter
	{
		SIZE_T memory;
		UINT index;
	};
	std::vector<CandidateAdapter> candidates;
	UINT adapterIndex = 0;
	IDXGIAdapter1* adapter{ nullptr };

	while (factory->EnumAdapters1(adapterIndex, &adapter) == S_OK)
	{
		DXGI_ADAPTER_DESC1 desc{};
		DX_RET(adapter->GetDesc1(&desc));
		if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
		{
			// This is a hardware adapter, pick it.
			candidates.push_back({ desc.DedicatedVideoMemory, adapterIndex });
		}
		adapter->Release();
		++adapterIndex;
	}
	std::sort(candidates.begin(), candidates.end(), [](const CandidateAdapter& a, const CandidateAdapter& b)
		{
			return a.memory > b.memory;
		});

	if (candidates.empty())
		return nullptr;

	DX_RET(factory->EnumAdapters1(candidates[0].index, &adapter));
	return adapter;
}

void GPU::transitionResource(ID3D12GraphicsCommandList* commandList, ID3D12Resource* rsc, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, UINT subresource)
{
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = rsc;
	barrier.Transition.Subresource = subresource;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	commandList->ResourceBarrier( 1, &barrier );
}

ID3D12Resource* GPU::createAndUploadBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::vector<ID3D12Resource*>& resourcePool, size_t size, char* data)
{
	// create buffers
	D3D12_HEAP_PROPERTIES uploadHeapDesc{};
	uploadHeapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_HEAP_PROPERTIES heapDesc{};
	heapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC rscDesc{};
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rscDesc.Width = size;
	rscDesc.Height = 1;
	rscDesc.DepthOrArraySize = 1;
	rscDesc.MipLevels = 1;
	rscDesc.Format = DXGI_FORMAT_UNKNOWN;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		
	// Intermediate buffer
	ID3D12Resource* uploadBuffer{ nullptr };
	DX_RET( device->CreateCommittedResource( &uploadHeapDesc, D3D12_HEAP_FLAG_NONE, &rscDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS( &uploadBuffer ) ) );
	resourcePool.push_back( uploadBuffer );
		
	// Final buffer
	ID3D12Resource* finalBuffer{ nullptr };
	DX_RET(device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAG_NONE, &rscDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&finalBuffer )));
		
	BYTE* uploadData{ nullptr };
	DX_RET(uploadBuffer->Map(0, nullptr, (void**)&uploadData));
	memcpy(uploadData, data, size );
	uploadBuffer->Unmap(0, nullptr);

	commandList->CopyBufferRegion(finalBuffer, 0, uploadBuffer, 0, size );
	return finalBuffer;
}

ID3D12Resource* GPU::createTexture(ID3D12Device* device, DXGI_FORMAT format, UINT w, UINT h, UINT d, UINT mip, D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_FLAGS flags)
{
	D3D12_HEAP_PROPERTIES heapDesc{};
	heapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC rscDesc{};
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Width = w;
	rscDesc.Height = h;
	rscDesc.DepthOrArraySize = d;
	rscDesc.MipLevels = mip;
	rscDesc.Format = format;
	rscDesc.Alignment = 0;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = flags;

	D3D12_CLEAR_VALUE clearValue{};
	D3D12_CLEAR_VALUE* activeClearValue{nullptr};

	if(flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;
		activeClearValue = &clearValue;
	}
	if( flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET )
	{
		clearValue.Format = format;
		clearValue.Color[ 0 ] = 1.0f;
		clearValue.Color[ 1 ] = 1.0f;
		clearValue.Color[ 2 ] = 1.0f;
		clearValue.Color[ 3 ] = 1.0f;
		activeClearValue = &clearValue;
	}

	ID3D12Resource* texture{nullptr};
	DX_RET(device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAG_NONE, &rscDesc, initialState, activeClearValue, IID_PPV_ARGS(&texture)));
	return texture;
}

void GPU::uploadImageToSubresource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DescriptorAllocator& descriptorAllocator, std::vector<ID3D12Resource*>& rscPool, ID3D12Resource* texture, const Image& img, UINT subresourceIndex)
{
	D3D12_RESOURCE_DESC realDesc = texture->GetDesc();


	// Get the layout in memory, we only care about the current subresource (at mip 0).
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
	UINT64 rowSize, totalSize;
	UINT numRows;
	device->GetCopyableFootprints(&realDesc, subresourceIndex, 1, 0, &layout, &numRows, &rowSize, &totalSize);

	ID3D12Resource* uploadBuffer{ nullptr };
	{
		D3D12_HEAP_PROPERTIES heapDesc{};
		heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC rscDesc{};
		rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		rscDesc.Width = totalSize;
		rscDesc.Height = 1;
		rscDesc.DepthOrArraySize = 1;
		rscDesc.MipLevels = 1;
		rscDesc.Format = DXGI_FORMAT_UNKNOWN;
		rscDesc.Alignment = 0;
		rscDesc.SampleDesc.Count = 1;
		rscDesc.SampleDesc.Quality = 0;
		rscDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		// Intermediate buffer
		DX_RET(device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAG_NONE, &rscDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer)));
		rscPool.push_back(uploadBuffer);
	}

	// Copy data to upload buffer, with proper offsets.
	{
		const UINT64 srcRowPitch = (UINT64)img.w * (UINT64)img.c;
		const UINT64 srcSlicePitch = srcRowPitch * (UINT64)img.h;

		const UINT64 dstRowPitch = layout.Footprint.RowPitch;
		const UINT64 dstSlicePitch = dstRowPitch * (UINT64)numRows;

		BYTE* dst{ nullptr };
		DX_RET(uploadBuffer->Map(0, nullptr, (void**)&dst));
		dst += layout.Offset;

		BYTE* src = (BYTE*)img.data.data();

		for (UINT z = 0; z < layout.Footprint.Depth; ++z) {
			BYTE* srcSlice = src + z * srcSlicePitch;
			BYTE* dstSlice = dst + z * dstSlicePitch;

			for (UINT y = 0; y < layout.Footprint.Height; ++y) {
				BYTE* srcRow = srcSlice + y * srcRowPitch;
				BYTE* dstRow = dstSlice + y * dstRowPitch;
				memcpy( dstRow, srcRow, rowSize);
			}
		}
		uploadBuffer->Unmap(0, nullptr);
	}

	GPU::transitionResource(commandList, texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST, subresourceIndex);

	// Then schedule a copy from the upload buffer to the texture level 0.
	{
		D3D12_TEXTURE_COPY_LOCATION src{};
		src.pResource = uploadBuffer;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = layout;

		D3D12_TEXTURE_COPY_LOCATION dst{};
		dst.pResource = texture;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = subresourceIndex;
		commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	}

	const D3D12_RESOURCE_STATES tgtState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	GPU::transitionResource(commandList, texture, D3D12_RESOURCE_STATE_COPY_DEST, tgtState, subresourceIndex);

	// Mipmap using a compute.
	UINT mipCount = realDesc.MipLevels;
	if( mipCount > 1){
		if(_mipmapGeneration.signature == nullptr){
			_mipmapGeneration.configureCompute("resources/shaders/Mipmap", device);
		}

		// Prepare descriptors
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = realDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.FirstArraySlice = subresourceIndex / mipCount;
		srvDesc.Texture2DArray.ArraySize = 1;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.Format = realDesc.Format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.FirstArraySlice = subresourceIndex / mipCount;
		uavDesc.Texture2DArray.ArraySize = 1;
		uavDesc.Texture2DArray.PlaneSlice = 0;

		commandList->SetComputeRootSignature(_mipmapGeneration.signature);
		commandList->SetPipelineState(_mipmapGeneration.pipeline);

		UINT nextLevelWidth = (UINT) realDesc.Width;
		UINT nextLevelHeight = (UINT)realDesc.Height;
		for(UINT i = 0; i < mipCount - 1; ++i){
			D3D12_CPU_DESCRIPTOR_HANDLE table = descriptorAllocator.allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2);
			D3D12_CPU_DESCRIPTOR_HANDLE srv = table;
			D3D12_CPU_DESCRIPTOR_HANDLE uav = descriptorAllocator.next(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, srv);
			D3D12_GPU_DESCRIPTOR_HANDLE tableGpu = descriptorAllocator.convert(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, table);
			
			// Create SRV view on subresourceIndex + i
			srvDesc.Texture2DArray.MostDetailedMip = i;
			device->CreateShaderResourceView(texture, &srvDesc, srv);
			// Create UAV view on subresourceIndex + i + 1
			uavDesc.Texture2DArray.MipSlice = i + 1;
			device->CreateUnorderedAccessView(texture, nullptr, &uavDesc, uav);
			
			// i is already in tgtState
			// i + 1 is already in D3D12_RESOURCE_STATE_UNORDERED_ACCESS

			// Set two view as resource table
			commandList->SetComputeRootDescriptorTable(0, tableGpu);

			nextLevelWidth /= 2;
			nextLevelHeight /= 2;
			const UINT gx = (nextLevelWidth + 7) / 8;
			const UINT gy = (nextLevelHeight + 7) / 8;
			commandList->Dispatch(gx, gy, 1);

			// Work done on i+1, transition to tgtState
			GPU::transitionResource(commandList, texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, tgtState, subresourceIndex + i + 1);
			
		}	
	}
}


Pipeline GPU::_mipmapGeneration;