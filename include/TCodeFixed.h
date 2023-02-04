// TCode-Fixed-Point-H v1.0,
// protocal by TempestMAx (https://www.patreon.com/tempestvr)
// implemented by Eve 05/02/2022
// usage of this class can be found at (https://github.com/Dreamer2345/Arduino_TCode_Parser)
// Please copy, share, learn, innovate, give attribution.
// Q16 Implementation for fractional calculations on hardware without an FPU
// History:
//
#pragma once
#ifndef TCODE_FIXED_POINT_H
#define TCODE_FIXED_POINT_H
#include "Arduino.h"

#define Q 16

/*
int32_t sat_Q16(int64_t x)
{
	if (x > 0x7FFFFFFF) return 0x7FFFFFFF;
	else if (x < -0x80000000) return -0x80000000;
	else return (int32_t)x;
}
*/

int32_t addQ16(int32_t a, int32_t b)
{
	int64_t tmp = (int64_t)a + (int64_t)b;
	return tmp;
}

int32_t subQ16(int32_t a, int32_t b)
{
	return a - b;
}

int32_t multQ16(int32_t a, int32_t b)
{
	int64_t tmp = ((int64_t)a * (int64_t)b) >> Q;
	return tmp;
}

int32_t divQ16(int32_t a, int32_t b)
{
	return ((((int64_t)a) << Q) / b);
}

int32_t constrainQ16(int32_t v, int32_t min, int32_t max)
{
	if (v < min)
		return min;
	if (v > max)
		return max;
	return v;
}

int32_t Q16fromInt(int i)
{
	return ((int32_t)i) << Q;
}

int32_t Q16fromFloat(float i)
{
	float newValuef = i * (1 << Q);
	return (int32_t)newValuef;
}

int32_t Q16fromDouble(double i)
{
	double newValued = i * (1 << Q);
	return (int32_t)newValued;
}

int IntfromQ16(int32_t i)
{
	i = addQ16(i, Q16fromFloat(0.5f));
	return (int)(i >> Q);
}

float FloatfromQ16(int32_t i)
{
	return (((float)i) / (1 << Q));
}

double DoublefromQ16(int32_t i)
{
	return (((double)i) / (1 << Q));
}

#endif