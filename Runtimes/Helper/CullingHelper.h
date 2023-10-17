// Meso Engine 2024
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct FCullingHelper
{
	float DistanceToPlane(const glm::vec4& Plane, const glm::vec3& Point) 
	{
		return glm::dot(glm::vec4(Point, 1.0f), Plane);
	}
    void ExtractPlanes(glm::vec4 Planes[6], const glm::mat4& ComboMatrix) 
    {
        // Left clipping plane
        Planes[0] = glm::vec4(ComboMatrix[3] + ComboMatrix[0]);
        // Right clipping plane
        Planes[1] = glm::vec4(ComboMatrix[3] - ComboMatrix[0]);
        // Top clipping plane
        Planes[2] = glm::vec4(ComboMatrix[3] - ComboMatrix[1]);
        // Bottom clipping plane
        Planes[3] = glm::vec4(ComboMatrix[3] + ComboMatrix[1]);
        // Near clipping plane
        Planes[4] = glm::vec4(ComboMatrix[3] + ComboMatrix[2]);
        // Far clipping plane
        Planes[5] = glm::vec4(ComboMatrix[3] - ComboMatrix[2]);

        // Normalize the planes
        for (int i = 0; i < 6; i++) 
        {
            //Planes[i] /= glm::length(glm::vec3(Planes[i]));
            float Len = glm::length(glm::vec3(Planes[i]));
            Planes[i] /= Len;
        }
    }
    //If the result is greater than 0, the sphere is potentially visible.If the result is less than or equal to 0, the sphere is outside the frustum and can be culled.
    float CullSphere(const glm::mat4& ComboMatrix, const glm::vec3& SphereCenter, float SphereRadius)
    {
        glm::vec4 Planes[6];
        ExtractPlanes(Planes, ComboMatrix);
        float Dist01 = std::min(DistanceToPlane(Planes[0], SphereCenter), DistanceToPlane(Planes[1], SphereCenter));
        float Dist23 = std::min(DistanceToPlane(Planes[2], SphereCenter), DistanceToPlane(Planes[3], SphereCenter));
        float Dist45 = std::min(DistanceToPlane(Planes[4], SphereCenter), DistanceToPlane(Planes[5], SphereCenter));
        return std::min(std::min(Dist01, Dist23), Dist45) + SphereRadius;
    }
    bool IsSphereCulled(const glm::mat4& ComboMatrix, const glm::vec3& SphereCenter, float SphereRadius)
    {
        glm::vec4 Planes[6];
        ExtractPlanes(Planes, ComboMatrix);
        for (int i = 0; i < 6; i++)
        {
            if (DistanceToPlane(Planes[i], SphereCenter) < -SphereRadius)
                return true; // Sphere is outside this plane, so it's culled
        }
        return false; // Sphere is inside all planes
    }
};