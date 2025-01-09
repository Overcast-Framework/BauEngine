#include "bepch.h"
#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <BauEngine/Core/Utils.h>

std::vector<UINT8> Texture2D::ConvertToD3D12()
{
	std::vector<UINT8> imageData(Data, Data + (Width * Height * 4));
	return imageData;
}

void Texture2D::Free()
{
	stbi_image_free(Data);
}

Texture2D::Texture2D(std::string path)
{
	Name = path;

	stbi_set_flip_vertically_on_load(true);

	Data = stbi_load(path.c_str(), &Width, &Height, &Channels, 4);

	if (Data == nullptr)
	{
		BE_ASSERT(!(Data == nullptr), "Failed to load image " + path);
		return;
	}
	
}

