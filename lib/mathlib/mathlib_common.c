#include "mathlib_common.h"
#include <stdarg.h>

int CompareS32(const void* a, const void* b) {
	return *(int32_t*)a - *(int32_t*)b;
}

int CompareS16(const void* a, const void* b) {
	return *(int16_t*)a - *(int16_t*)b;
}

int CompareU16(const void* a, const void* b) {
	return *(uint16_t*)a < *(uint16_t*)b ? -1 : *(uint16_t*)a > *(uint16_t*)b ? 1 : 0;
}

int CompareU64(const void* a, const void* b) {
	return *(uint64_t*)a < *(uint64_t*)b ? -1 : *(uint64_t*)a > *(uint64_t*)b ? 1 : 0;
}

int CompareFloat(const void* a, const void* b) {
	return *(float*)a < *(float*)b ? -1 : *(float*)a > *(float*)b ? 1 : 0;
}

unsigned int fibonacchi(unsigned int a) {
	return a == 0 ? 0 : a == 1 ? 1 : fibonacchi(a - 2) + fibonacchi(a - 1);
}

float linear_interpol(uint64_t x0, float y0, uint64_t x1, float y1, uint64_t x_req) {
	return y0 + ((y1 - y0) * (float)(x_req - x0)) / (float)(x1 - x0);
}

float cos_interpol(float a, float b, float t) {
	float ft = t * M_PI;
	float f	 = (1 - cosf(ft)) * 0.5;
	return a * (1 - f) + b * f;
}

float lerp(float a, float b, float t) {	 //another one linear interpolation
	return a + (b - a) * t;
}

float exp_curve(float x, float xMin, float xMax, float yMin, float yMax, float k) {
	if (k == 0) {
		return yMin + (yMax - yMin) * (x - xMin) / (xMax - xMin);
	}

	float exp_min = 1.0f;
	float exp_max = expf(k * (xMax - xMin));
	float exp_x	  = expf(k * (x - xMin));

	return yMin + (yMax - yMin) * (exp_x - exp_min) / (exp_max - exp_min);
}

float shaped_sine(float t, float power) {
	float s = sinf(t);
	return (s >= 0.0f ? 1.0f : -1.0f) * powf(fabsf(s), power);
}

float haversine(float lat1, float lon1, float lat2, float lon2) {
	double lat_diff = (double)D2R(lat2 - lat1);
	double lon_diff = (double)D2R(lon2 - lon1);

	double a =
		pow(sin(lat_diff / 2.0), 2) + cos(D2R(lat1)) * cos(D2R(lat2)) * pow(sin(lon_diff / 2.0), 2);
	double b = atan2(sqrt(a), sqrt(1 - a));
	return (float)(2.0 * AVG_EARTH_RADIUS * b);
}

float norma(unsigned int cardinality, ...) {
	va_list vectorElements;
	va_start(vectorElements, cardinality);
	float normaRet = 0;
	for (unsigned int i = 0; i < cardinality; i++) {
		float component = (float)va_arg(vectorElements, double);
		normaRet += (component * component);
	}
	va_end(vectorElements);

	return sqrtf(normaRet);
}

static const unsigned char reverse_byte_table[] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

unsigned char reverse_byte(unsigned char a) {
	return reverse_byte_table[a];
}

Quaternion_t quaternion_multiply(Quaternion_t qa, Quaternion_t qb) {
	Quaternion_t resultQ = {
		.W = qa.W * qb.W - qa.X * qb.X - qa.Y * qb.Y - qa.Z * qb.Z,
		.X = qa.W * qb.X + qa.X * qb.W + qa.Y * qb.Z - qa.Z * qb.Y,
		.Y = qa.W * qb.Y + qa.Y * qb.W + qa.Z * qb.X - qa.X * qb.Z,
		.Z = qa.W * qb.Z + qa.Z * qb.W + qa.X * qb.Y - qa.Y * qb.X,
	};

	return resultQ;
}

float distance_two_pt(Point2D_t pt1, Point2D_t pt2) {
	return sqrtf(powf(pt1.X - pt2.X, 2) + powf(pt1.Y - pt2.Y, 2));
}

/**
 * Jenkins hash function
 * https://en.wikipedia.org/wiki/Jenkins_hash_function
 */
unsigned int jenkins_hash(unsigned char* pBuff, unsigned int len) {
	unsigned int hash = 0;
	for (unsigned int i = 0; i < len; i++) {
		hash += pBuff[i];
		hash += hash << 10;
		hash ^= hash >> 6;
	}

	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;

	return hash;
}

static float fixed_random_gradient_1D(int x) {
	x = (x << 13) ^ x;
	return (1.0 - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}

static float fixed_random_gradient_2D(int x, int y) {
	int seed = x * 49632 + y * 325176;
	seed	 = (seed << 13) ^ seed;
	return (1.0 -
			((seed * (seed * seed * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}

float perlin_1D(float x) {
	int x0 = (int)x;
	int x1 = x0 + 1;

	float g0 = fixed_random_gradient_1D(x0);
	float g1 = fixed_random_gradient_1D(x1);

	float t = x - (float)x0;

	float v0 = g0 * t;
	float v1 = g1 * (t - 1);

	return cos_interpol(v0, v1, t);
}

float perlin_2D(float x, float y) {
	int x0 = (int)x;
	int x1 = x0 + 1;
	int y0 = (int)y;
	int y1 = y0 + 1;

	float g00 = fixed_random_gradient_2D(x0, y0);
	float g10 = fixed_random_gradient_2D(x1, y0);
	float g01 = fixed_random_gradient_2D(x0, y1);
	float g11 = fixed_random_gradient_2D(x1, y1);

	float tx = x - (float)x0;
	float ty = y - (float)y0;

	float v00 = g00 * (tx) + g01 * (ty);
	float v10 = g10 * (tx - 1) + g11 * (ty);
	float v0  = cos_interpol(v00, v10, tx);

	float v01 = g00 * (tx) + g10 * (ty);
	float v11 = g01 * (tx - 1) + g11 * (ty);
	float v1  = cos_interpol(v01, v11, tx);

	return cos_interpol(v0, v1, ty);
}

static int perlin_hash(int x, int y) {
	int seed = x * 49632 + y * 325176;
	seed	 = (seed << 13) ^ seed;
	seed	 = (seed * (seed * seed * 15731 + 789221) + 1376312589) & 0x7fffffff;
	return seed;
}

static float perlin_grad(int hash) {
	return 2.0f * (hash & 15) / 15.0f - 1.0f;
}

float perlin2D(float x, float y) {
	int X	 = (int)floorf(x) & 255;  // must be power of 2
	int Y	 = (int)floorf(y) & 255;
	float xf = x - floorf(x);
	float yf = y - floorf(y);

	int top_left	 = perlin_hash(X, Y);
	int top_right	 = perlin_hash(X + 1, Y);
	int bottom_left	 = perlin_hash(X, Y + 1);
	int bottom_right = perlin_hash(X + 1, Y + 1);

	float u = perlin_grad(top_left) * xf + perlin_grad(top_right) * (xf - 1);
	float v = perlin_grad(bottom_left) * xf + perlin_grad(bottom_right) * (xf - 1);

	return cos_interpol(u, v, yf);
}

float wrap_range(float x, float min, float max) {
	float range = max - min;
	if (range == 0.0f)
		return min;

	float result = fmodf(x - min, range);
	if (result < 0.0f)
		result += range;
	return result + min;
}

static int P[512] = {
	59,	 32,  66,  84,	192, 224, 168, 29,	124, 37,  177, 88,	99,	 132, 120, 218, 232, 98,  100,
	216, 67,  78,  15,	128, 155, 238, 111, 31,	 48,  185, 41,	160, 247, 50,  95,	228, 157, 49,
	107, 115, 178, 237, 201, 208, 173, 164, 149, 200, 118, 53,	229, 20,  83,  137, 176, 182, 43,
	226, 85,  246, 64,	60,	 80,  102, 244, 196, 104, 103, 18,	101, 4,	  122, 171, 119, 251, 125,
	71,	 172, 143, 217, 47,	 14,  141, 97,	249, 45,  183, 145, 136, 220, 140, 156, 187, 65,  74,
	138, 245, 81,  151, 198, 152, 86,  254, 117, 76,  35,  212, 126, 44,  63,  154, 175, 56,  250,
	30,	 225, 240, 233, 39,	 11,  24,  210, 195, 127, 51,  21,	234, 194, 13,  197, 70,	 91,  241,
	133, 109, 42,  28,	252, 34,  75,  223, 255, 193, 121, 222, 190, 62,  205, 189, 147, 236, 38,
	25,	 7,	  89,  87,	33,	 231, 22,  61,	169, 0,	  142, 26,	114, 243, 227, 248, 207, 68,  209,
	230, 123, 52,  69,	235, 10,  159, 77,	166, 199, 58,  179, 79,	 184, 55,  3,	186, 5,	  40,
	242, 90,  82,  6,	57,	 148, 181, 17,	221, 130, 167, 1,	2,	 16,  106, 129, 54,	 253, 134,
	206, 8,	  146, 219, 163, 93,  144, 150, 211, 110, 180, 165, 92,	 73,  139, 19,	105, 23,  12,
	214, 204, 36,  113, 108, 188, 96,  72,	203, 153, 116, 174, 27,	 202, 135, 112, 94,	 161, 9,
	131, 162, 213, 191, 215, 170, 158, 239, 46,
};

static float grad3d(int hash, float x, float y, float z) {
	int h	= hash & 15;
	float u = h < 8 ? x : y, v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

static float fade(float t) {
	return t * t * t * (t * (t * 6 - 15) + 10);
}

float perlin_3D(float x, float y, float z) {
	int X = (int)x & 255;
	int Y = (int)y & 255;
	int Z = (int)z & 255;
	float u, v, w;

	x -= (int)x;
	y -= (int)y;
	z -= (int)z;

	u = fade(x);
	v = fade(y);
	w = fade(z);

	for (int i = 0; i < 256; i++)
		P[256 + i] = P[i];

	int aaa = P[P[P[X] + Y] + Z];
	int aba = P[P[P[X] + Y + 1] + Z];
	int aab = P[P[P[X] + Y] + Z + 1];
	int abb = P[P[P[X] + Y + 1] + Z + 1];
	int baa = P[P[P[X + 1] + Y] + Z];
	int bba = P[P[P[X + 1] + Y + 1] + Z];
	int bab = P[P[P[X + 1] + Y] + Z + 1];
	int bbb = P[P[P[X + 1] + Y + 1] + Z + 1];

	float x1 = lerp(grad3d(aaa, x, y, z), grad3d(aba, x, y - 1, z), v);
	float x2 = lerp(grad3d(baa, x - 1, y, z), grad3d(bba, x - 1, y - 1, z), v);
	float y1 = lerp(x1, x2, u);

	x1		 = lerp(grad3d(aab, x, y, z - 1), grad3d(abb, x, y - 1, z - 1), v);
	x2		 = lerp(grad3d(bab, x - 1, y, z - 1), grad3d(bbb, x - 1, y - 1, z - 1), v);
	float y2 = lerp(x1, x2, u);

	return lerp(y1, y2, w);
}
