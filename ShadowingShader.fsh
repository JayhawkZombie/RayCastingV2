#version 120

uniform sampler2D LightMap;
uniform sampler2D ShadowMap;

void main()
{

  vec4 light_color = texture2D(LightMap, gl_TexCoord[0].st);
  vec4 shadow_color = texture2D(ShadowMap, gl_TexCoord[0].st);

  //If the color is light, we will brighten it (if needed)

  float intensity = length(shadow_color);

  gl_FragColor = light_color * intensity;
}
