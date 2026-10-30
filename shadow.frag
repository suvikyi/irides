#version 440
// Outside-only port of Hyprland 0.56's rounded-shadow geometry. The surface
// box and radius are cut out of both the soft and the sharp path.
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
  mat4 qt_Matrix;
  float qt_Opacity;
  vec2 itemSize;
  vec2 winMin;
  vec2 winMax;
  vec2 shMin;
  vec2 shMax;
  vec4 shadowPaint;
  float radius;
  float windowRadius;
  float range;
  float shadowPower;
  float roundingPower;
  float sharp;
  float shadowOpacity;
};

const float smoothingConstant = 3.14159265358979323846 / 5.34665792551;

float distanceWithRounding(vec2 coords, float power) {
  return pow(pow(abs(coords.x), power) + pow(abs(coords.y), power), 1.0 / power);
}

float roundedCoverage(vec2 pos, vec2 tl, vec2 br, float round, float power) {
  vec2 size = br - tl;
  if (size.x <= 0.0 || size.y <= 0.0) return 0.0;
  if (round <= 0.0) return 1.0;

  vec2 pixCoord = abs(pos - tl - size * 0.5);
  pixCoord -= size * 0.5 - round;
  pixCoord += vec2(1.0) / size;
  if (pixCoord.x + pixCoord.y <= round) return 1.0;

  float dist = distanceWithRounding(pixCoord, power);
  if (dist > round + smoothingConstant) return 0.0;
  return 1.0 - smoothstep(0.0, 1.0,
    (dist - round + smoothingConstant) / (smoothingConstant * 2.0));
}

bool pointInRoundedRect(vec2 pos, vec2 tl, vec2 br, float round, float power) {
  if (pos.x < tl.x || pos.x > br.x || pos.y < tl.y || pos.y > br.y)
    return false;
  if (round <= 0.0) return true;

  round = min(round, min((br.x - tl.x) * 0.5, (br.y - tl.y) * 0.5));
  vec2 innerTL = tl + vec2(round);
  vec2 innerBR = br - vec2(round);
  if (pos.x >= innerTL.x && pos.x <= innerBR.x) return true;
  if (pos.y >= innerTL.y && pos.y <= innerBR.y) return true;

  vec2 delta = vec2(
    pos.x < innerTL.x ? innerTL.x - pos.x : pos.x - innerBR.x,
    pos.y < innerTL.y ? innerTL.y - pos.y : pos.y - innerBR.y
  );
  return distanceWithRounding(delta, power) <= round;
}

float softShadowCoverage(vec2 pos) {
  vec2 fpos = pos - shMin;
  float fw = shMax.x - shMin.x;
  float fh = shMax.y - shMin.y;
  float cornerRadius = range + radius;
  vec2 tl = vec2(cornerRadius);
  vec2 br = vec2(fw - cornerRadius, fh - cornerRadius);
  float fall = 1.0;
  bool inCorner = false;
  float d = 0.0;

  if (fpos.x < tl.x && fpos.y < tl.y) {
    d = distanceWithRounding(fpos - tl, roundingPower);
    inCorner = true;
  } else if (fpos.x < tl.x && fpos.y > br.y) {
    d = distanceWithRounding(fpos - vec2(tl.x, br.y), roundingPower);
    inCorner = true;
  } else if (fpos.x > br.x && fpos.y < tl.y) {
    d = distanceWithRounding(fpos - vec2(br.x, tl.y), roundingPower);
    inCorner = true;
  } else if (fpos.x > br.x && fpos.y > br.y) {
    d = distanceWithRounding(fpos - br, roundingPower);
    inCorner = true;
  }

  if (inCorner) {
    if (d > cornerRadius) fall = 0.0;
    else if (range > 0.0 && d > cornerRadius - range)
      fall = pow((cornerRadius - d) / range, shadowPower);
  } else if (range > 0.0) {
    float smallest = min(min(fpos.y, fh - fpos.y), min(fpos.x, fw - fpos.x));
    if (smallest < range) fall = pow(smallest / range, shadowPower);
  }
  return fall;
}

void main() {
  vec2 pos = qt_TexCoord0 * itemSize;
  if (pos.x < shMin.x || pos.x > shMax.x || pos.y < shMin.y || pos.y > shMax.y)
    discard;

  float power = max(roundingPower, 1.0);
  float outA = sharp > 0.5
    ? roundedCoverage(pos, shMin, shMax, radius, power)
    : softShadowCoverage(pos);

  if (pointInRoundedRect(pos, winMin, winMax, windowRadius, power))
    outA = 0.0;
  if (outA <= 0.0) discard;

  float opacity = outA * qt_Opacity;
  float alpha = shadowPaint.a * shadowOpacity * opacity;
  // Qt blends a ShaderEffect premultiplied and hands the colour uniform over
  // premultiplied too, so `shadowPaint.rgb` is already straight * a. The
  // fragment the blender wants is straight * a * coverage, so rgb carries the
  // coverage as well.
  fragColor = vec4(shadowPaint.rgb * shadowOpacity * opacity, alpha);
}
