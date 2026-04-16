// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: math

// Rect class wrapping raylib ::Rect
foreign class Rect {
    // Create Rect, with components x, y, width, and height
    foreign construct new(arg0,arg1,arg2,arg3)
    // Create Rect
    foreign construct new()
    // Copy a Rect
    foreign construct copy(arg0)
    // Get X component
    foreign x
    // Get Y component
    foreign y
    // Get Width component
    foreign width
    // Get height component
    foreign height
    // Set X component
    foreign x=(arg0)
    // Set Y component
    foreign y=(arg0)
    // Set Width component
    foreign width=(arg0)
    // Set Height component
    foreign height=(arg0)
}

class Math {
    // Positive infinity
    foreign static infinity
    // Negative infinity
    foreign static negInfinity
    // Not a Number
    foreign static nan
    // Pi constant
    foreign static pi
    // Euler's number
    foreign static e
    // Maximum of two numbers
    foreign static max(arg0,arg1)
    // Minimum of two numbers
    foreign static min(arg0,arg1)
    // Clamp value between min and max
    foreign static clamp(arg0,arg1,arg2)
    // Absolute value
    foreign static abs(arg0)
    // Square root
    foreign static sqrt(arg0)
    // Power
    foreign static pow(arg0,arg1)
    // Linear interpolation
    foreign static lerp(arg0,arg1,arg2)
    // Sign of number (-1, 0, 1)
    foreign static sign(arg0)
    // Sine
    foreign static sin(arg0)
    // Cosine
    foreign static cos(arg0)
    // Tangent
    foreign static tan(arg0)
    // Arc sine
    foreign static asin(arg0)
    // Arc cosine
    foreign static acos(arg0)
    // Arc tangent
    foreign static atan(arg0)
    // Arc tangent of y/x
    foreign static atan2(arg0,arg1)
    // Floor
    foreign static floor(arg0)
    // Ceiling
    foreign static ceil(arg0)
    // Round to nearest integer
    foreign static round(arg0)
    // Check if infinite
    foreign static isInfinite(arg0)
    // Check if NaN
    foreign static isNaN(arg0)
    // Check if finite
    foreign static isFinite(arg0)
    // Convert degrees to radians
    foreign static radians(arg0)
    // Convert radians to degrees
    foreign static degrees(arg0)
}

// 2D vector class wrapping glm::vec2
foreign class Vec2 {
    // Create zero vector
    foreign construct new()
    // Create vector with x, y components
    foreign construct new(arg0,arg1)
    // Copy a vector
    foreign construct copy(arg0)
    // Get X component
    foreign x
    // Get Y component
    foreign y
    // Set X component
    foreign x=(arg0)
    // Set Y component
    foreign y=(arg0)
    // Convert to string
    foreign toString
    // Add two vectors
    foreign +(arg0)
    // Subtract vectors
    foreign -(arg0)
    // Multiply by scalar
    foreign *(arg0)
    // Get vector length
    foreign length()
    // Get vector length squared
    foreign lengthSquared()
    // Get normalized vector
    foreign normalize()
    // Distance between two vectors
    foreign static distance(arg0,arg1)
    // Squared distance
    foreign static distanceSquared(arg0,arg1)
    // Linear interpolation
    foreign static lerp(arg0,arg1,arg2)
    // Dot product (static)
    foreign static dot(arg0,arg1)
    // Reflect vector
    foreign static reflect(arg0,arg1)
    // Component-wise minimum
    foreign static min(arg0,arg1)
    // Component-wise maximum
    foreign static max(arg0,arg1)
    // Clamp vector
    foreign static clamp(arg0,arg1,arg2)
    // Zero vector (0, 0)
    foreign static zero()
    // One vector (1, 1)
    foreign static one()
    // Right vector (1, 0)
    foreign static right()
    // Up vector (0, 1)
    foreign static up()
    // Left vector (-1, 0)
    foreign static left()
    // Down vector (0, -1)
    foreign static down()
}

// 3D vector class wrapping glm::vec3
foreign class Vec3 {
    // Create zero vector
    foreign construct new()
    // Create vector with x, y, z components
    foreign construct new(arg0,arg1,arg2)
    // Copy a vector
    foreign construct copy(arg0)
    // Get X component
    foreign x
    // Get Y component
    foreign y
    // Get Z component
    foreign z
    // Set X component
    foreign x=(arg0)
    // Set Y component
    foreign y=(arg0)
    // Set Z component
    foreign z=(arg0)
    // Convert to string
    foreign toString()
    // Add two vectors
    foreign +(arg0)
    // Subtract vectors
    foreign -(arg0)
    // Multiply by scalar
    foreign *(arg0)
    // Get vector length
    foreign length()
    // Get vector length squared
    foreign lengthSquared()
    // Get normalized vector
    foreign normalize
    // Distance between two vectors
    foreign static distance(arg0,arg1)
    // Squared distance
    foreign static distanceSquared(arg0,arg1)
    // Linear interpolation
    foreign static lerp(arg0,arg1,arg2)
    // Dot product (static)
    foreign static dot(arg0,arg1)
    // Cross product (static)
    foreign static cross(arg0,arg1)
    // Reflect vector
    foreign static reflect(arg0,arg1)
    // Refract vector
    foreign static refract(arg0,arg1,arg2)
    // Component-wise minimum
    foreign static min(arg0,arg1)
    // Component-wise maximum
    foreign static max(arg0,arg1)
    // Clamp vector
    foreign static clamp(arg0,arg1,arg2)
    // Zero vector (0, 0, 0)
    foreign static zero()
    // One vector (1, 1, 1)
    foreign static one()
    // Right vector (1, 0, 0)
    foreign static right()
    // Up vector (0, 1, 0)
    foreign static up()
    // Forward vector (0, 0, -1)
    foreign static forward()
    // Left vector (-1, 0, 0)
    foreign static left()
    // Down vector (0, -1, 0)
    foreign static down()
    // Back vector (0, 0, 1)
    foreign static back()
}

// Matix 4 class wrapping glm::mat4
foreign class Mat4 {
    // Create identity Matix 4
    foreign construct new()
    // Copy a Matix 4
    foreign construct copy(arg0)
    // Multiply matix's
    foreign *(arg0)
    // Translate matix
    foreign translate(arg0)
    // Rotate matix
    foreign rotate(arg0,arg1)
    // Scale matix
    foreign scale(arg0,arg1)
    // Get matix element
    foreign get(arg0,arg1)
    // Set matix element
    foreign set(arg0,arg1)
    // Get matrix Determinant
    foreign determinant()
    // Get matrix Inverse
    foreign inverse()
    // Get matrix transpose
    foreign transpose()
    // Get matrix as list
    foreign toList()
    // Convert to string
    foreign toString()
    // Create identity Matix 4
    foreign static identity()
    // Dot product
    foreign static perspective(arg0,arg1,arg2,arg3)
    // Create identity quaternion
    foreign static ortho(arg0,arg1,arg2,arg3,arg4,arg5)
    // Create quaternion from axis and angle
    foreign static lookAt(arg0,arg1,arg2)
}

// 4D vector class wrapping glm::vec4
foreign class Vec4 {
    // Create zero vector
    foreign construct new()
    // Create vector with x, y, z, w components
    foreign construct new(arg0,arg1,arg2,arg3)
    // Copy a vector
    foreign construct copy(arg0)
    // Get X component
    foreign x
    // Get Y component
    foreign y
    // Get Z component
    foreign z
    // Get W component
    foreign w
    // Set X component
    foreign x=(arg0)
    // Set Y component
    foreign y=(arg0)
    // Set Z component
    foreign z=(arg0)
    // Set W component
    foreign w=(arg0)
    // Convert to string
    foreign toString()
    // Add two vectors
    foreign +(arg0)
    // Subtract vectors
    foreign -(arg0)
    // Multiply by scalar
    foreign *(arg0)
    // Get vector length
    foreign length()
    // Get vector length squared
    foreign lengthSquared()
    // Get normalized vector
    foreign normalize()
    // Distance between two vectors
    foreign static distance(arg0,arg1)
    // Squared distance
    foreign static distanceSquared(arg0,arg1)
    // Linear interpolation
    foreign static lerp(arg0,arg1,arg2)
    // Dot product (static)
    foreign static dot(arg0,arg1)
    // Component-wise minimum
    foreign static min(arg0,arg1)
    // Component-wise maximum
    foreign static max(arg0,arg1)
    // Clamp vector
    foreign static clamp(arg0,arg1,arg2)
    // Zero vector (0, 0, 0, 0)
    foreign static zero()
    // One vector (1, 1, 1, 1)
    foreign static one()
}

// Quaternion class for 3D rotations wrapping glm::quat
foreign class Quat {
    // Create identity quaternion
    foreign construct new()
    // Create quaternion with w, x, y, z components
    foreign construct new(arg0,arg1,arg2,arg3)
    // Copy a quaternion
    foreign construct copy(arg0)
    // Get W component
    foreign w
    // Get X component
    foreign x
    // Get Y component
    foreign y
    // Get Z component
    foreign z
    // Set W component
    foreign w=(arg0)
    // Set X component
    foreign x=(arg0)
    // Set Y component
    foreign y=(arg0)
    // Set Z component
    foreign z=(arg0)
    // Convert to string
    foreign toString()
    // Get normalized quaternion
    foreign normalize()
    // Get inverse quaternion
    foreign inverse()
    // Get conjugate quaternion
    foreign conjugate()
    // Get quaternion length
    foreign length()
    // Get quaternion length squared
    foreign lengthSquared()
    // Convert to Euler angles (radians)
    foreign toEuler()
    // Convert to Euler angles (degrees)
    foreign toEulerDegrees()
    // Convert to axis-angle representation
    foreign toAxisAngle()
    // Multiply quaternions
    foreign *(arg0)
    // Rotate vector by quaternion
    foreign rotate(arg0)
    // Get forward direction vector
    foreign forward()
    // Get up direction vector
    foreign up()
    // Get right direction vector
    foreign right()
    // Dot product
    foreign static dot(arg0,arg1)
    // Create identity quaternion
    foreign static identity()
    // Create quaternion from axis and angle
    foreign static fromAxisAngle(arg0,arg1)
    // Create quaternion from Euler angles (radians)
    foreign static fromEuler(arg0,arg1,arg2)
    // Create quaternion from Euler angles (degrees)
    foreign static fromEulerDegrees(arg0,arg1,arg2)
    // Create quaternion from forward and up vectors
    foreign static lookAt(arg0,arg1)
    // Spherical linear interpolation
    foreign static slerp(arg0,arg1,arg2)
    // Linear interpolation
    foreign static lerp(arg0,arg1,arg2)
}

