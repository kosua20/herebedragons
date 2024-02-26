#pragma once
#include "Pipeline.hpp"
#include "DescriptorAllocator.hpp"
#include "helpers/Resources.hpp"

#include <d3d12.h>
#include <dxgi1_4.h>
#include <vector>

class GPU {

public:

	static IDXGIAdapter1* retrieveBestAdapter(IDXGIFactory2* factory);

	static void transitionResource(ID3D12GraphicsCommandList* commandList, ID3D12Resource* rsc, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	static ID3D12Resource* createAndUploadBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::vector<ID3D12Resource*>& resourcePool, size_t size, char* data);

	static void uploadImageToSubresource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DescriptorAllocator& descriptorAllocator, std::vector<ID3D12Resource*>& rscPool, ID3D12Resource* texture, const Image& img, UINT subresourceIndex);

	static ID3D12Resource* createTexture(ID3D12Device* device, DXGI_FORMAT format, UINT w, UINT h, UINT d, UINT mip, D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

private:

	static Pipeline _mipmapGeneration;
};

