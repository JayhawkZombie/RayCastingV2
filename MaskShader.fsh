#version 120

/*
  Shder to blend the light map with the scene's texture
  -- Render the scene to a renderTexture and give that to the light system
  -- This will blend the lightmap with that texture and you can then render your scene texture to the window
*/

uniform sampler2D MaskTexture;
uniform sampler2D SceneTexture;

uniform float MinimumIntensity;
uniform vec4 AmbientColor;
uniform float AmbientIntensity;
uniform vec4 LightHue;
uniform float HueIntensity;
uniform float MaximumIntensity;

void main()
{
  //vec4 color = normalize(AmbientColor) * AmbientIntensity + texture2D(SceneTexture, gl_TexCoord[0].st);
  vec4 color = texture2D(SceneTexture, gl_TexCoord[0].st);
  vec4 maskColor = texture2D(MaskTexture, gl_TexCoord[0].st);

  vec4 light_influence = vec4(0, 0, 0, 0);
  
  float intensity = length(maskColor.rgb);
  light_influence = maskColor * LightHue * HueIntensity;
  if (intensity > 0)
    color.rgb = color.rgb + color.rgb * light_influence.rgb * light_influence.a;
  
  gl_FragColor = color;
}