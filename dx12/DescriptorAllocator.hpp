#pragma once
#include "helpers/Log.hpp"

#include <d3d12.h>
#include <vector>

class DescriptorSubAllocator
{
public:

	DescriptorSubAllocator(){};

	void init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT size, bool shaderVisible );

	D3D12_CPU_DESCRIPTOR_HANDLE allocate(UINT count);

	D3D12_CPU_DESCRIPTOR_HANDLE next( D3D12_CPU_DESCRIPTOR_HANDLE handle, UINT offset )
	{
		D3D12_CPU_DESCRIPTOR_HANDLE nextHandle;
		nextHandle.ptr = handle.ptr + offset * _descriptorSize;
		return nextHandle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE convert( D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle )
	{
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
		gpuHandle.ptr = _gpuStart.ptr + ( cpuHandle.ptr - _cpuStart.ptr );
		return gpuHandle;
	}

	ID3D12DescriptorHeap* getHeap() { return _heap; }

	void clear();

private:

	ID3D12DescriptorHeap* _heap{nullptr};
	D3D12_DESCRIPTOR_HEAP_TYPE _type{ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES };
	D3D12_CPU_DESCRIPTOR_HANDLE _cpuStart{ 0 };
	D3D12_GPU_DESCRIPTOR_HANDLE _gpuStart{ 0 };
	UINT _size{0};
	UINT _allocated{0};
	UINT _descriptorSize{0};

};

class DescriptorAllocator {
public:

	void init(ID3D12Device* device);

	D3D12_CPU_DESCRIPTOR_HANDLE allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count = 0);

	D3D12_CPU_DESCRIPTOR_HANDLE next(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_CPU_DESCRIPTOR_HANDLE  handle, UINT offset = 1);

	D3D12_GPU_DESCRIPTOR_HANDLE convert(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_CPU_DESCRIPTOR_HANDLE handle);

	ID3D12DescriptorHeap** getGpuHeaps(){ return _gpuHeaps.data(); }

	void clear();

private:

	DescriptorSubAllocator _allocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	std::vector<ID3D12DescriptorHeap*> _gpuHeaps;
	
};