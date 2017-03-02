uniform vec2 LightOrigin;

uniform vec3 LightColor;
uniform float Attenuation;
uniform vec2 ScreenResolution;

void main()
{
  vec2 baseDistance =  gl_FragCoord.xy;
  baseDistance.y = ScreenResolution.y - baseDistance.y;

  vec2 distance = LightOrigin - baseDistance;
  float linear_distance = length(distance);

  float d = sqrt(linear_distance / Attenuation);
  float atten = 1.0 - (sqrt(d));

  vec4 lightColor = vec4(LightColor, 1.0) * atten;
  gl_FragColor = lightColor;
};