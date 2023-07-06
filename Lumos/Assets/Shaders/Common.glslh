#define PI 3.1415926535897932384626433832795
#define GAMMA 2.2

const float TwoPI = 2 * PI;
const float Epsilon = 0.00001;

float sq(float x)
{
    return x * x;
}

float pow5(float x) 
{
    float x2 = x * x;
    return x2 * x2 * x;
}

float max3(const vec3 v) 
{
    return max(v.x, max(v.y, v.z));
}

vec3 DeGamma(vec3 colour)
{
	return pow(colour, vec3(GAMMA));
}

vec4 DeGamma(vec4 colour)
{
	return vec4(DeGamma(colour.xyz), colour.w);
}

vec3 Gamma(vec3 colour)
{
	return pow(colour, vec3(1.0f / GAMMA));
}

float saturate(float value)
{
	return clamp(value, 0.0f, 1.0f);
}

vec2 saturate(vec2 colour)
{
	return clamp(colour , 0.0, 1.0);
}

vec3 saturate(vec3 colour)
{
	return clamp(colour , 0.0, 1.0);
}

float LinearizeDepth(const float screenDepth, float DepthUnpackX, float DepthUnpackY)
{
	float depthLinearizeMul = DepthUnpackX;
	float depthLinearizeAdd = DepthUnpackY;
	return depthLinearizeMul / (depthLinearizeAdd - screenDepth);
}