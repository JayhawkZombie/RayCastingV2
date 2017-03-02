#pragma once
#include <GL/glew.h>

#include <iostream>
#include <SFML\Graphics.hpp>
#include <SFML\OpenGL.hpp>

void normalize(sf::Vector2f &v)
{
  float mag = (v.x * v.x) + (v.y * v.y);
  mag = sqrt(mag);
  v = sf::Vector2f(v.x / mag, v.y / mag);
}

struct Edge
{
  sf::Vector2f Start;
  sf::Vector2f End;

  //Polar form
  float r = 0.f;
  float mag = 0.f;
};

struct light_stc_data_type
{
  sf::Glsl::Vec4 light_color;
  sf::Glsl::Vec2 light_position;
  float light_attenuation;
  float light_intensity;
};

struct edge_data_type
{
  sf::Glsl::Vec4 *Edges;
};

struct shadow_caster_data_type
{
  sf::Glsl::Vec2 EdgeStart;
  sf::Glsl::Vec2 EdgeEnd;
};

class Light {
public:
  Light() = default;
  Light(const Light &other)
    :
    Position(other.Position),
    Radius(other.Radius),
    Color(other.Color),
    Expand(other.Expand)
  {}

  float Attenuation = 0.f;
  sf::Vector2f Position;
  float Radius = 0.f;
  sf::Color Color;
  float Expand = 0.f;
  int ID = 0;
  float Intensity = 1.f;
  sf::CircleShape Circle;

  sf::VertexArray LightVerts;
  sf::VertexArray Shadowverts;
};

struct LightObject {
  std::vector<Edge> Edges;
};

class LSystem
{
public:
  LSystem()
  {
    ShadowingShader.loadFromFile("ShadowingShader.fsh", sf::Shader::Fragment);
    BlendShader.loadFromFile("MaskShader.fsh", sf::Shader::Fragment);
    LightShader.loadFromFile("SuperBright.fsh", sf::Shader::Fragment);
    ShadowRegions = sf::VertexArray(sf::Triangles);
  }

  void CreateGlobalLightMap(int height, int width) {
    GlobalLightMap.create(width, height);

    GLint size = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &size);
    std::cout << "Max textue size: " << size << std::endl;
  }

  int AddLight(const sf::Vector2f &position, float intensity, const sf::Color &color, float attenuation, float expand, float radius) {
    static int lights_int = 0;
    lights_int++;

    Lights[lights_int] = {};
    Lights[lights_int].Intensity = intensity;
    Lights[lights_int].Attenuation = attenuation;
    Lights[lights_int].Color = color;
    Lights[lights_int].Expand = expand;
    Lights[lights_int].Position = position;
    Lights[lights_int].Radius = radius;
    Lights[lights_int].ID = lights_int;
    Lights[lights_int].LightVerts = sf::VertexArray(sf::Triangles);
    Lights[lights_int].Shadowverts = sf::VertexArray(sf::Triangles);
    CreateLightTexture(Lights[lights_int]);
    return lights_int;
  }

  void RemoveLight() {

  }

  void MoveLight(int index, const sf::Vector2f &NewPos) {
    auto light = Lights.find(index);
    if (light != Lights.end())
      light->second.Position = NewPos;
  }

  void UpdateLights() {
    //For each light, update it
    ShadowRegions.clear();
    
    for (auto & light : Lights)
      UpdateLight(light.second);
  }

  void AddShadowCaster(const std::vector<Edge> Edges) {
    static int caster_id = 0;
    caster_id++;
    Casters[caster_id] = {};
    for (auto & edge : Edges) {
      Casters[caster_id].Edges.push_back({});
      Casters[caster_id].Edges.back().Start = edge.Start;
      Casters[caster_id].Edges.back().End = edge.End;
    }
  }

  void RenderOntoScene(sf::RenderTexture &SceneTexture, sf::RenderTexture &NewSceneTexture) {
    //We should have the light map ready to go, so all we should have to do is blend it
    sf::RectangleShape rect;
    rect.setSize(static_cast<sf::Vector2f>(NewSceneTexture.getSize()));
    sf::RenderStates state;

    state.blendMode = sf::BlendAdd;
    state.shader = &BlendShader;
    rect.setTexture(&SceneTexture.getTexture());
    //rect.setTexture(&LightMaps[light.second.ID].getTexture());
    //Now plow it through
    SceneTexture.draw(rect, state);

    //For each light that we have in this system, render a big 'ol quad and push it through the fragment shader
    for (auto & light : Lights) {
      CreateLightMap(light.second);

      //Now that we have the maps, we need to blend it with the scene
      BlendShader.setUniform("MaskTexture", LightMaps[light.second.ID].getTexture());
      BlendShader.setUniform("SceneTexture", SceneTexture.getTexture());
      BlendShader.setUniform("MinimumIntensity", 1.0f);
      BlendShader.setUniform("LightHue", sf::Glsl::Vec4(light.second.Color.r, light.second.Color.g, light.second.Color.b, light.second.Color.a));
      BlendShader.setUniform("HueIntensity", light.second.Intensity);
      BlendShader.setUniform("MaximumIntensity", 5.f);
      BlendShader.setUniform("AmbientColor", sf::Glsl::Vec4(255, 255, 255, 0));
      BlendShader.setUniform("AmbientIntensity", 0.1f);

      state.blendMode = sf::BlendAlpha;
      state.shader = &BlendShader;
      rect.setTexture(&SceneTexture.getTexture());
      //rect.setTexture(&LightMaps[light.second.ID].getTexture());
      //Now plow it through
      SceneTexture.draw(rect, state);
    }

  }

  void GPUInit(sf::RenderWindow &CurrentWindow)
  {
    auto settings = sf::Context::getActiveContext()->getSettings();
    if (settings.majorVersion < 4 || (settings.majorVersion == 4 && settings.minorVersion < 3)) {
      std::cerr << "Incompatable context version made. A 4.3+ context is required" << std::endl;
      return;
    }

    light_stc_data_type light_alloc;
    light_alloc.light_attenuation = 0.f;
    light_alloc.light_color = sf::Glsl::Vec4(0.f, 0.f, 0.f, 0.f);
    light_alloc.light_position = sf::Glsl::Vec2(0.f, 0.f);

    edge_data_type edge_alloc;
    edge_alloc.Edges = new sf::Glsl::Vec4[1];

    //Generate the buffers for the SSBO
    glGenBuffers(1, &LightSSBOBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, LightSSBOBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(light_stc_data_type), &light_alloc, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, LightSSBOBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenBuffers(1, &EdgeSSBOBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, EdgeSSBOBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(sf::Glsl::Vec4), &edge_alloc, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, EdgeSSBOBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    delete[] edge_alloc.Edges;
  }

  void GPURender()
  {
    //Oh god, here we go
    
    //update light data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, LightSSBOBuffer);
    GLvoid *p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, &TestGPULight, sizeof(light_stc_data_type));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    
  }

  void SetWindowHeight(float h) {
    WindowHeight = h;
  }

  void CreateShadowedTexture(int x, int y)
  {
    ShadowedLights.create(x, y);
  }

  void DrawSegments(sf::RenderTarget &target) {
    target.draw(TestTriangles);
  }
  
protected:
  void CreateLightTexture(const Light &light) {
    LightShader.setUniform("LightColor", sf::Glsl::Vec3(255, 255, 255));
    LightShader.setUniform("LightOrigin", light.Position);
    LightShader.setUniform("Attenuation", light.Attenuation);
    LightShader.setUniform("ScreenResolution", sf::Glsl::Vec2(WindowHeight, WindowHeight));

    LightTextures[light.ID].create(800, 800);
    LightTextures[light.ID].clear(sf::Color::Transparent);
    ShadowMaps[light.ID].create(800, 800);
    ShadowMaps[light.ID].clear(sf::Color::Transparent);
    LightMaps[light.ID].create(800, 800);
    LightMaps[light.ID].clear(sf::Color::Transparent);
   
    sf::CircleShape circle;
    circle.setRadius(light.Attenuation);
    circle.setOrigin(light.Attenuation, light.Attenuation);
    circle.setPosition({ 400, 400 });
    circle.setFillColor(sf::Color::Transparent);

    LightTextures[light.ID].draw(circle, &LightShader);
    LightTextures[light.ID].display();

    auto image = LightTextures[light.ID].getTexture().copyToImage();
    image.saveToFile("NEWLIGHTSYSTEMTEST" + std::to_string(light.ID) + ".png");
  }

  void CreateCombinedLightMap(sf::RenderTexture &Target)
  {
    static bool once = false;

    sf::CircleShape circle;
    sf::RenderStates state;

    state.blendMode = sf::BlendMultiply;

    Target.clear(sf::Color::Transparent);
    for (auto & light : Lights) {
      LightShader.setUniform("LightColor", sf::Glsl::Vec3(light.second.Color.r, light.second.Color.g, light.second.Color.b));
      LightShader.setUniform("LightOrigin", light.second.Position);
      LightShader.setUniform("Attenuation", light.second.Attenuation);
      LightShader.setUniform("ScreenResolution", sf::Glsl::Vec2(WindowHeight, WindowHeight));

      state.shader = &LightShader;
      state.blendMode = sf::BlendAdd;
      circle.setRadius(light.second.Attenuation);
      circle.setOrigin(light.second.Attenuation, light.second.Attenuation);
      circle.setPosition(light.second.Position);
      circle.setFillColor(sf::Color::Transparent);

      Target.draw(circle, state);
    }

    state.blendMode = sf::BlendAlpha;
    state.shader = nullptr;
    Target.draw(ShadowRegions, state);

    //for (auto & light : Lights) {
    //  state.blendMode = sf::BlendAlpha;
    //  state.shader = nullptr;
    //  
    //  Target.draw(light.second.Shadowverts, state);
    //}

    Target.display();
    state.blendMode = sf::BlendAlpha;
    state.texture = &Target.getTexture();
    for (auto & light : Lights) {
      Target.draw(light.second.Shadowverts, state);
    }

    if (!once) {
      auto image = Target.getTexture().copyToImage();
      image.saveToFile("combinedlightmap.png");
      once = true;
    }
  }

  void CreateLightMap(const Light &light) {
    //Just render the radial shader onto the texture, then apply the shadow regions
    sf::RectangleShape rect;
    rect.setSize(static_cast<sf::Vector2f>(LightTextures[light.ID].getSize()));
    rect.setTexture(&LightTextures[light.ID].getTexture());
    sf::RenderStates state;

    //Now draw the normal radial light gradient to the LightMap texture using our light triangles
    state.texture = &LightTextures[light.ID].getTexture();
    LightMaps[light.ID].clear(sf::Color::Transparent);
    LightMaps[light.ID].draw(light.LightVerts, state);

    //Now draw the shadow regions
    state.blendMode = sf::BlendMultiply; //We want the black regions to COMPLETELY remove the lighting effect (ie 0 * anything = 0, no color added when blending)
    state.texture = &LightTextures[light.ID].getTexture();
    LightMaps[light.ID].draw(light.Shadowverts, state);

    //And display the texture
    LightMaps[light.ID].display();
  }

  /*
                                   * (4)
                                    
                                *  * *
                                                        
                             *     *   *
                                T2
                          *(3)    *       *
                          |
                        * |      *          *
                          |  
                      *   |     *              *
                          |              T1         
                    *     |    *                  *(2)
                          |                 *
                  *       |           *
                          |   *
                *         *(1)
                    *
           L  *
  */

  void UpdateLight(Light &light) {
    static sf::Vector2f vToEdge1, vToEdge2;
    static sf::Vector2f outerPt1, outerPt2;
    static sf::Vector2f OffsetFromCenterOfTexture;
    static sf::Vector2f TextureSize;

    //Find out where the light is relative to the center of the texture
    TextureSize = static_cast<sf::Vector2f>(LightTextures[light.ID].getSize());
    OffsetFromCenterOfTexture = light.Position - sf::Vector2f(TextureSize.x / 2.f, TextureSize.y / 2.f);

    //we wll always have 4 vertices
    sf::Vertex V1, V2, V3, V4;
    light.Shadowverts.clear();
    light.Shadowverts = sf::VertexArray(sf::Triangles);

    light.LightVerts.clear();
    light.LightVerts = sf::VertexArray(sf::Triangles);

    //we will have 4 points that shoot out in 4 directions w the length of the attenuation radius
    sf::VertexArray TL, TR, BR, BL;

    //sf::Vector2f vToTL = { -0.7071678f, 0.7071678f };
    //sf::Vector2f vToTR = { 0.7071678f , 0.7071678f };
    //sf::Vector2f vToBR = { 0.7071678f , -0.7071678f };
    //sf::Vector2f vToBL = { -0.7071678f , -0.7071678f };

    sf::Vector2f vToTL = { -1, 1 };
    sf::Vector2f vToTR = { 1, 1 };
    sf::Vector2f vToBR = { 1, -1 };
    sf::Vector2f vToBL = { -1, -1 };

    sf::Vector2f outTL = light.Position + vToTL * 1.4142135f * light.Attenuation;
    sf::Vector2f outTR = light.Position + vToTR * 1.4142135f * light.Attenuation;
    sf::Vector2f outBR = light.Position + vToBR * 1.4142135f * light.Attenuation;
    sf::Vector2f outBL = light.Position + vToBL * 1.4142135f * light.Attenuation;

    sf::Vertex VCenter, VOutTL, VOutTR, VOutBR, VOutBL;
    VCenter.position = light.Position;
    VCenter.texCoords = light.Position - OffsetFromCenterOfTexture;

    VOutTL.position = outTL;
    VOutTL.texCoords = outTL - OffsetFromCenterOfTexture;

    VOutTR.position = outTR;
    VOutTR.texCoords = outTR - OffsetFromCenterOfTexture;

    VOutBR.position = outBR;
    VOutBR.texCoords = outBR - OffsetFromCenterOfTexture;

    VOutBL.position = outBL;
    VOutBL.texCoords = outBL - OffsetFromCenterOfTexture;

    //Light triangle 1 : VCenter -> VOutTL -> VOutRT
    light.LightVerts.append(VCenter);
    light.LightVerts.append(VOutTL);
    light.LightVerts.append(VOutTR);

    //Light triangle 2 : VCenter -> VOutTR -> VOutBR
    light.LightVerts.append(VCenter);
    light.LightVerts.append(VOutTR);
    light.LightVerts.append(VOutBR);

    //Light triangle 3 : VCenter -> VOutBR -> VOutBL
    light.LightVerts.append(VCenter);
    light.LightVerts.append(VOutBR);
    light.LightVerts.append(VOutBL);

    //Light triangle 4 : VCenter -> VOutBL -> VOutTL
    light.LightVerts.append(VCenter);
    light.LightVerts.append(VOutBL);
    light.LightVerts.append(VOutTL);

    TestTriangles.clear();
    TestTriangles = sf::VertexArray(sf::Triangles);

    const sf::Vector2f lightSource = light.Position;
    
    for (auto & obj : Casters) {
      //For each edge, we will create 2 triangles out of it
      //Some will overlap, but they are all black, so it shouldn't hurt us if we just use a BlendAdd - we can optimize it away later
      
      for (auto & edge : obj.second.Edges) {
        vToEdge1 = edge.Start - light.Position;
        vToEdge2 = edge.End - light.Position;
        normalize(vToEdge1); normalize(vToEdge2);

        outerPt1 = edge.Start + (vToEdge1 * light.Attenuation);
        outerPt2 = edge.End + (vToEdge2 * light.Attenuation);

        V1.position = edge.Start; V1.color = sf::Color(0, 0, 0, 0); //V1.texCoords = edge.Start - OffsetFromCenterOfTexture;
        V2.position = outerPt1;   V2.color = sf::Color(0, 0, 0, 0); //V2.texCoords = outerPt1 - OffsetFromCenterOfTexture;
        V3.position = edge.End;   V3.color = sf::Color(0, 0, 0, 0); //V2.texCoords = edge.End - OffsetFromCenterOfTexture;
        V4.position = outerPt2;   V4.color = sf::Color(0, 0, 0, 0); //V3.texCoords = outerPt2 - OffsetFromCenterOfTexture;

        //Triangle 1 : V1 -> V2 -> V3
        ShadowRegions.append(V1);
        ShadowRegions.append(V2);
        ShadowRegions.append(V3);

        //Triangle 2 : V2 -> V3 -> V4
        ShadowRegions.append(V2);
        ShadowRegions.append(V3);
        ShadowRegions.append(V4);

        //Triangle 1 : V1 -> V2 -> V3
        light.Shadowverts.append(V1);
        light.Shadowverts.append(V2);
        light.Shadowverts.append(V3);

        //Triangle 2 : V2 -> V3 -> V4
        light.Shadowverts.append(V2);
        light.Shadowverts.append(V3);
        light.Shadowverts.append(V4);
      }
      
    }


  }

  std::map<int, Light> Lights;
  std::map<int, LightObject> Casters;

  std::map<int, sf::RenderTexture> LightTextures;
  std::map<int, sf::RenderTexture> LightMaps;
  std::map<int, sf::RenderTexture> ShadowMaps;

  //One main renderTexture for drawing all the lights to
  sf::RenderTexture ShadowedLights;
  sf::RenderTexture GlobalLightMap;

  sf::VertexArray TestTriangles;
  sf::VertexArray ShadowRegions;

  sf::Shader LightShader;
  sf::Shader BlendShader;
  sf::Shader ShadowingShader;

  float WindowHeight = 0.f;

  //Please ignore, shader-only lighting in the works!
  bool UseGPU = false;
  GLuint LightSSBOBuffer;
  GLuint EdgeSSBOBuffer;

  light_stc_data_type TestGPULight;
};
