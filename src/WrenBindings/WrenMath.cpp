#include "WrenMath.h"

#include <limits>
#define _USE_MATH_DEFINES
#include <math.h>
#include <glm/gtc/type_ptr.hpp>
#include <wren.hpp>
#include <iomanip>
#include <format>

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Debug/Assertions.h"
// ============================================================================
// Math utialaty functions
// ============================================================================

// Math.infinity -> Num
void wren_MathInfinity(WrenVM* vm) {
	wrenSetSlotDouble(vm, 0, std::numeric_limits<double>::infinity());
}

// Math.negInfinity -> Num
void wren_MathNegInfinity(WrenVM* vm) {
	wrenSetSlotDouble(vm, 0, -std::numeric_limits<double>::infinity());
}

// Math.nan -> Num
void wren_MathNaN(WrenVM* vm) {
	wrenSetSlotDouble(vm, 0, std::numeric_limits<double>::quiet_NaN());
}

// Math.pi -> Num
void wren_MathPi(WrenVM* vm) {
	wrenSetSlotDouble(vm, 0, M_PI);
}

// Math.e -> Num
void wren_MathE(WrenVM* vm) {
	wrenSetSlotDouble(vm, 0, M_E);
}

// Math.max(a, b) -> Num
void wren_MathMax(WrenVM* vm) {
	double a = wrenGetSlotDouble(vm, 1);
	double b = wrenGetSlotDouble(vm, 2);
	wrenSetSlotDouble(vm, 0, std::max(a, b));
}

// Math.min(a, b) -> Num
void wren_MathMin(WrenVM* vm) {
	double a = wrenGetSlotDouble(vm, 1);
	double b = wrenGetSlotDouble(vm, 2);
	wrenSetSlotDouble(vm, 0, std::min(a, b));
}

// Math.clamp(value, min, max) -> Num
void wren_MathClamp(WrenVM* vm) {
	double value = wrenGetSlotDouble(vm, 1);
	double min = wrenGetSlotDouble(vm, 2);
	double max = wrenGetSlotDouble(vm, 3);

	if (value < min) value = min;
	if (value > max) value = max;

	wrenSetSlotDouble(vm, 0, value);
}

// Math.abs(n) -> Num
void wren_MathAbs(WrenVM* vm) {
	double n = wrenGetSlotDouble(vm, 1);
	wrenSetSlotDouble(vm, 0, std::abs(n));
}

// Math.sqrt(n) -> Num
void wren_MathSqrt(WrenVM* vm) {
	double n = wrenGetSlotDouble(vm, 1);
	wrenSetSlotDouble(vm, 0, std::sqrt(n));
}

// Math.pow(base, exponent) -> Num
void wren_MathPow(WrenVM* vm) {
	double base = wrenGetSlotDouble(vm, 1);
	double exponent = wrenGetSlotDouble(vm, 2);
	wrenSetSlotDouble(vm, 0, std::pow(base, exponent));
}

// Math.sin(angle) -> Num
void wren_MathSin(WrenVM* vm) {
	double angle = wrenGetSlotDouble(vm, 1);
	wrenSetSlotDouble(vm, 0, std::sin(angle));
}

// Math.cos(angle) -> Num
void wren_MathCos(WrenVM* vm) {
	double angle = wrenGetSlotDouble(vm, 1);
	wrenSetSlotDouble(vm, 0, std::cos(angle));
}

// Math.tan(angle) -> Num
void wren_MathTan(WrenVM* vm) {
	double angle = wrenGetSlotDouble(vm, 1);
	wrenSetSlotDouble(vm, 0, std::tan(angle));
}

// Math.asin(n) -> Num
void wren_MathAsin(WrenVM* vm) {
	double n = wrenGetSlotDouble(vm, 1);
	wrenSetSlotDouble(vm, 0, std::asin(n));
}

// Math.acos(n) -> Num
void wren_MathAcos(WrenVM* vm) {
	double n = wrenGetSlotDouble(vm, 1);
	wrenSetSlotDouble(vm, 0, std::acos(n));
}

// Math.atan(n) -> Num
void wren_MathAtan(WrenVM* vm) {
	double n = wrenGetSlotDouble(vm, 1);
	wrenSetSlotDouble(vm, 0, std::atan(n));
}

// Math.atan2(y, x) -> Num
void wren_MathAtan2(WrenVM* vm) {
	double y = wrenGetSlotDouble(vm, 1);
	double x = wrenGetSlotDouble(vm, 2);
	wrenSetSlotDouble(vm, 0, std::atan2(y, x));
}

// Math.floor(n) -> Num
void wren_MathFloor(WrenVM* vm) {
	double n = wrenGetSlotDouble(vm, 1);
	wrenSetSlotDouble(vm, 0, std::floor(n));
}

// Math.ceil(n) -> Num
void wren_MathCeil(WrenVM* vm) {
	double n = wrenGetSlotDouble(vm, 1);
	wrenSetSlotDouble(vm, 0, std::ceil(n));
}

// Math.round(n) -> Num
void wren_MathRound(WrenVM* vm) {
	double n = wrenGetSlotDouble(vm, 1);
	wrenSetSlotDouble(vm, 0, std::round(n));
}

// Math.lerp(a, b, t) -> Num
void wren_MathLerp(WrenVM* vm) {
	double a = wrenGetSlotDouble(vm, 1);
	double b = wrenGetSlotDouble(vm, 2);
	double t = wrenGetSlotDouble(vm, 3);
	wrenSetSlotDouble(vm, 0, a + (b - a) * t);
}

// Math.isInfinite(n) -> Bool
void wren_MathIsInfinite(WrenVM* vm) {
	double n = wrenGetSlotDouble(vm, 1);
	wrenSetSlotBool(vm, 0, std::isinf(n));
}

// Math.isNaN(n) -> Bool
void wren_MathIsNaN(WrenVM* vm) {
	double n = wrenGetSlotDouble(vm, 1);
	wrenSetSlotBool(vm, 0, std::isnan(n));
}

// Math.isFinite(n) -> Bool
void wren_MathIsFinite(WrenVM* vm) {
	double n = wrenGetSlotDouble(vm, 1);
	wrenSetSlotBool(vm, 0, std::isfinite(n));
}

// Math.sign(n) -> Num
void wren_MathSign(WrenVM* vm) {
	double n = wrenGetSlotDouble(vm, 1);

	if (n > 0) {
		wrenSetSlotDouble(vm, 0, 1.0);
	}
	else if (n < 0) {
		wrenSetSlotDouble(vm, 0, -1.0);
	}
	else {
		wrenSetSlotDouble(vm, 0, 0.0);
	}
}

// Math.radians(degrees) -> Num
void wren_MathRadians(WrenVM* vm) {
	double degrees = wrenGetSlotDouble(vm, 1);
	wrenSetSlotDouble(vm, 0, degrees * M_PI / 180.0);
}

// Math.degrees(radians) -> Num
void wren_MathDegrees(WrenVM* vm) {
	double radians = wrenGetSlotDouble(vm, 1);
	wrenSetSlotDouble(vm, 0, radians * 180.0 / M_PI);
}

// Constants (as static getters)
WREN_CLASS_STATIC("math", "Math", "infinity", wren_MathInfinity, "Positive infinity");
WREN_CLASS_STATIC("math", "Math", "negInfinity", wren_MathNegInfinity, "Negative infinity");
WREN_CLASS_STATIC("math", "Math", "nan", wren_MathNaN, "Not a Number");
WREN_CLASS_STATIC("math", "Math", "pi", wren_MathPi, "Pi constant");
WREN_CLASS_STATIC("math", "Math", "e", wren_MathE, "Euler's number");

// Utility functions
WREN_CLASS_STATIC("math", "Math", "max(_,_)", wren_MathMax, "Maximum of two numbers");
WREN_CLASS_STATIC("math", "Math", "min(_,_)", wren_MathMin, "Minimum of two numbers");
WREN_CLASS_STATIC("math", "Math", "clamp(_,_,_)", wren_MathClamp, "Clamp value between min and max");
WREN_CLASS_STATIC("math", "Math", "abs(_)", wren_MathAbs, "Absolute value");
WREN_CLASS_STATIC("math", "Math", "sqrt(_)", wren_MathSqrt, "Square root");
WREN_CLASS_STATIC("math", "Math", "pow(_,_)", wren_MathPow, "Power");
WREN_CLASS_STATIC("math", "Math", "lerp(_,_,_)", wren_MathLerp, "Linear interpolation");
WREN_CLASS_STATIC("math", "Math", "sign(_)", wren_MathSign, "Sign of number (-1, 0, 1)");

// Trigonometry
WREN_CLASS_STATIC("math", "Math", "sin(_)", wren_MathSin, "Sine");
WREN_CLASS_STATIC("math", "Math", "cos(_)", wren_MathCos, "Cosine");
WREN_CLASS_STATIC("math", "Math", "tan(_)", wren_MathTan, "Tangent");
WREN_CLASS_STATIC("math", "Math", "asin(_)", wren_MathAsin, "Arc sine");
WREN_CLASS_STATIC("math", "Math", "acos(_)", wren_MathAcos, "Arc cosine");
WREN_CLASS_STATIC("math", "Math", "atan(_)", wren_MathAtan, "Arc tangent");
WREN_CLASS_STATIC("math", "Math", "atan2(_,_)", wren_MathAtan2, "Arc tangent of y/x");

// Rounding
WREN_CLASS_STATIC("math", "Math", "floor(_)", wren_MathFloor, "Floor");
WREN_CLASS_STATIC("math", "Math", "ceil(_)", wren_MathCeil, "Ceiling");
WREN_CLASS_STATIC("math", "Math", "round(_)", wren_MathRound, "Round to nearest integer");

// Checks
WREN_CLASS_STATIC("math", "Math", "isInfinite(_)", wren_MathIsInfinite, "Check if infinite");
WREN_CLASS_STATIC("math", "Math", "isNaN(_)", wren_MathIsNaN, "Check if NaN");
WREN_CLASS_STATIC("math", "Math", "isFinite(_)", wren_MathIsFinite, "Check if finite");

// Conversions
WREN_CLASS_STATIC("math", "Math", "radians(_)", wren_MathRadians, "Convert degrees to radians");
WREN_CLASS_STATIC("math", "Math", "degrees(_)", wren_MathDegrees, "Convert radians to degrees");

// ============================================================================
// Vec2 - Foreign class wrapping glm::vec2
// ============================================================================

// Allocator - called when Vec2.new() is invoked
void wren_Vec2Allocate(WrenVM* vm)
{
	// Allocate foreign object
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenVec2));
}

// Finalizer - called when garbage collected
void wren_Vec2Finalize(void* data)
{
	WrenVec2* vec = (WrenVec2*)data;
	vec->~WrenVec2();
}

// Vec2.new(_)
void wren_Vec2New(WrenVM* vm)
{
	WrenVec2* vec = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 0));

	// Initialise with constructor parameters if provided
	if (wrenGetSlotCount(vm) >= 3)
	{
		float x = static_cast<float>(wrenGetSlotDouble(vm, 1));
		float y = static_cast<float>(wrenGetSlotDouble(vm, 2));
		new (vec) WrenVec2(x, y);
	}
	else
	{
		new (vec) WrenVec2();
	}
}

// Getters
void wren_Vec2GetX(WrenVM* vm)
{
	WrenVec2* vec = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotDouble(vm, 0, vec->value.x);
}

void wren_Vec2GetY(WrenVM* vm)
{
	WrenVec2* vec = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotDouble(vm, 0, vec->value.y);
}

// Setters
void wren_Vec2SetX(WrenVM* vm)
{
	WrenVec2* vec = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 0));
	vec->value.x = static_cast<float>(wrenGetSlotDouble(vm, 1));
}

void wren_Vec2SetY(WrenVM* vm)
{
	WrenVec2* vec = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 0));
	vec->value.y = static_cast<float>(wrenGetSlotDouble(vm, 1));
}

// toString()
void wren_Vec2ToString(WrenVM* vm)
{
	WrenVec2* vec = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 0));

	std::string buffer = std::format("Vec2({:.2f}, {:.2f})", vec->value.x, vec->value.y);

	wrenSetSlotString(vm, 0, buffer.c_str());
}

// Math operations
void wren_Vec2Add(WrenVM* vm)
{
	WrenVec2* a = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 0));
	WrenVec2* b = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));

	// Create new Vec2 with result
	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (result) WrenVec2(a->value + b->value);
}

void wren_Vec2Subtract(WrenVM* vm)
{
	WrenVec2* a = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 0));
	WrenVec2* b = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));

	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (result) WrenVec2(a->value - b->value);
}

void wren_Vec2Multiply(WrenVM* vm)
{
	WrenVec2* vec = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 0));
	float scalar = (float)wrenGetSlotDouble(vm, 1);

	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (result) WrenVec2(vec->value * scalar);
}

void wren_Vec2Length(WrenVM* vm)
{
	WrenVec2* vec = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotDouble(vm, 0, glm::length(vec->value));
}

void wren_Vec2LengthSquared(WrenVM* vm) {
	WrenVec2* vec = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotDouble(vm, 0, glm::length2(vec->value));
}

void wren_Vec2Normalize(WrenVM* vm)
{
	WrenVec2* vec = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 0));

	wrenEnsureSlots(vm, 2);
	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (result) WrenVec2(glm::normalize(vec->value));
}

// Vec2.distance(a, b) -> Num
void wren_Vec2Distance(WrenVM* vm)
{
	WrenVec2* a = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* b = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));

	wrenSetSlotDouble(vm, 0, glm::distance(a->value, b->value));
}

// Vec2.distanceSquared(a, b) -> Num
void wren_Vec2DistanceSquared(WrenVM* vm)
{
	WrenVec2* a = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* b = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));

	wrenSetSlotDouble(vm, 0, glm::distance2(a->value, b->value));
}

// Vec2.lerp(a, b, t) -> Vec2
void wren_Vec2Lerp(WrenVM* vm)
{
	WrenVec2* a = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* b = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
	float t = (float)wrenGetSlotDouble(vm, 3);

	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (result) WrenVec2(glm::mix(a->value, b->value, t));
}

// Vec2.dot(a, b) -> Num
void wren_Vec2DotStatic(WrenVM* vm)
{
	WrenVec2* a = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* b = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));

	wrenSetSlotDouble(vm, 0, glm::dot(a->value, b->value));
}

// Vec2.reflect(incident, normal) -> Vec2
void wren_Vec2Reflect(WrenVM* vm)
{
	WrenVec2* incident = (WrenVec2*)wrenGetSlotForeign(vm, 1);
	WrenVec2* normal = (WrenVec2*)wrenGetSlotForeign(vm, 2);

	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (result) WrenVec2(glm::reflect(incident->value, normal->value));
}

// Vec2.min(a, b) -> Vec2
void wren_Vec2Min(WrenVM* vm)
{
	WrenVec2* a = (WrenVec2*)wrenGetSlotForeign(vm, 1);
	WrenVec2* b = (WrenVec2*)wrenGetSlotForeign(vm, 2);

	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (result) WrenVec2(glm::min(a->value, b->value));
}

// Vec2.max(a, b) -> Vec2
void wren_Vec2Max(WrenVM* vm)
{
	WrenVec2* a = (WrenVec2*)wrenGetSlotForeign(vm, 1);
	WrenVec2* b = (WrenVec2*)wrenGetSlotForeign(vm, 2);

	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (result) WrenVec2(glm::max(a->value, b->value));
}

// Vec2.clamp(v, min, max) -> Vec2
void wren_Vec2Clamp(WrenVM* vm)
{
	WrenVec2* v = (WrenVec2*)wrenGetSlotForeign(vm, 1);
	WrenVec2* minVec = (WrenVec2*)wrenGetSlotForeign(vm, 2);
	WrenVec2* maxVec = (WrenVec2*)wrenGetSlotForeign(vm, 3);

	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (result) WrenVec2(glm::clamp(v->value, minVec->value, maxVec->value));
}

// Vec2.zero() -> Vec2
void wren_Vec2Zero(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (result) WrenVec2(0.0f, 0.0f);
}

// Vec2.one() -> Vec2
void wren_Vec2One(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (result) WrenVec2(1.0f, 1.0f);
}

// Vec2.right() -> Vec2
void wren_Vec2Right(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (result) WrenVec2(1.0f, 0.0f);
}

// Vec2.up() -> Vec2
void wren_Vec2Up(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (result) WrenVec2(0.0f, 1.0f);
}

// Vec2.left() -> Vec2
void wren_Vec2Left(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (result) WrenVec2(-1.0f, 0.0f);
}

// Vec2.down() -> Vec2
void wren_Vec2Down(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec2", 0);  // Get class into slot 1
	WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenVec2));
	new (result) WrenVec2(0.0f, -1.0f);
}

// Register Vec2 foreign class
WREN_FOREIGN_CLASS("math", "Vec2", wren_Vec2Allocate, wren_Vec2Finalize, "2D vector class wrapping glm::vec2");

// Register constructors
WREN_CONSTRUCTOR("math", "Vec2", "new()", wren_Vec2New, "Create zero vector");
WREN_CONSTRUCTOR("math", "Vec2", "new(_,_)", wren_Vec2New, "Create vector with x, y components");

// Register methods
WREN_CLASS_METHOD("math", "Vec2", "x", wren_Vec2GetX, "Get X component");
WREN_CLASS_METHOD("math", "Vec2", "y", wren_Vec2GetY, "Get Y component");
WREN_CLASS_METHOD("math", "Vec2", "x=(_)", wren_Vec2SetX, "Set X component");
WREN_CLASS_METHOD("math", "Vec2", "y=(_)", wren_Vec2SetY, "Set Y component");
WREN_CLASS_METHOD("math", "Vec2", "toString", wren_Vec2ToString, "Convert to string");
WREN_CLASS_METHOD("math", "Vec2", "+(_)", wren_Vec2Add, "Add two vectors");
WREN_CLASS_METHOD("math", "Vec2", "-(_)", wren_Vec2Subtract, "Subtract vectors");
WREN_CLASS_METHOD("math", "Vec2", "*(_)", wren_Vec2Multiply, "Multiply by scalar");
WREN_CLASS_METHOD("math", "Vec2", "length()", wren_Vec2Length, "Get vector length");
WREN_CLASS_METHOD("math", "Vec2", "lengthSquared()", wren_Vec2LengthSquared, "Get vector length squared");
WREN_CLASS_METHOD("math", "Vec2", "normalize()", wren_Vec2Normalize, "Get normalized vector");

// Static methods
WREN_CLASS_STATIC("math", "Vec2", "distance(_,_)", wren_Vec2Distance, "Distance between two vectors");
WREN_CLASS_STATIC("math", "Vec2", "distanceSquared(_,_)", wren_Vec2DistanceSquared, "Squared distance");
WREN_CLASS_STATIC("math", "Vec2", "lerp(_,_,_)", wren_Vec2Lerp, "Linear interpolation");
WREN_CLASS_STATIC("math", "Vec2", "dot(_,_)", wren_Vec2DotStatic, "Dot product (static)");
WREN_CLASS_STATIC("math", "Vec2", "reflect(_,_)", wren_Vec2Reflect, "Reflect vector");
WREN_CLASS_STATIC("math", "Vec2", "min(_,_)", wren_Vec2Min, "Component-wise minimum");
WREN_CLASS_STATIC("math", "Vec2", "max(_,_)", wren_Vec2Max, "Component-wise maximum");
WREN_CLASS_STATIC("math", "Vec2", "clamp(_,_,_)", wren_Vec2Clamp, "Clamp vector");
WREN_CLASS_STATIC("math", "Vec2", "zero()", wren_Vec2Zero, "Zero vector (0, 0)");
WREN_CLASS_STATIC("math", "Vec2", "one()", wren_Vec2One, "One vector (1, 1)");
WREN_CLASS_STATIC("math", "Vec2", "right()", wren_Vec2Right, "Right vector (1, 0)");
WREN_CLASS_STATIC("math", "Vec2", "up()", wren_Vec2Up, "Up vector (0, 1)");
WREN_CLASS_STATIC("math", "Vec2", "left()", wren_Vec2Left, "Left vector (-1, 0)");
WREN_CLASS_STATIC("math", "Vec2", "down()", wren_Vec2Down, "Down vector (0, -1)");


// ============================================================================
// Vec3 - Foreign class wrapping glm::vec3
// ============================================================================

void wren_Vec3Allocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenVec3));
}

void wren_Vec3Finalize(void* data)
{
	WrenVec3* vec = (WrenVec3*)data;
	vec->~WrenVec3();
}

// Vec3.new(_)
void wren_Vec3New(WrenVM* vm)
{
	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 0));

	// Initialise with constructor parameters if provided
	if (wrenGetSlotCount(vm) >= 3)
	{
		float x = (float)wrenGetSlotDouble(vm, 1);
		float y = (float)wrenGetSlotDouble(vm, 2);
		float z = (float)wrenGetSlotDouble(vm, 3);
		new (vec) WrenVec3(x, y, z);
	}
	else
	{
		new (vec) WrenVec3();
	}
}

void wren_Vec3GetX(WrenVM* vm)
{
	WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, vec->value.x);
}

void wren_Vec3GetY(WrenVM* vm)
{
	WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, vec->value.y);
}

void wren_Vec3GetZ(WrenVM* vm)
{
	WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, vec->value.z);
}

void wren_Vec3SetX(WrenVM* vm)
{
	WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
	vec->value.x = (float)wrenGetSlotDouble(vm, 1);
}

void wren_Vec3SetY(WrenVM* vm)
{
	WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
	vec->value.y = (float)wrenGetSlotDouble(vm, 1);
}

void wren_Vec3SetZ(WrenVM* vm)
{
	WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
	vec->value.z = (float)wrenGetSlotDouble(vm, 1);
}

void wren_Vec3ToString(WrenVM* vm)
{
	WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);

	std::string buffer = std::format("Vec3({:.2f}, {:.2f}, {:.2f})", vec->value.x, vec->value.y, vec->value.z);

	wrenSetSlotString(vm, 0, buffer.c_str());
}

void wren_Vec3Add(WrenVM* vm)
{
	WrenVec3* a = (WrenVec3*)wrenGetSlotForeign(vm, 0);
	WrenVec3* b = (WrenVec3*)wrenGetSlotForeign(vm, 1);

	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(a->value + b->value);
}

void wren_Vec3Subtract(WrenVM* vm)
{
	WrenVec3* a = (WrenVec3*)wrenGetSlotForeign(vm, 0);
	WrenVec3* b = (WrenVec3*)wrenGetSlotForeign(vm, 1);

	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(a->value - b->value);
}

void wren_Vec3Multiply(WrenVM* vm)
{
	WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
	float scalar = (float)wrenGetSlotDouble(vm, 1);

	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(vec->value * scalar);
}

void wren_Vec3Length(WrenVM* vm)
{
	WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, glm::length(vec->value));
}

void wren_Vec3LengthSquared(WrenVM* vm)
{
	WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, glm::length2(vec->value));
}

void wren_Vec3Normalize(WrenVM* vm)
{
	WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);

	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(glm::normalize(vec->value));
}


// Vec3.distance(a, b) -> Num
void wren_Vec3Distance(WrenVM* vm)
{
	WrenVec3* a = (WrenVec3*)wrenGetSlotForeign(vm, 1);
	WrenVec3* b = (WrenVec3*)wrenGetSlotForeign(vm, 2);

	wrenSetSlotDouble(vm, 0, glm::distance(a->value, b->value));
}

// Vec3.distanceSquared(a, b) -> Num
void wren_Vec3DistanceSquared(WrenVM* vm)
{
	WrenVec3* a = (WrenVec3*)wrenGetSlotForeign(vm, 1);
	WrenVec3* b = (WrenVec3*)wrenGetSlotForeign(vm, 2);

	wrenSetSlotDouble(vm, 0, glm::distance2(a->value, b->value));
}

// Vec3.lerp(a, b, t) -> Vec3
void wren_Vec3Lerp(WrenVM* vm)
{
	WrenVec3* a = (WrenVec3*)wrenGetSlotForeign(vm, 1);
	WrenVec3* b = (WrenVec3*)wrenGetSlotForeign(vm, 2);
	float t = (float)wrenGetSlotDouble(vm, 3);

	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(glm::mix(a->value, b->value, t));
}

// Vec3.dot(a, b) -> Num
void wren_Vec3DotStatic(WrenVM* vm)
{
	WrenVec3* a = (WrenVec3*)wrenGetSlotForeign(vm, 1);
	WrenVec3* b = (WrenVec3*)wrenGetSlotForeign(vm, 2);

	wrenSetSlotDouble(vm, 0, glm::dot(a->value, b->value));
}

// Vec3.cross(a, b) -> Vec3
void wren_Vec3CrossStatic(WrenVM* vm)
{
	WrenVec3* a = (WrenVec3*)wrenGetSlotForeign(vm, 1);
	WrenVec3* b = (WrenVec3*)wrenGetSlotForeign(vm, 2);

	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(glm::cross(a->value, b->value));
}

// Vec3.reflect(incident, normal) -> Vec3
void wren_Vec3Reflect(WrenVM* vm)
{
	WrenVec3* incident = (WrenVec3*)wrenGetSlotForeign(vm, 1);
	WrenVec3* normal = (WrenVec3*)wrenGetSlotForeign(vm, 2);

	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(glm::reflect(incident->value, normal->value));
}

// Vec3.refract(incident, normal, eta) -> Vec3
void wren_Vec3Refract(WrenVM* vm)
{
	WrenVec3* incident = (WrenVec3*)wrenGetSlotForeign(vm, 1);
	WrenVec3* normal = (WrenVec3*)wrenGetSlotForeign(vm, 2);
	float eta = (float)wrenGetSlotDouble(vm, 3);

	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(glm::refract(incident->value, normal->value, eta));
}

// Vec3.min(a, b) -> Vec3
void wren_Vec3Min(WrenVM* vm)
{
	WrenVec3* a = (WrenVec3*)wrenGetSlotForeign(vm, 1);
	WrenVec3* b = (WrenVec3*)wrenGetSlotForeign(vm, 2);

	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(glm::min(a->value, b->value));
}

// Vec3.max(a, b) -> Vec3
void wren_Vec3Max(WrenVM* vm)
{
	WrenVec3* a = (WrenVec3*)wrenGetSlotForeign(vm, 1);
	WrenVec3* b = (WrenVec3*)wrenGetSlotForeign(vm, 2);

	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(glm::max(a->value, b->value));
}

// Vec3.clamp(v, min, max) -> Vec3
void wren_Vec3Clamp(WrenVM* vm)
{
	WrenVec3* v = (WrenVec3*)wrenGetSlotForeign(vm, 1);
	WrenVec3* minVec = (WrenVec3*)wrenGetSlotForeign(vm, 2);
	WrenVec3* maxVec = (WrenVec3*)wrenGetSlotForeign(vm, 3);

	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(glm::clamp(v->value, minVec->value, maxVec->value));
}

// Vec3.zero() -> Vec3
void wren_Vec3Zero(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(0.0f, 0.0f, 0.0f);
}

// Vec3.one() -> Vec3
void wren_Vec3One(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(1.0f, 1.0f, 1.0f);
}

// Vec3.right() -> Vec3
void wren_Vec3Right(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(1.0f, 0.0f, 0.0f);
}

// Vec3.up() -> Vec3
void wren_Vec3Up(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(0.0f, 1.0f, 0.0f);
}

// Vec3.forward() -> Vec3
void wren_Vec3Forward(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(0.0f, 0.0f, -1.0f);
}

// Vec3.left() -> Vec3
void wren_Vec3Left(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(-1.0f, 0.0f, 0.0f);
}

// Vec3.down() -> Vec3
void wren_Vec3Down(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(0.0f, -1.0f, 0.0f);
}

// Vec3.back() -> Vec3
void wren_Vec3Back(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec3", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(0.0f, 0.0f, 1.0f);
}

// Register Vec3 foreign class
WREN_FOREIGN_CLASS("math", "Vec3", wren_Vec3Allocate, wren_Vec3Finalize, "3D vector class wrapping glm::vec3");

// Register constructors
WREN_CONSTRUCTOR("math", "Vec3", "new()", wren_Vec3New, "Create zero vector");
WREN_CONSTRUCTOR("math", "Vec3", "new(_,_,_)", wren_Vec3New, "Create vector with x, y, z components");

// Register methods
WREN_CLASS_METHOD("math", "Vec3", "x", wren_Vec3GetX, "Get X component");
WREN_CLASS_METHOD("math", "Vec3", "y", wren_Vec3GetY, "Get Y component");
WREN_CLASS_METHOD("math", "Vec3", "z", wren_Vec3GetZ, "Get Z component");
WREN_CLASS_METHOD("math", "Vec3", "x=(_)", wren_Vec3SetX, "Set X component");
WREN_CLASS_METHOD("math", "Vec3", "y=(_)", wren_Vec3SetY, "Set Y component");
WREN_CLASS_METHOD("math", "Vec3", "z=(_)", wren_Vec3SetZ, "Set Z component");
WREN_CLASS_METHOD("math", "Vec3", "toString()", wren_Vec3ToString, "Convert to string");
WREN_CLASS_METHOD("math", "Vec3", "+(_)", wren_Vec3Add, "Add two vectors");
WREN_CLASS_METHOD("math", "Vec3", "-(_)", wren_Vec3Subtract, "Subtract vectors");
WREN_CLASS_METHOD("math", "Vec3", "*(_)", wren_Vec3Multiply, "Multiply by scalar");
WREN_CLASS_METHOD("math", "Vec3", "length()", wren_Vec3Length, "Get vector length");
WREN_CLASS_METHOD("math", "Vec3", "lengthSquared()", wren_Vec3LengthSquared, "Get vector length squared");
WREN_CLASS_METHOD("math", "Vec3", "normalize", wren_Vec3Normalize, "Get normalized vector");

// Static methods
WREN_CLASS_STATIC("math", "Vec3", "distance(_,_)", wren_Vec3Distance, "Distance between two vectors");
WREN_CLASS_STATIC("math", "Vec3", "distanceSquared(_,_)", wren_Vec3DistanceSquared, "Squared distance");
WREN_CLASS_STATIC("math", "Vec3", "lerp(_,_,_)", wren_Vec3Lerp, "Linear interpolation");
WREN_CLASS_STATIC("math", "Vec3", "dot(_,_)", wren_Vec3DotStatic, "Dot product (static)");
WREN_CLASS_STATIC("math", "Vec3", "cross(_,_)", wren_Vec3CrossStatic, "Cross product (static)");
WREN_CLASS_STATIC("math", "Vec3", "reflect(_,_)", wren_Vec3Reflect, "Reflect vector");
WREN_CLASS_STATIC("math", "Vec3", "refract(_,_,_)", wren_Vec3Refract, "Refract vector");
WREN_CLASS_STATIC("math", "Vec3", "min(_,_)", wren_Vec3Min, "Component-wise minimum");
WREN_CLASS_STATIC("math", "Vec3", "max(_,_)", wren_Vec3Max, "Component-wise maximum");
WREN_CLASS_STATIC("math", "Vec3", "clamp(_,_,_)", wren_Vec3Clamp, "Clamp vector");
WREN_CLASS_STATIC("math", "Vec3", "zero()", wren_Vec3Zero, "Zero vector (0, 0, 0)");
WREN_CLASS_STATIC("math", "Vec3", "one()", wren_Vec3One, "One vector (1, 1, 1)");
WREN_CLASS_STATIC("math", "Vec3", "right()", wren_Vec3Right, "Right vector (1, 0, 0)");
WREN_CLASS_STATIC("math", "Vec3", "up()", wren_Vec3Up, "Up vector (0, 1, 0)");
WREN_CLASS_STATIC("math", "Vec3", "forward()", wren_Vec3Forward, "Forward vector (0, 0, -1)");
WREN_CLASS_STATIC("math", "Vec3", "left()", wren_Vec3Left, "Left vector (-1, 0, 0)");
WREN_CLASS_STATIC("math", "Vec3", "down()", wren_Vec3Down, "Down vector (0, -1, 0)");
WREN_CLASS_STATIC("math", "Vec3", "back()", wren_Vec3Back, "Back vector (0, 0, 1)");

// ============================================================================
// Vec4 - Foreign class wrapping glm::vec4
// ============================================================================

void wren_Vec4Allocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenVec4));
}

void wren_Vec4Finalize(void* data)
{
	WrenVec4* vec = (WrenVec4*)data;
	vec->~WrenVec4();
}

// Vec4.new(_)
void wren_Vec4New(WrenVM* vm)
{
	WrenVec4* vec = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 0));

	// Initialise with constructor parameters if provided
	if (wrenGetSlotCount(vm) >= 5)
	{
		float x = static_cast<int>(wrenGetSlotDouble(vm, 1));
		float y = static_cast<int>(wrenGetSlotDouble(vm, 2));
		float z = static_cast<int>(wrenGetSlotDouble(vm, 3));
		float w = static_cast<int>(wrenGetSlotDouble(vm, 4));
		new (vec) WrenVec4(x, y, z, w);
	}
	else
	{
		new (vec) WrenVec4();
	}
}

void wren_Vec4GetX(WrenVM* vm)
{
	WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, vec->value.x);
}

void wren_Vec4GetY(WrenVM* vm)
{
	WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, vec->value.y);
}

void wren_Vec4GetZ(WrenVM* vm)
{
	WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, vec->value.z);
}

void wren_Vec4GetW(WrenVM* vm)
{
	WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, vec->value.w);
}

void wren_Vec4SetX(WrenVM* vm)
{
	WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
	vec->value.x = (float)wrenGetSlotDouble(vm, 1);
}

void wren_Vec4SetY(WrenVM* vm)
{
	WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
	vec->value.y = (float)wrenGetSlotDouble(vm, 1);
}

void wren_Vec4SetZ(WrenVM* vm)
{
	WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
	vec->value.z = (float)wrenGetSlotDouble(vm, 1);
}

void wren_Vec4SetW(WrenVM* vm)
{
	WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
	vec->value.w = (float)wrenGetSlotDouble(vm, 1);
}

void wren_Vec4ToString(WrenVM* vm)
{
	WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);

	std::string buffer = std::format("Vec4({:.2f}, {:.2f}, {:.2f}, {:.2f})", vec->value.x, vec->value.y, vec->value.z, vec->value.w);

	wrenSetSlotString(vm, 0, buffer.c_str());
}

void wren_Vec4Add(WrenVM* vm)
{
	WrenVec4* a = (WrenVec4*)wrenGetSlotForeign(vm, 0);
	WrenVec4* b = (WrenVec4*)wrenGetSlotForeign(vm, 1);

	wrenGetVariable(vm, "math", "Vec4", 1);  // Get class into slot 1
	WrenVec4* result = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
	new (result) WrenVec4(a->value + b->value);
}

void wren_Vec4Subtract(WrenVM* vm)
{
	WrenVec4* a = (WrenVec4*)wrenGetSlotForeign(vm, 0);
	WrenVec4* b = (WrenVec4*)wrenGetSlotForeign(vm, 1);

	wrenGetVariable(vm, "math", "Vec4", 1);  // Get class into slot 1
	WrenVec4* result = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
	new (result) WrenVec4(a->value - b->value);
}

void wren_Vec4Multiply(WrenVM* vm)
{
	WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
	float scalar = (float)wrenGetSlotDouble(vm, 1);

	wrenGetVariable(vm, "math", "Vec4", 1);  // Get class into slot 1
	WrenVec4* result = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
	new (result) WrenVec4(vec->value * scalar);
}

void wren_Vec4Length(WrenVM* vm)
{
	WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, glm::length(vec->value));
}

void wren_Vec4LengthSquared(WrenVM* vm)
{
	WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, glm::length2(vec->value));
}

void wren_Vec4Normalize(WrenVM* vm)
{
	WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);

	wrenGetVariable(vm, "math", "Vec4", 1);  // Get class into slot 1
	WrenVec4* result = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
	new (result) WrenVec4(glm::normalize(vec->value));
}


// Vec4.distance(a, b) -> Num
void wren_Vec4Distance(WrenVM* vm)
{
	WrenVec4* a = (WrenVec4*)wrenGetSlotForeign(vm, 1);
	WrenVec4* b = (WrenVec4*)wrenGetSlotForeign(vm, 2);

	wrenSetSlotDouble(vm, 0, glm::distance(a->value, b->value));
}

// Vec4.distanceSquared(a, b) -> Num
void wren_Vec4DistanceSquared(WrenVM* vm)
{
	WrenVec4* a = (WrenVec4*)wrenGetSlotForeign(vm, 1);
	WrenVec4* b = (WrenVec4*)wrenGetSlotForeign(vm, 2);

	wrenSetSlotDouble(vm, 0, glm::distance2(a->value, b->value));
}

// Vec4.lerp(a, b, t) -> Vec4
void wren_Vec4Lerp(WrenVM* vm)
{
	WrenVec4* a = (WrenVec4*)wrenGetSlotForeign(vm, 1);
	WrenVec4* b = (WrenVec4*)wrenGetSlotForeign(vm, 2);
	float t = (float)wrenGetSlotDouble(vm, 3);

	wrenGetVariable(vm, "math", "Vec4", 1);  // Get class into slot 1
	WrenVec4* result = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
	new (result) WrenVec4(glm::mix(a->value, b->value, t));
}

// Vec4.dot(a, b) -> Num
void wren_Vec4DotStatic(WrenVM* vm)
{
	WrenVec4* a = (WrenVec4*)wrenGetSlotForeign(vm, 1);
	WrenVec4* b = (WrenVec4*)wrenGetSlotForeign(vm, 2);

	wrenSetSlotDouble(vm, 0, glm::dot(a->value, b->value));
}

// Vec4.min(a, b) -> Vec4
void wren_Vec4Min(WrenVM* vm)
{
	WrenVec4* a = (WrenVec4*)wrenGetSlotForeign(vm, 1);
	WrenVec4* b = (WrenVec4*)wrenGetSlotForeign(vm, 2);

	wrenGetVariable(vm, "math", "Vec4", 1);  // Get class into slot 1
	WrenVec4* result = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
	new (result) WrenVec4(glm::min(a->value, b->value));
}

// Vec4.max(a, b) -> Vec4
void wren_Vec4Max(WrenVM* vm)
{
	WrenVec4* a = (WrenVec4*)wrenGetSlotForeign(vm, 1);
	WrenVec4* b = (WrenVec4*)wrenGetSlotForeign(vm, 2);

	wrenGetVariable(vm, "math", "Vec4", 1);  // Get class into slot 1
	WrenVec4* result = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
	new (result) WrenVec4(glm::max(a->value, b->value));
}

// Vec4.clamp(v, min, max) -> Vec4
void wren_Vec4Clamp(WrenVM* vm)
{
	WrenVec4* v = (WrenVec4*)wrenGetSlotForeign(vm, 1);
	WrenVec4* minVec = (WrenVec4*)wrenGetSlotForeign(vm, 2);
	WrenVec4* maxVec = (WrenVec4*)wrenGetSlotForeign(vm, 3);

	wrenGetVariable(vm, "math", "Vec4", 1);  // Get class into slot 1
	WrenVec4* result = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
	new (result) WrenVec4(glm::clamp(v->value, minVec->value, maxVec->value));
}

// Vec4.zero() -> Vec4
void wren_Vec4Zero(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec4", 1);  // Get class into slot 1
	WrenVec4* result = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
	new (result) WrenVec4(0.0f, 0.0f, 0.0f, 0.0f);
}

// Vec4.one() -> Vec4
void wren_Vec4One(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Vec4", 1);  // Get class into slot 1
	WrenVec4* result = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
	new (result) WrenVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

// Register Vec3 foreign class
WREN_FOREIGN_CLASS("math", "Vec4", wren_Vec4Allocate, wren_Vec4Finalize, "4D vector class wrapping glm::vec4");

// Register constructors
WREN_CONSTRUCTOR("math", "Vec4", "new()", wren_Vec4New, "Create zero vector");
WREN_CONSTRUCTOR("math", "Vec4", "new(_,_,_,_)", wren_Vec4New, "Create vector with x, y, z, w components");

// Register methods
WREN_CLASS_METHOD("math", "Vec4", "x", wren_Vec4GetX, "Get X component");
WREN_CLASS_METHOD("math", "Vec4", "y", wren_Vec4GetY, "Get Y component");
WREN_CLASS_METHOD("math", "Vec4", "z", wren_Vec4GetZ, "Get Z component");
WREN_CLASS_METHOD("math", "Vec4", "w", wren_Vec4GetW, "Get W component");
WREN_CLASS_METHOD("math", "Vec4", "x=(_)", wren_Vec4SetX, "Set X component");
WREN_CLASS_METHOD("math", "Vec4", "y=(_)", wren_Vec4SetY, "Set Y component");
WREN_CLASS_METHOD("math", "Vec4", "z=(_)", wren_Vec4SetZ, "Set Z component");
WREN_CLASS_METHOD("math", "Vec4", "w=(_)", wren_Vec4SetW, "Set W component");
WREN_CLASS_METHOD("math", "Vec4", "toString()", wren_Vec4ToString, "Convert to string");
WREN_CLASS_METHOD("math", "Vec4", "+(_)", wren_Vec4Add, "Add two vectors");
WREN_CLASS_METHOD("math", "Vec4", "-(_)", wren_Vec4Subtract, "Subtract vectors");
WREN_CLASS_METHOD("math", "Vec4", "*(_)", wren_Vec4Multiply, "Multiply by scalar");
WREN_CLASS_METHOD("math", "Vec4", "length()", wren_Vec4Length, "Get vector length");
WREN_CLASS_METHOD("math", "Vec4", "lengthSquared()", wren_Vec4LengthSquared, "Get vector length squared");
WREN_CLASS_METHOD("math", "Vec4", "normalize()", wren_Vec4Normalize, "Get normalized vector");

// Static methods
WREN_CLASS_STATIC("math", "Vec4", "distance(_,_)", wren_Vec4Distance, "Distance between two vectors");
WREN_CLASS_STATIC("math", "Vec4", "distanceSquared(_,_)", wren_Vec4DistanceSquared, "Squared distance");
WREN_CLASS_STATIC("math", "Vec4", "lerp(_,_,_)", wren_Vec4Lerp, "Linear interpolation");
WREN_CLASS_STATIC("math", "Vec4", "dot(_,_)", wren_Vec4DotStatic, "Dot product (static)");
WREN_CLASS_STATIC("math", "Vec4", "min(_,_)", wren_Vec4Min, "Component-wise minimum");
WREN_CLASS_STATIC("math", "Vec4", "max(_,_)", wren_Vec4Max, "Component-wise maximum");
WREN_CLASS_STATIC("math", "Vec4", "clamp(_,_,_)", wren_Vec4Clamp, "Clamp vector");
WREN_CLASS_STATIC("math", "Vec4", "zero()", wren_Vec4Zero, "Zero vector (0, 0, 0, 0)");
WREN_CLASS_STATIC("math", "Vec4", "one()", wren_Vec4One, "One vector (1, 1, 1, 1)");

// ============================================================================
// Quat - Foreign class wrapping glm::quat
// ============================================================================

// Allocator
void wren_QuatAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenQuat));
}

// Finalizer
void wren_QuatFinalize(void* data)
{
	WrenQuat* quat = (WrenQuat*)data;
	quat->~WrenQuat();
}

// Quat.new(_)
void wren_QuatNew(WrenVM* vm)
{
	WrenQuat* quat = static_cast<WrenQuat*>(wrenGetSlotForeign(vm, 0));

	// Initialise with constructor parameters if provided
	if (wrenGetSlotCount(vm) >= 5)
	{
		// Quat.new(w, x, y, z)
		float w = (float)wrenGetSlotDouble(vm, 1);
		float x = (float)wrenGetSlotDouble(vm, 2);
		float y = (float)wrenGetSlotDouble(vm, 3);
		float z = (float)wrenGetSlotDouble(vm, 4);
		new (quat) WrenQuat(w, x, y, z);
	}
	else {
		// Quat.new() - identity
		new (quat) WrenQuat();
	}
}

// Component getters
void wren_QuatGetW(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, quat->value.w);
}

void wren_QuatGetX(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, quat->value.x);
}

void wren_QuatGetY(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, quat->value.y);
}

void wren_QuatGetZ(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, quat->value.z);
}

// Component setters
void wren_QuatSetW(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
	quat->value.w = (float)wrenGetSlotDouble(vm, 1);
}

void wren_QuatSetX(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
	quat->value.x = (float)wrenGetSlotDouble(vm, 1);
}

void wren_QuatSetY(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
	quat->value.y = (float)wrenGetSlotDouble(vm, 1);
}

void wren_QuatSetZ(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
	quat->value.z = (float)wrenGetSlotDouble(vm, 1);
}

// toString
void wren_QuatToString(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);

	std::string buffer = std::format("Quat({:.2f}, {:.2f}, {:.2f}, {:.2f})", quat->value.w, quat->value.x, quat->value.y, quat->value.z);

	wrenSetSlotString(vm, 0, buffer.c_str());
}

// Static constructors for common rotations

// Quat.identity() -> Quat
void wren_QuatIdentity(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(1.0f, 0.0f, 0.0f, 0.0f);
}

// Quat.fromAxisAngle(axis, angle) -> Quat
void wren_QuatFromAxisAngle(WrenVM* vm)
{
	WrenVec3* axis = (WrenVec3*)wrenGetSlotForeign(vm, 1);
	float angle = (float)wrenGetSlotDouble(vm, 2);

	if (!axis)
	{
		DEBUG_ERROR("Quat.fromAxisAngle: Invalid axis");
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(glm::angleAxis(angle, glm::normalize(axis->value)));
}

// Quat.fromEuler(x, y, z) -> Quat (angles in radians)
void wren_QuatFromEuler(WrenVM* vm)
{
	float x = (float)wrenGetSlotDouble(vm, 1);
	float y = (float)wrenGetSlotDouble(vm, 2);
	float z = (float)wrenGetSlotDouble(vm, 3);

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(glm::quat(glm::vec3(x, y, z)));
}

// Quat.fromEulerDegrees(x, y, z) -> Quat (angles in degrees)
void wren_QuatFromEulerDegrees(WrenVM* vm)
{
	float x = glm::radians((float)wrenGetSlotDouble(vm, 1));
	float y = glm::radians((float)wrenGetSlotDouble(vm, 2));
	float z = glm::radians((float)wrenGetSlotDouble(vm, 3));

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(glm::quat(glm::vec3(x, y, z)));
}

// Quat.lookAt(forward, up) -> Quat
void wren_QuatLookAt(WrenVM* vm)
{
	WrenVec3* forward = (WrenVec3*)wrenGetSlotForeign(vm, 1);
	WrenVec3* up = (WrenVec3*)wrenGetSlotForeign(vm, 2);

	if (!forward || !up)
	{
		DEBUG_ERROR("Quat.lookAt: Invalid vectors");
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(glm::quatLookAt(glm::normalize(forward->value), up->value));
}

// Instance methods

// quat.normalize() -> Quat
void wren_QuatNormalize(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* result = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (result) WrenQuat(glm::normalize(quat->value));
}

// quat.inverse() -> Quat
void wren_QuatInverse(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* result = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (result) WrenQuat(glm::inverse(quat->value));
}

// quat.conjugate() -> Quat
void wren_QuatConjugate(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* result = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (result) WrenQuat(glm::conjugate(quat->value));
}

// quat.length -> Num
void wren_QuatLength(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, glm::length(quat->value));
}

// quat.lengthSquared -> Num
void wren_QuatLengthSquared(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, glm::length2(quat->value));
}

// quat.toEuler() -> Vec3 (radians)
void wren_QuatToEuler(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);

	glm::vec3 euler = glm::eulerAngles(quat->value);

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(euler);
}

// quat.toEulerDegrees() -> Vec3 (degrees)
void wren_QuatToEulerDegrees(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);

	glm::vec3 euler = glm::eulerAngles(quat->value);
	euler = glm::degrees(euler);

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(euler);
}

// quat.toAxisAngle() -> Map {"axis": Vec3, "angle": Num}
void wren_QuatToAxisAngle(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);

	float angle = glm::angle(quat->value);
	glm::vec3 axis = glm::axis(quat->value);

	// Create map
	wrenSetSlotNewMap(vm, 0);

	// Add axis
	wrenSetSlotString(vm, 1, "axis");
	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenVec3* axisVec = (WrenVec3*)wrenSetSlotNewForeign(vm, 2, 1, sizeof(WrenVec3));
	new (axisVec) WrenVec3(axis);
	wrenSetMapValue(vm, 0, 1, 2);

	// Add angle
	wrenSetSlotString(vm, 1, "angle");
	wrenSetSlotDouble(vm, 2, angle);
	wrenSetMapValue(vm, 0, 1, 2);
}

// Operators

// quat * quat -> Quat (quaternion multiplication)
void wren_QuatMultiply(WrenVM* vm)
{
	WrenQuat* a = (WrenQuat*)wrenGetSlotForeign(vm, 0);
	WrenQuat* b = (WrenQuat*)wrenGetSlotForeign(vm, 1);

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* result = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (result) WrenQuat(a->value * b->value);
}

// quat.rotate(vec3) -> Vec3 (rotate vector by quaternion)
void wren_QuatRotateVec3(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
	WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 1);

	if (!vec)
	{
		DEBUG_ERROR("Quat.rotate: Invalid Vec3");
		wrenSetSlotNull(vm, 0);
		return;
	}

	glm::vec3 rotated = quat->value * vec->value;

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(rotated);
}

// Quat.slerp(a, b, t) -> Quat (spherical linear interpolation)
void wren_QuatSlerp(WrenVM* vm)
{
	WrenQuat* a = (WrenQuat*)wrenGetSlotForeign(vm, 1);
	WrenQuat* b = (WrenQuat*)wrenGetSlotForeign(vm, 2);
	float t = (float)wrenGetSlotDouble(vm, 3);

	if (!a || !b)
	{
		DEBUG_ERROR("Quat.slerp: Invalid quaternions");
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* result = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (result) WrenQuat(glm::slerp(a->value, b->value, t));
}

// Quat.lerp(a, b, t) -> Quat (linear interpolation)
void wren_QuatLerp(WrenVM* vm)
{
	WrenQuat* a = (WrenQuat*)wrenGetSlotForeign(vm, 1);
	WrenQuat* b = (WrenQuat*)wrenGetSlotForeign(vm, 2);
	float t = (float)wrenGetSlotDouble(vm, 3);

	if (!a || !b)
	{
		DEBUG_ERROR("Quat.lerp: Invalid quaternions");
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenQuat* result = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (result) WrenQuat(glm::lerp(a->value, b->value, t));
}

// Quat.dot(a,b) -> Num
void wren_QuatDot(WrenVM* vm)
{
	WrenQuat* a = (WrenQuat*)wrenGetSlotForeign(vm, 1);
	WrenQuat* b = (WrenQuat*)wrenGetSlotForeign(vm, 2);

	if (!b)
	{
		DEBUG_ERROR("Quat.dot: Invalid quaternion");
		wrenSetSlotDouble(vm, 0, 0.0);
		return;
	}

	wrenSetSlotDouble(vm, 0, glm::dot(a->value, b->value));
}

// Helper: Get forward/up/right vectors from quaternion

// quat.forward() -> Vec3
void wren_QuatForward(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);

	glm::vec3 forward = quat->value * glm::vec3(0, 0, -1);

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(forward);
}

// quat.up() -> Vec3
void wren_QuatUp(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);

	glm::vec3 up = quat->value * glm::vec3(0, 1, 0);

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(up);
}

// Quat.right() -> Vec3
void wren_QuatRight(WrenVM* vm)
{
	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);

	glm::vec3 right = quat->value * glm::vec3(1, 0, 0);

	wrenGetVariable(vm, "math", "Quat", 1);  // Get class into slot 1
	WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (result) WrenVec3(right);
}

// Register Quat foreign class
WREN_FOREIGN_CLASS("math", "Quat", wren_QuatAllocate, wren_QuatFinalize, "Quaternion class for 3D rotations wrapping glm::quat");

// Register constructors
WREN_CONSTRUCTOR("math", "Quat", "new()", wren_QuatNew, "Create identity quaternion");
WREN_CONSTRUCTOR("math", "Quat", "new(_,_,_,_)", wren_QuatNew, "Create quaternion with w, x, y, z components");

// Register instance methods
WREN_CLASS_METHOD("math", "Quat", "w", wren_QuatGetW, "Get W component");
WREN_CLASS_METHOD("math", "Quat", "x", wren_QuatGetX, "Get X component");
WREN_CLASS_METHOD("math", "Quat", "y", wren_QuatGetY, "Get Y component");
WREN_CLASS_METHOD("math", "Quat", "z", wren_QuatGetZ, "Get Z component");
WREN_CLASS_METHOD("math", "Quat", "w=(_)", wren_QuatSetW, "Set W component");
WREN_CLASS_METHOD("math", "Quat", "x=(_)", wren_QuatSetX, "Set X component");
WREN_CLASS_METHOD("math", "Quat", "y=(_)", wren_QuatSetY, "Set Y component");
WREN_CLASS_METHOD("math", "Quat", "z=(_)", wren_QuatSetZ, "Set Z component");
WREN_CLASS_METHOD("math", "Quat", "toString()", wren_QuatToString, "Convert to string");
WREN_CLASS_METHOD("math", "Quat", "normalize()", wren_QuatNormalize, "Get normalized quaternion");
WREN_CLASS_METHOD("math", "Quat", "inverse()", wren_QuatInverse, "Get inverse quaternion");
WREN_CLASS_METHOD("math", "Quat", "conjugate()", wren_QuatConjugate, "Get conjugate quaternion");
WREN_CLASS_METHOD("math", "Quat", "length()", wren_QuatLength, "Get quaternion length");
WREN_CLASS_METHOD("math", "Quat", "lengthSquared()", wren_QuatLengthSquared, "Get quaternion length squared");
WREN_CLASS_METHOD("math", "Quat", "toEuler()", wren_QuatToEuler, "Convert to Euler angles (radians)");
WREN_CLASS_METHOD("math", "Quat", "toEulerDegrees()", wren_QuatToEulerDegrees, "Convert to Euler angles (degrees)");
WREN_CLASS_METHOD("math", "Quat", "toAxisAngle()", wren_QuatToAxisAngle, "Convert to axis-angle representation");
WREN_CLASS_METHOD("math", "Quat", "*(_)", wren_QuatMultiply, "Multiply quaternions");
WREN_CLASS_METHOD("math", "Quat", "rotate(_)", wren_QuatRotateVec3, "Rotate vector by quaternion");
WREN_CLASS_METHOD("math", "Quat", "forward()", wren_QuatForward, "Get forward direction vector");
WREN_CLASS_METHOD("math", "Quat", "up()", wren_QuatUp, "Get up direction vector");
WREN_CLASS_METHOD("math", "Quat", "right()", wren_QuatRight, "Get right direction vector");

// Register static methods
WREN_CLASS_STATIC("math", "Quat", "dot(_,_)", wren_QuatDot, "Dot product");
WREN_CLASS_STATIC("math", "Quat", "identity()", wren_QuatIdentity, "Create identity quaternion");
WREN_CLASS_STATIC("math", "Quat", "fromAxisAngle(_,_)", wren_QuatFromAxisAngle, "Create quaternion from axis and angle");
WREN_CLASS_STATIC("math", "Quat", "fromEuler(_,_,_)", wren_QuatFromEuler, "Create quaternion from Euler angles (radians)");
WREN_CLASS_STATIC("math", "Quat", "fromEulerDegrees(_,_,_)", wren_QuatFromEulerDegrees, "Create quaternion from Euler angles (degrees)");
WREN_CLASS_STATIC("math", "Quat", "lookAt(_,_)", wren_QuatLookAt, "Create quaternion from forward and up vectors");
WREN_CLASS_STATIC("math", "Quat", "slerp(_,_,_)", wren_QuatSlerp, "Spherical linear interpolation");
WREN_CLASS_STATIC("math", "Quat", "lerp(_,_,_)", wren_QuatLerp, "Linear interpolation");

// ============================================================================
// Mat4 - Foreign class wrapping glm::mat4
// ============================================================================

void wren_Mat4Allocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenMat4));
}

// Finalize
void wren_Mat4Finalize(void* data)
{
	WrenMat4* mat = static_cast<WrenMat4*>(data);
	mat->~WrenMat4();
}

// Mat4.new(_)
void wren_Mat4New(WrenVM* vm)
{
	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 0));
	new (mat) WrenMat4();
}

void wren_Mat4Identity(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Mat4", 0);  // Get class into slot 1
	WrenMat4* mat = static_cast<WrenMat4*>(wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenMat4)));
	new (mat) WrenMat4(glm::mat4(1.0f));
}

void wren_Mat4Perspective(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Mat4", 0);  // Get class into slot 1
	WrenMat4* mat = static_cast<WrenMat4*>(wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenMat4)));
	new (mat) WrenMat4(glm::mat4(1.0f));

	float fov = static_cast<float>(wrenGetSlotDouble(vm, 1));
	float aspect = static_cast<float>(wrenGetSlotDouble(vm, 2));
	float near = static_cast<float>(wrenGetSlotDouble(vm, 3));
	float far = static_cast<float>(wrenGetSlotDouble(vm, 4));
	mat->value = glm::perspective(fov, aspect, near, far);
}

void wren_Mat4Ortho(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Mat4", 0);  // Get class into slot 1
	WrenMat4* mat = static_cast<WrenMat4*>(wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenMat4)));
	new (mat) WrenMat4(glm::mat4(1.0f));

	float left = static_cast<float>(wrenGetSlotDouble(vm, 1));
	float right = static_cast<float>(wrenGetSlotDouble(vm, 2));
	float bottom = static_cast<float>(wrenGetSlotDouble(vm, 3));
	float top = static_cast<float>(wrenGetSlotDouble(vm, 4));
	float near = static_cast<float>(wrenGetSlotDouble(vm, 5));
	float far = static_cast<float>(wrenGetSlotDouble(vm, 6));
	mat->value = glm::ortho(left, right, bottom, top, near, far);
}

void wren_Mat4LookAt(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Mat4", 0);  // Get class into slot 1
	WrenMat4* mat = static_cast<WrenMat4*>(wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenMat4)));
	new (mat) WrenMat4(glm::mat4(1.0f));

	WrenVec3* eye = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));
	WrenVec3* center = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));
	WrenVec3* up = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));
	mat->value = glm::lookAt(eye->value, center->value, up->value);
}

// Operations
void wren_Mat4Multiply(WrenVM* vm)
{
	WrenMat4* a = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 0));
	WrenMat4* b = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 1));

	wrenGetVariable(vm, "math", "Mat4", 1);  // Get class into slot 1
	WrenMat4* result = static_cast<WrenMat4*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenMat4)));
	new (result) WrenMat4(a->value * b->value);
}

// Transformations (modify in place)
void wren_Mat4Translate(WrenVM* vm)
{
	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 0));
	WrenVec3* vec3 = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));
	mat->value = glm::translate(mat->value, vec3->value);
}

void wren_Mat4Rotate(WrenVM* vm)
{
	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 0));
	float angle = static_cast<float>(wrenGetSlotDouble(vm, 1));
	WrenVec3* vec3 = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));
	mat->value = glm::rotate(mat->value, angle, vec3->value);
}

void wren_Mat4Scale(WrenVM* vm)
{
	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 0));
	WrenVec3* vec3 = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 1));
	mat->value = glm::scale(mat->value, vec3->value);
}

// Access
void wren_Mat4Get(WrenVM* vm)
{
	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 0));
	int row = static_cast<int>(wrenGetSlotDouble(vm, 1));
	int col = static_cast<int>(wrenGetSlotDouble(vm, 2));

	if (row < 0 || row >= 4 || col < 0 || col >= 4) {
		wrenSetSlotString(vm, 0, "Index out of bounds");
		wrenAbortFiber(vm, 0);
		return;
	}

	wrenSetSlotDouble(vm, 0, mat->value[col][row]);
}

void wren_Mat4Set(WrenVM* vm)
{
	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 0));
	int row = static_cast<int>(wrenGetSlotDouble(vm, 1));
	int col = static_cast<int>(wrenGetSlotDouble(vm, 2));
	float value = static_cast<float>(wrenGetSlotDouble(vm, 3));

	if (row < 0 || row >= 4 || col < 0 || col >= 4) {
		wrenSetSlotString(vm, 0, "Index out of bounds");
		wrenAbortFiber(vm, 0);
		return;
	}

	mat->value[col][row] = value;
}

// Properties
void wren_Mat4Determinant(WrenVM* vm)
{
	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotDouble(vm, 0, glm::determinant(mat->value));
}

void wren_Mat4Inverse(WrenVM* vm)
{
	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 0));
	mat->value = glm::inverse(mat->value);
}

void wren_Mat4Transpose(WrenVM* vm)
{
	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 0));
	mat->value = glm::transpose(mat->value);
}

// Conversion
void wren_Mat4ToList(WrenVM* vm)
{
	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 0));
	const float* data = glm::value_ptr(mat->value);

	wrenSetSlotNewList(vm, 0);
	for (int i = 0; i < 16; i++) {
		wrenSetSlotDouble(vm, 1, data[i]);
		wrenInsertInList(vm, 0, -1, 1);
	}
}

void wren_Mat4ToString(WrenVM* vm)
{
	WrenMat4* mat = static_cast<WrenMat4*>(wrenGetSlotForeign(vm, 0));

	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2);
	oss << "Mat4[\n";
	for (int row = 0; row < 4; row++) {
		oss << "  [";
		for (int col = 0; col < 4; col++) {
			oss << std::setw(8) << mat->value[col][row];
			if (col < 3) oss << ", ";
		}
		oss << "]\n";
	}
	oss << "]";

	wrenSetSlotString(vm, 0, oss.str().c_str());
}

// Register Mat4 foreign class
WREN_FOREIGN_CLASS("math", "Mat4", wren_Mat4Allocate, wren_Mat4Finalize, "Matix 4 class wrapping glm::mat4");

// Register constructors
WREN_CONSTRUCTOR("math", "Mat4", "new()", wren_Mat4New, "Create identity Matix 4");

// Register instance methods
WREN_CLASS_METHOD("math", "Mat4", "*(_)", wren_Mat4Multiply, "Multiply matix's");
WREN_CLASS_METHOD("math", "Mat4", "translate(_)", wren_Mat4Translate, "Translate matix");
WREN_CLASS_METHOD("math", "Mat4", "rotate(_,_)", wren_Mat4Rotate, "Rotate matix");
WREN_CLASS_METHOD("math", "Mat4", "scale(_,_)", wren_Mat4Scale, "Scale matix");
WREN_CLASS_METHOD("math", "Mat4", "get(_,_)", wren_Mat4Get, "Get matix element");
WREN_CLASS_METHOD("math", "Mat4", "set(_,_)", wren_Mat4Set, "Set matix element");
WREN_CLASS_METHOD("math", "Mat4", "determinant()", wren_Mat4Determinant, "Get matrix Determinant");
WREN_CLASS_METHOD("math", "Mat4", "inverse()", wren_Mat4Inverse, "Get matrix Inverse");
WREN_CLASS_METHOD("math", "Mat4", "transpose()", wren_Mat4Transpose, "Get matrix transpose");
WREN_CLASS_METHOD("math", "Mat4", "toList()", wren_Mat4ToList, "Get matrix as list");
WREN_CLASS_METHOD("math", "Mat4", "toString()", wren_Mat4ToString, "Convert to string");

// Register static methods
WREN_CLASS_STATIC("math", "Mat4", "identity()", wren_Mat4Identity, "Create identity Matix 4");
WREN_CLASS_STATIC("math", "Mat4", "perspective(_,_,_,_)", wren_Mat4Perspective, "Dot product");
WREN_CLASS_STATIC("math", "Mat4", "ortho(_,_,_,_,_,_)", wren_Mat4Ortho, "Create identity quaternion");
WREN_CLASS_STATIC("math", "Mat4", "lookAt(_,_,_)", wren_Mat4LookAt, "Create quaternion from axis and angle");

// ============================================================================
// Rect - Foreign class wrapping raylib ::Rect
// ============================================================================

void wren_RectAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "math", "Rect", 0);  // Get class into slot 1
	WrenRect* rect = static_cast<WrenRect*>(wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenRect)));
	new (rect) WrenRect({ 0.0f, 0.0f, 0.0f, 0.0f });
}

// Finalize
void wren_RectFinalize(void* data)
{
	WrenRect* rect = static_cast<WrenRect*>(data);
	rect->~WrenRect();
}

// Rect.new(_)
void wren_RectNew(WrenVM* vm)
{
	WrenRect* rect = static_cast<WrenRect*>(wrenGetSlotForeign(vm, 0));

	// Initialise with constructor parameters if provided
	if (wrenGetSlotCount(vm) >= 5)
	{
		// Quat.new(w, x, y, z)
		float x = (float)wrenGetSlotDouble(vm, 1);
		float y = (float)wrenGetSlotDouble(vm, 2);
		float width = (float)wrenGetSlotDouble(vm, 3);
		float height = (float)wrenGetSlotDouble(vm, 4);
		new (rect) WrenRect({ x, y, width, height });
	}
	else {
		new(rect) WrenRect({ 0.0f, 0.0f, 0.0f, 0.0f });
	}
}

// Component getters
void wren_RectGetX(WrenVM* vm)
{
	WrenRect* rect = (WrenRect*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, rect->value.x);
}

void wren_RectGetY(WrenVM* vm)
{
	WrenRect* rect = (WrenRect*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, rect->value.y);
}

void wren_RectGetWidth(WrenVM* vm)
{
	WrenRect* rect = (WrenRect*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, rect->value.width);
}

void wren_RectGetHeight(WrenVM* vm)
{
	WrenRect* rect = (WrenRect*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, rect->value.height);
}

// Component setters
void wren_RectSetX(WrenVM* vm)
{
	WrenRect* rect = (WrenRect*)wrenGetSlotForeign(vm, 0);
	rect->value.x = (float)wrenGetSlotDouble(vm, 1);
}

void wren_RectSetY(WrenVM* vm)
{
	WrenRect* rect = (WrenRect*)wrenGetSlotForeign(vm, 0);
	rect->value.y = (float)wrenGetSlotDouble(vm, 1);
}

void wren_RectSetWidth(WrenVM* vm)
{
	WrenRect* rect = (WrenRect*)wrenGetSlotForeign(vm, 0);
	rect->value.width = (float)wrenGetSlotDouble(vm, 1);
}

void wren_RectSetHeight(WrenVM* vm)
{
	WrenRect* rect = (WrenRect*)wrenGetSlotForeign(vm, 0);
	rect->value.height = (float)wrenGetSlotDouble(vm, 1);
}

// Register Mat4 foreign class
WREN_FOREIGN_CLASS("math", "Rect", wren_RectAllocate, wren_RectFinalize, "Rect class wrapping raylib ::Rect");

// Register constructors
WREN_CONSTRUCTOR("math", "Rect", "new(_,_,_,_)", wren_RectNew, "Create Rect, with components x, y, width, and height", x, y, width, height);
WREN_CONSTRUCTOR("math", "Rect", "new()", wren_RectNew, "Create Rect");

// Register instance methods
WREN_CLASS_METHOD("math", "Rect", "x", wren_RectGetX, "Get X component");
WREN_CLASS_METHOD("math", "Rect", "y", wren_RectGetY, "Get Y component");
WREN_CLASS_METHOD("math", "Rect", "width", wren_RectGetWidth, "Get Width component");
WREN_CLASS_METHOD("math", "Rect", "height", wren_RectGetHeight, "Get height component");
WREN_CLASS_METHOD("math", "Rect", "x=(_)", wren_RectSetX, "Set X component");
WREN_CLASS_METHOD("math", "Rect", "y=(_)", wren_RectSetY, "Set Y component");
WREN_CLASS_METHOD("math", "Rect", "width=(_)", wren_RectSetWidth, "Set Width component");
WREN_CLASS_METHOD("math", "Rect", "height=(_)", wren_RectSetHeight, "Set Height component");
