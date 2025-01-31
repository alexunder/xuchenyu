/*
 * The PerspectiveCamera class derived from Camera
 * Implemented by Mohism Research
 */

#ifndef __H_PERSPECTIVECAMERA
#define __H_PERSPECTIVECAMERA

#include "Camera.h"
#include "vectors.h"

class PerspectiveCamera : public Camera
{
public:
    PerspectiveCamera(Vec3f &center, Vec3f &direction, Vec3f &up, float angle);
	Ray generateRay(Vec2f point);
    void setRatio(float ratio)
    {
        mRatio = ratio;
    }

    CameraType getCameraType()
    {
        return CameraType::Perspective;
    }

    float getTMin() const
    {
        return 0.0;
    }
private:
    Vec3f mCenter;
    Vec3f mDirection;
    Vec3f mUp;
    Vec3f mHorizontal;
    float mAngle;
    float mRatio;
};

#endif
