//m before a name indicates that it is a member. helps differentiate between members of a class and
//functions of similar names.

#include <vector>
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif
#include <cmath>
#include <iostream>

//vectors and points
class Vector2
{
public: 
    float x, y;
};
class Vector3 
{ 
public: 
    float x, y, z;
    Vector3(float x, float y, float z): x(x), y(y), z(z){}
    Vector3 operator+(const Vector3 &vec) const {return Vector3(x+vec.x,y+vec.y,z+vec.z);}
    Vector3 operator*(const float s) const {return Vector3(s*x,s*y,s*z);}
    Vector3 operator-(const Vector3 &vec) const {return (*this)+(-vec);}
    Vector3 operator/(const float s) const {return Vector3(x/s,y/s,z/s);}
    float norm() const {return sqrt(x*x+y*y+z*z);}
    Vector3 normalize() {return (*this)/this->norm();}
    friend Vector3 operator-(const Vector3 &vec);
    Vector3 cross(const Vector3 &vec) const
    {
        return Vector3(y*vec.z-z*vec.y,
                        z*vec.x-x*vec.z,
                        x*vec.y-y*vec.x);
    }
    float dot(const Vector3 &vec) const {return x*vec.x+y*vec.y+z*vec.z;}
};
Vector3 operator-(const Vector3 &vec){return Vector3(-vec.x,-vec.y,-vec.z);}
typedef Vector2 Point2;
typedef Vector3 Point3;

//color and power values
class Color3 
{ 
public: 
    float r, g, b;
    Color3(float r, float g, float b): r(r), g(g), b(b){}
    Color3():r(0),g(0),b(0){}
    Color3 operator/(const float s) const {return Color3(r/s,g/s,b/s);}
    Color3 operator*(const Color3 &col) const {return Color3(r*col.r, g*col.g, b*col.b);}
    Color3 operator*(const float s) const {return Color3(r*s, g*s, b*s);}
    Color3 operator+=(const Color3 &col)
    {
        r=r+col.r;
        g=g+col.g;
        b=b+col.b;
    }
};
typedef Color3 Radiance3;
typedef Color3 Power3;

//ray object (ray casting)
class Ray 
{
private:
    Point3 m_origin;
    Vector3 m_direction;
public:
    Ray(const Point3& org, const Vector3& dir) : m_origin(org), m_direction(dir) {}
    const Point3& origin() const { return m_origin; }
    const Vector3& direction() const { return m_direction; }
};

//basic image-as-2D-array class
class Image
{
private:
    int m_width;
    int m_height;
    //one dimensional dynamic array
    std::vector<Radiance3> m_data;
    int gammaEncode(float radiance, float displayConstant) const
    {
        //gammaEncodedValue=(value^(1/2.2)*255)
        return int(pow(std::min(1.0f,std::max(0.0f,radiance*displayConstant)),1.0/2.2)*255.0);
    }
    /*
    a faster encoding using square roots
    int gammaEncode(float radiance, float displayConstant) const
    {
        return int(sqrt(std::min(1.0,std::max(0.0,radiance*displayConstant)))*255.0);
    }
    */
public:
    Image(int width, int height) : m_width(width), m_height(height), m_data(width * height) {}
    int width() const { return m_width; }
    int height() const { return m_height; }
    void set(int x, int y, const Radiance3& value)
    {
        //instead of m_data[x][y] we use x+y*width
        m_data[x + y * m_width] = value;
    }
    const Radiance3& get(int x, int y) const
    {
        return m_data[x + y * m_width];
    }
    //save as PPM file (convenient for learning purposes)
    void save(const std::string& filename, float displayConstant) const 
    {
        FILE* file = fopen(filename.c_str(), "wt");
        //required parameters
        fprintf(file, "P3 %d %d 255\n", m_width, m_height);
        for (int y = 0; y < m_height; ++y)
        {
            fprintf(file, "\n# y = %d\n", y);
            for (int x = 0; x < m_width; ++x)
            {
                const Radiance3& c(get(x, y));
                fprintf(file, "%d %d %d\n", gammaEncode(c.r, displayConstant),
                                            gammaEncode(c.g, displayConstant), 
                                            gammaEncode(c.b, displayConstant));
            }
        }
        fclose(file);
    }
};

class DepthBuffer
{
private:
    int m_width;
    int m_height;
    //one dimensional dynamic array
    std::vector<float> m_data;
public:
    DepthBuffer(int width, int height, float x) : m_width(width), m_height(height), m_data(width * height) 
    {
        for(std::vector<float>::iterator i = m_data.begin(); i!=m_data.end();i++)
        {
            *i=x;
        }
    }
    int width() const { return m_width; }
    int height() const { return m_height; }
    void set(int x, int y, float value)
    {
        //instead of m_data[x][y] we use x+y*width
        m_data[x + y * m_width] = value;
    }
    const float get(int x, int y) const
    {
        return m_data[x + y * m_width];
    }
};

//material biderectional scattering distribution function base class
class BSDF
{
public:
    Color3 k_L;
    /** Returns f = L_o / (L_i * w_i.dot(n)) assuming
    incident and outgoing directions are both in the
    positive hemisphere above the normal */
    Color3 evaluateFiniteScatteringDensity (const Vector3& w_i,
                            const Vector3& w_o) const 
    {
        return k_L / M_PI;
    }
};

//triangle, each vertex has a position vector and a normal vector
class Triangle
{
private:
    Point3 m_vertex[3];
    Vector3 m_normal[3];
    //the bidirectional scattering distribution function of the triangle
    BSDF m_bsdf;
public:
    const Point3& vertex(int i) const { return m_vertex[i]; }
    const Vector3& normal(int i) const { return m_normal[i]; }
    const BSDF& bsdf() const { return m_bsdf; }
};

//light: position and power
class Light
{
public:
    Point3 position;
    /* Over the entire sphere. */
    Power3 power;
};

//scene:arrays or triangles and lights
class Scene
{
public:
    //arbitrarly ordered triangles and lights arrays
    std::vector<Triangle> triangleArray;
    std::vector<Light> lightArray;
};

//camera: view frustum
class Camera
{
public:
    float zNear;
    float zFar;
    float fieldOfViewX;
    Camera() : zNear(-0.1f), zFar(-100.0f), fieldOfViewX(M_PI) {}
};
