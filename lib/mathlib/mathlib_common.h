#ifndef __MATHLIB_COMMON_H
#define __MATHLIB_COMMON_H

#include <math.h>
#include <stdint.h>

/*
 * Constants
 */
#define AVG_EARTH_RADIUS 6371.0088	//km
#define M_PI			 3.14159265358979323846
#define EPSILON			 0.000001f

/*
 * Macro functions
 */
#define ABS(a)			  (((a) < 0) ? -(a) : (a))
#define R2D(a)			  (((a) * 180.0) / (M_PI))
#define D2R(a)			  (((a) * (M_PI)) / 180.0)
#define IS_CLOSE(a, b)	  (fabs(a - b) < EPSILON ? true : false)
#define SHIFT_AND_NORM(a) ((a) * 0.5f + 0.5f)

/** 
 * Converting wheel speed from km/h to m/s 
 */
#define KMH2MS(speed) ((speed) / 3.6)

/*
 * Custom data types
 */
typedef unsigned int (*CurveFunc_t)(unsigned int);

typedef struct {
	float X;
	float Y;
} Point2D_t;

typedef struct {
	float W;
	float X;
	float Y;
	float Z;
} Quaternion_t;

/*
 * Functions
 */
int CompareS32(const void* a, const void* b);
int CompareS16(const void* a, const void* b);
int CompareU16(const void* a, const void* b);
int CompareU64(const void* a, const void* b);
int CompareFloat(const void* a, const void* b);

unsigned int fibonacchi(unsigned int a);
float linear_interpol(uint64_t x0, float y0, uint64_t x1, float y1, uint64_t x_req);
float cos_interpol(float a, float b, float t);
float lerp(float a, float b, float t);
float exp_curve(float x, float xMin, float xMax, float yMin, float yMax, float k);
float shaped_sine(float t, float power);
float haversine(float lat1, float lon1, float lat2, float lon2);
float norma(unsigned int cardinality, ...);
unsigned char reverse_byte(unsigned char a);
Quaternion_t quaternion_multiply(Quaternion_t qa, Quaternion_t qb);
float distance_two_pt(Point2D_t pt1, Point2D_t pt2);
unsigned int jenkins_hash(unsigned char* pBuff, unsigned int len);
float perlin_1D(float x);
float perlin_2D(float x, float y);
float wrap_range(float x, float min, float max);
float perlin_3D(float x, float y, float z);

#endif /* __MATHLIB_COMMON_H */
