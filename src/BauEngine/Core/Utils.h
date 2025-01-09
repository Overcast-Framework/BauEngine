#pragma once
#include <iostream>
#include <d3d12.h>

#define BE_ASSERT(assert, error) if(!assert) std::cout << (std::string("\x1b[1;31m") + "[ERROR/CRIT]: " + "\x1b[0m" + error ) << std::endl
#define D3D12_ASSERT(hr, error) if(!SUCCEEDED(hr)) std::cout << (std::string("\x1b[1;31m") + "[ERROR/D3D12]: " + "\x1b[0m" + error ) << std::endl
#define DX_ASSERT(hr) if(!SUCCEEDED(hr)) std::cout << (std::string("\x1b[1;31m") + "[ERROR/D3D12]" + "\x1b[0m") << "HRESULT: " << std::hex << hr << std::dec << __FILE__ << " " << __LINE__ << std::endl