#pragma once
#include <iostream>
#include <vector>
#include <BauEngine/Graphics/Vertex.h>

class Mesh
{
public:
	std::vector<BEVertex> Vertices;
	std::vector<unsigned int> Indices;
};