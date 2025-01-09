#pragma once
#include <bepch.h>
#include <iostream>

class Texture2D
{
public:
	std::string Name;
	unsigned char* Data;

	int Width, Height, Channels;

	std::vector<UINT8> ConvertToD3D12();
	void Free();

	Texture2D(std::string path);
};