const float pi = 3.1415926535897932385;

// Random number generation using pcg32i_random_t, using inc = 1. Our random state is a uint.
uint stepRNG(uint rngState)
{
  return rngState * 747796405 + 1;
}

// Steps the RNG and returns a floating-point value between 0 and 1 inclusive.
float stepAndOutputRNGFloat(inout uint rngState)
{
  // Condensed version of pcg_output_rxs_m_xs_32_32, with simple conversion to floating-point [0,1].
  rngState  = stepRNG(rngState);
  uint word = ((rngState >> ((rngState >> 28) + 4)) ^ rngState) * 277803737;
  word      = (word >> 22) ^ word;
  return float(word) / 4294967295.0f;
}

uint rngState =  (600 * gl_GlobalInvocationID.x + gl_GlobalInvocationID.y) * (ubo.currentSample+1);
float random() {
    return stepAndOutputRNGFloat(rngState);
}

float random(float min, float max) {
    // Returns a random real in [min,max).
    return min + (max-min)*random();
}

vec3 random_in_unit_sphere() {
    vec3 p = vec3(random(-0.3,0.3),random(-0.3,0.3),random(-0.3,0.3));
    return normalize(p);
}

vec3 random_in_hemisphere(vec3 normal) {
    vec3 in_unit_sphere = random_in_unit_sphere();
    if (dot(in_unit_sphere, normal) > 0.0) // In the same hemisphere as the normal
        return in_unit_sphere;
    else
        return -in_unit_sphere;
}

vec3 random_cosine_direction() {
    float r1 = random();
    float r2 = random();
    float z = sqrt(1-r2);

    float phi = 2*pi*r1;
    float x = cos(phi)*sqrt(r2);
    float y = sin(phi)*sqrt(r2);

    return vec3(x, y, z);
}
