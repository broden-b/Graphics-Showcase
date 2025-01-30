#pragma once
#include <string>
#include "mesh.h"
#include <algorithm>

class HeightMap : public Mesh {
public:
    HeightMap(const std::string& name);
    ~HeightMap(void) {};

    Vector3 GetHeightmapSize() const { return heightmapSize; }

protected:
    Vector3 heightmapSize;
};