#include "RayCaster.h"
void rasterize(Image& image, const Scene& scene, const Camera& camera)
{
    const int w = image.width(), h = image.height();
    DepthBuffer depthBuffer(w, h, INFINITY);
    // For each triangle
    for (unsigned int t = 0; t < scene.triangleArray.size(); ++t)
    {
        const Triangle& T = scene.triangleArray[t];
        // Very conservative bounds: the whole screen
        const int x0 = 0;
        const int x1 = w;
        const int y0 = 0;
        const int y1 = h;
        // For each pixel
        for (int y = y0; y < y1; ++y)
        {
            for (int x = x0; x < x1; ++x) 
            {
                const Ray& R = sendRay(x, y, w, h, camera);
                Radiance3 L_o;
                float distance = depthBuffer.get(x, y);
                if (castRayTriangle(scene, x, y, R, T, L_o, distance))
                {
                    image.set(x, y, L_o);
                    depthBuffer.set(x, y, distance);
                }
            }
        }
    }
}