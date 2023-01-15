#include "Utility.h"
#include <cmath>
bool castRayTriangle(const Scene& scene, int x, int y,
const Ray& R, const Triangle& T,
Radiance3& radiance, float& distance);
Ray sendRay(float x, float y, int width, int height, const Camera& camera);
float intersect(const Ray& R, const Triangle& T, float weight[3]);
//
void rayCast(Image& image, const Scene& scene,
const Camera& camera, int x0, int x1, int y0, int y1)
{
    // For each pixel
    for (int y = y0; y < y1; ++y)
    {
        for (int x = y0; x < x1; ++x)
        {
            // Ray through the pixel
            const Ray& R = sendRay(x + 0.5f, y + 0.5f, image.width(),
            image.height(), camera);
            // Distance to closest known intersection
            float distance = INFINITY;
            Radiance3 L_o;
            // For each triangle
            for (unsigned int t = 0; t < scene.triangleArray.size(); ++t)
            {
                const Triangle& T = scene.triangleArray[t];
                if (castRayTriangle(scene, x, y, R, T, L_o, distance))
                {
                    image.set(x, y, L_o);
                }
            }
        }
    }
}
Ray sendRay(float x, float y, int width, int height, const Camera& camera)
{
    const float aspect = float(height) / width;
    // Compute the side of a square at z = -1 based on our
    // horizontal left-edge-to-right-edge field of view
    const float s = -2.0f * tan(camera.fieldOfViewX * 0.5f);
    Vector3& start =
    Vector3( (x / width - 0.5f) * s,
            -(y / height - 0.5f) * s * aspect, 
            1.0f) * camera.zNear;
    return Ray(start, start.normalize());
}
bool castRayTriangle(const Scene& scene, int x, int y, const Ray& R,
const Triangle& T, Radiance3& radiance, float& distance)
{
    float weight[3];
    const float d = intersect(R, T, weight);
    if (d >= distance)
    {
        return false;
    }
    // This intersection is closer than the previous one
    distance = d;
    // Intersection point
    const Point3& P = R.origin() + (R.direction())*d;
    // Find the interpolated vertex normal at the intersection
    const Vector3& n = (T.normal(0) * weight[0] +
                        T.normal(1) * weight[1] +
                        T.normal(2) * weight[2]).normalize();
    const Vector3& w_o = -R.direction();
    shade(scene, T, P, n, w_o, radiance);
    // Debugging intersect: set to white on any intersection
    //radiance = Radiance3(1, 1, 1);
    // Debugging barycentric
    //radiance = Radiance3(weight[0], weight[1], weight[2]) / 15;
    return true;
}
float intersect(const Ray& R, const Triangle& T, float weight[3])
{
    const Vector3& e1 = T.vertex(1) - T.vertex(0);
    const Vector3& e2 = T.vertex(2) - T.vertex(0);
    const Vector3& q = R.direction().cross(e2);
    const float a = e1.dot(q);
    const Vector3& s = R.origin() - T.vertex(0);
    const Vector3& r = s.cross(e1);
    // Barycentric vertex weights
    weight[1] = s.dot(q) / a;
    weight[2] = R.direction().dot(r) / a;
    weight[0] = 1.0f - (weight[1] + weight[2]);
    const float dist = e2.dot(r) / a;
    static const float epsilon = 1e-7f;
    static const float epsilon2 = 1e-10;
    if ((a <= epsilon) || (weight[0] < -epsilon2) ||
        (weight[1] < -epsilon2) || (weight[2] < -epsilon2) ||
        (dist <= 0.0f)) 
    {
        // The ray is nearly parallel to the triangle, or the
        // intersection lies outside the triangle or behind
        // the ray origin: "infinite" distance until intersection.
        return INFINITY;
    } 
    else 
    {
        return dist;
    }
}
void shade(const Scene& scene, const Triangle& T, const Point3& P,
const Vector3& n, const Vector3& w_o, Radiance3& L_o)
{
    L_o = Color3(0.0f, 0.0f, 0.0f);
    // For each direction (to a light source)
    for (unsigned int i = 0; i < scene.lightArray.size(); ++i)
    {
        const Light& light = scene.lightArray[i];
        const Vector3& offset = light.position - P;
        const float distanceToLight = offset.norm();
        const Vector3& w_i = offset / distanceToLight;
        if (visible(P, w_i, distanceToLight, scene))
        {
            const Radiance3& L_i = light.power / (4 * M_PI * square(distanceToLight));
            // Scatter the light
            L_o += L_i * T.bsdf().evaluateFiniteScatteringDensity(w_i, w_o) *
            std::max(0.0f,  w_i.dot(n));
        }
    }
}
float square(float s){return s*s;}
bool visible(const Vector3& P, const Vector3& direction, float
distance, const Scene& scene)
{
    static const float rayBumpEpsilon = 1e-4;
    const Ray shadowRay(P + direction * rayBumpEpsilon, direction);
    distance -= rayBumpEpsilon;
    // Test each potential shadow caster to see if it lies between P and the light
    float ignore[3];
    for (unsigned int s = 0; s < scene.triangleArray.size(); ++s)
    {
        if (intersect(shadowRay, scene.triangleArray[s], ignore) < distance)
        {
            // This triangle is closer than the light
            return false;
        }
    }
    return true;
}