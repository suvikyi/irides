#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
  mat4 qt_Matrix;
  float qt_Opacity;
  vec2 itemSize;        // the surface grown by 2 * outsetSize
  vec2 surfaceSize;     // the surface's own box
  float radius;         // the surface's corner radius
  float roundingPower;
  float insetSize;      // inset band width
  float outsetSize;     // outset band width
  vec4 insetPaint;      // inset band colour (painted path)
  vec4 outsetPaint;     // outset band colour
  float flatPaint;      // 1 = paint the inset band, 0 = transform the source
  float gradeOpacity;
  float lightness;
  float saturation;
  float chroma;
  float hue;
  float colorizeStrength;
  int colorizeStopCount;
  float colorizeAngle;
  vec4 colorizeStop0;
  vec4 colorizeStop1;
  vec4 colorizeStop2;
  vec4 colorizeStop3;
  vec4 colorizeStop4;
  vec4 colorizeStop5;
  vec4 colorizeStop6;
  vec4 colorizeStop7;
  vec4 colorizeStop8;
  vec4 colorizeStop9;
};
layout(binding = 1) uniform sampler2D source;

// Hyprion's border coverage, sharing one smoothing ramp across the straight
// edges and the rounded corners. The two bands tile the surface's edge, so
// their premultiplied contributions add.
const float smoothing = 3.14159265358979323846 / 5.34665792551;
#define SRGB_POW 2.4
#define SRGB_CUT 0.0031308
#define SRGB_SCALE 12.92
#define SRGB_ALPHA 1.055

vec3 toLinear(vec3 color) {
  bvec3 low = lessThanEqual(color, vec3(SRGB_CUT * SRGB_SCALE));
  vec3 lo = color / SRGB_SCALE;
  vec3 hi = pow(max((color + SRGB_ALPHA - 1.0) / SRGB_ALPHA, vec3(0.0)), vec3(SRGB_POW));
  return mix(hi, lo, low);
}
vec3 toSrgb(vec3 color) {
  bvec3 low = lessThanEqual(color, vec3(SRGB_CUT));
  vec3 lo = color * SRGB_SCALE;
  vec3 hi = pow(max(color, vec3(0.0)), vec3(1.0 / SRGB_POW)) * SRGB_ALPHA - (SRGB_ALPHA - 1.0);
  return mix(hi, lo, low);
}

vec3 linearToOklab(vec3 rgb) {
  float l = pow(max(dot(vec3(0.4122214708, 0.5363325363, 0.0514459929), rgb), 0.0), 1.0 / 3.0);
  float m = pow(max(dot(vec3(0.2119034982, 0.6806995451, 0.1073969566), rgb), 0.0), 1.0 / 3.0);
  float s = pow(max(dot(vec3(0.0883024619, 0.2817188376, 0.6299787005), rgb), 0.0), 1.0 / 3.0);
  return vec3(0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
              1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
              0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s);
}
vec3 oklabToLinear(vec3 lab) {
  float l_ = lab.x + 0.3963377774 * lab.y + 0.2158037573 * lab.z;
  float m_ = lab.x - 0.1055613458 * lab.y - 0.0638541728 * lab.z;
  float s_ = lab.x - 0.0894841775 * lab.y - 1.2914855480 * lab.z;
  float l = l_ * l_ * l_;
  float m = m_ * m_ * m_;
  float s = s_ * s_ * s_;
  return vec3(4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
             -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
             -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s);
}
bool inGamut(vec3 rgb) {
  return all(greaterThanEqual(rgb, vec3(0.0))) && all(lessThanEqual(rgb, vec3(1.0)));
}
vec3 compressChroma(float lightness, vec2 chroma) {
  vec3 candidate = oklabToLinear(vec3(lightness, chroma));
  if (inGamut(candidate))
    return candidate;
  float low = 0.0;
  float high = 1.0;
  for (int i = 0; i < 10; ++i) {
    float middle = (low + high) * 0.5;
    if (inGamut(oklabToLinear(vec3(lightness, chroma * middle))))
      low = middle;
    else
      high = middle;
  }
  return clamp(oklabToLinear(vec3(lightness, chroma * low)), vec3(0.0), vec3(1.0));
}

float distanceWithRounding(vec2 coords, float power) {
  return pow(pow(abs(coords.x), power) + pow(abs(coords.y), power), 1.0 / power);
}

float borderCoverage(vec2 point, vec2 halfSize, float radius, float outerRadius,
    float thickness, float power) {
  vec2 correction = vec2(1.0) / (halfSize * 2.0);
  vec2 inner = abs(point) - halfSize + vec2(radius) + correction;
  vec2 outer = abs(point) - halfSize + vec2(outerRadius) + correction;
  if (min(inner.x, inner.y) > 0.0 && radius > 0.0) {
    float dist = distanceWithRounding(inner, power);
    float distOuter = distanceWithRounding(max(outer, vec2(0.0)), power);
    float h = thickness * 0.5;
    if (dist < radius - h)
      return smoothstep(0.0, 1.0, (dist - radius + thickness + smoothing) / (smoothing * 2.0));
    if (min(outer.x, outer.y) > 0.0)
      return 1.0 - smoothstep(0.0, 1.0, (distOuter - outerRadius + smoothing) / (smoothing * 2.0));
    if (distOuter < outerRadius - h)
      return 1.0;
  }
  float edge = min((halfSize - abs(point)).x, (halfSize - abs(point)).y);
  return smoothstep(0.0, 1.0, (edge + smoothing) / (smoothing * 2.0))
    * (1.0 - smoothstep(0.0, 1.0, (edge - thickness + smoothing) / (smoothing * 2.0)));
}

vec4 gradientStop(int index) {
  if (index <= 0) return colorizeStop0;
  if (index == 1) return colorizeStop1;
  if (index == 2) return colorizeStop2;
  if (index == 3) return colorizeStop3;
  if (index == 4) return colorizeStop4;
  if (index == 5) return colorizeStop5;
  if (index == 6) return colorizeStop6;
  if (index == 7) return colorizeStop7;
  if (index == 8) return colorizeStop8;
  return colorizeStop9;
}

vec4 sampleColorizeGradient(vec2 coord) {
  if (colorizeStopCount <= 0)
    return vec4(0.0);
  if (colorizeStopCount == 1)
    return colorizeStop0;

  vec2 normalized = clamp(coord, vec2(0.0), vec2(1.0));
  float angle = radians(colorizeAngle);
  if (angle > 4.71) {
    normalized.y = 1.0 - normalized.y;
    angle = 6.28 - angle;
  } else if (angle > 3.14) {
    normalized = vec2(1.0) - normalized;
    angle = angle - 3.14;
  } else if (angle > 1.57) {
    normalized.x = 1.0 - normalized.x;
    angle = 3.14 - angle;
  }

  float progress = (normalized.y * sin(angle) + normalized.x * (1.0 - sin(angle))) * float(colorizeStopCount - 1);
  int lower = int(floor(progress));
  int upper = lower + 1;
  return mix(gradientStop(lower), gradientStop(upper), progress - float(lower));
}

vec2 colorizeChroma(vec4 color) {
  // Colour uniforms arrive premultiplied; the chroma wanted here is straight.
  vec3 straight = color.a > 0.001 ? clamp(color.rgb / color.a, vec3(0.0), vec3(1.0))
    : vec3(0.0);
  return linearToOklab(clamp(toLinear(straight), vec3(0.0), vec3(1.0))).yz;
}

// Hyprion's transformSample, trimmed to SDR sRGB. Premultiplied in,
// premultiplied out.
vec4 transformSample(vec4 sampleColor, vec2 colorizeTarget) {
  if (sampleColor.a <= 0.001)
    return sampleColor;
  vec3 straight = clamp(sampleColor.rgb / sampleColor.a, vec3(0.0), vec3(1.0));
  vec3 lab = linearToOklab(clamp(toLinear(straight), vec3(0.0), vec3(1.0)));
  lab.x = clamp(lab.x + lightness, 0.0, 1.0);

  // Rotate hue in the (a, b) plane, scale the chroma magnitude, then offset
  // it. The offset only touches chroma the sample already has, so neutrals
  // stay neutral; the smoothstep keeps that gate continuous near zero.
  float magnitude = length(lab.yz);
  vec2 direction = magnitude > 0.000001 ? lab.yz / magnitude : vec2(0.0);
  float angle = radians(hue);
  float cosine = cos(angle);
  float sine = sin(angle);
  vec2 rotated = vec2(cosine * direction.x - sine * direction.y,
                      sine * direction.x + cosine * direction.y);
  float offset = chroma * smoothstep(0.0, 0.02, magnitude);
  lab.yz = rotated * max(magnitude * saturation + offset, 0.0);

  if (colorizeStrength > 0.0)
    lab.yz = mix(lab.yz, colorizeTarget, colorizeStrength);

  vec3 adjusted = toSrgb(clamp(compressChroma(lab.x, lab.yz), vec3(0.0), vec3(1.0)));
  return vec4(adjusted * sampleColor.a, sampleColor.a);
}

// The fallback paints a flat band, but it must still respond to the same
// grade style the compositor would apply.
vec4 gradePaint(vec4 paint) {
  if (paint.a <= 0.001)
    return paint;
  if (lightness == 0.0 && saturation == 1.0 && chroma == 0.0 && hue == 0.0 && colorizeStrength <= 0.0)
    return paint;
  return transformSample(paint, colorizeChroma(sampleColorizeGradient(qt_TexCoord0)));
}

void main() {
  if (itemSize.x <= 0.0 || itemSize.y <= 0.0)
    discard;

  vec2 boxHalf = itemSize * 0.5;
  vec2 surfaceHalf = surfaceSize * 0.5;
  vec2 point = qt_TexCoord0 * itemSize - boxHalf;
  float power = max(roundingPower, 1.0);

  float coverageInset = insetSize > 0.0
    ? borderCoverage(point, surfaceHalf, radius, radius, insetSize, power)
    : 0.0;
  float outsetCorrection = outsetSize * (sqrt(2.0) - 1.0) * max(2.0 - power, 0.0);
  float coverageOutset = outsetSize > 0.0
    ? borderCoverage(point, boxHalf, radius + outsetSize,
        radius + outsetSize - outsetCorrection, outsetSize, power)
    : 0.0;
  if (coverageInset <= 0.0 && coverageOutset <= 0.0)
    discard;

  vec4 sampleColor = vec4(0.0);
  if (flatPaint <= 0.5) {
    vec2 uvSurface = (qt_TexCoord0 * itemSize - outsetSize) / surfaceSize;
    sampleColor = texture(source, clamp(uvSurface, vec2(0.0), vec2(1.0)));
  }

  vec4 result = vec4(0.0);

  if (coverageOutset > 0.0)
    result += gradePaint(outsetPaint) * (coverageOutset * gradeOpacity);

  if (coverageInset > 0.0) {
    if (flatPaint > 0.5) {
      result += gradePaint(insetPaint) * (coverageInset * gradeOpacity);
    } else {
      float amount = clamp(gradeOpacity * coverageInset, 0.0, 1.0);
      vec4 band = sampleColor;
      if (amount > 0.0 && (lightness != 0.0 || saturation != 1.0 || chroma != 0.0 || hue != 0.0 || colorizeStrength > 0.0))
        band = transformSample(sampleColor, colorizeChroma(sampleColorizeGradient(qt_TexCoord0)));
      result += band * coverageInset;
    }
  }

  if (result.a <= 0.0)
    discard;
  fragColor = result * qt_Opacity;
}
