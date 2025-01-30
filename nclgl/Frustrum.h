#pragma once
#include "Plane.h"
class SceneNode; //These let the compiler know that these keywords are classes, without including the whole header
class Matrix4;

class Frustrum {
public:
	Frustrum(void) {};
	~Frustrum(void) {};

	void FromMatrix(const Matrix4& mvp);
	bool InsideFrustrum(SceneNode& n);

protected:
	Plane planes[6];
};

