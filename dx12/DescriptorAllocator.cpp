#include "DescriptorAllocator.hpp"


void DescriptorSubAllocator::init( ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT size, bool shaderVisible )
{
	_size = size;
	_type = type;
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = _size;
	desc.Type = _type;
	desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DX_RET( device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &_heap ) ) );

	_descriptorSize = device->GetDescriptorHandleIncrementSize( _type );
	_cpuStart = _heap->GetCPUDescriptorHandleForHeapStart();
	_gpuStart = _heap->GetGPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorSubAllocator::allocate(UINT count)
{
	if( _allocated + count > _size )
		return { 0 };

	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = _cpuStart.ptr + _allocated * _descriptorSize;
	_allocated += count;
	return handle;
}

void DescriptorSubAllocator::clear()
{
	_heap->Release();
}


void DescriptorAllocator::init(ID3D12Device* device){
	const bool shaderVisibles[] = {
		true, true, false, false
	};
	const UINT sizes[] = {
		1024, 16, 128, 128
	};

	for(int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i){
		_allocators[i].init(device, ( D3D12_DESCRIPTOR_HEAP_TYPE )i, sizes[i], shaderVisibles[i]);
		if(shaderVisibles[i]){
			_gpuHeaps.push_back(_allocators[i].getHeap());
		}
	}
}


D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::allocate( D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count )
{
	return _allocators[ type ].allocate(count);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::next(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_CPU_DESCRIPTOR_HANDLE  handle, UINT offset)
{
	return _allocators[ type ].next(handle, offset);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorAllocator::convert( D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_CPU_DESCRIPTOR_HANDLE  handle )
{
	return _allocators[ type ].convert( handle );
}

void DescriptorAllocator::clear()
{
	for( int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
	{
		_allocators[ i ].clear();
	}
}