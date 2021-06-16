#pragma once
#include "../Environments/Common.h"
#include <stdint.h>
#include <cmath>
#include <Windows.h>

/*********************************************
 ******    Float and Rational utility   ******
 *********************************************

 *********************************************
 ******  Thanks to RAYMOD2 for his help  *****
 ******  in beating continued fractions  *****
 ******  into a usable form.       IanB  *****
 *********************************************/

static bool float_to_frac(float input, unsigned& num, unsigned& den);
static bool reduce_float(float value, unsigned& num, unsigned& den);
static void reduce_frac(uint32_t& num, uint32_t& den, uint32_t limit);
void FloatToFPS(const char* name, float n, uint32_t& num, uint32_t& den, ICommonEnvironment& env);
void PresetToFPS(const char* name, const char* p, uint32_t& num, uint32_t& den, ICommonEnvironment& env);
