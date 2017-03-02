#include "NewLightSystem.h"

int main()
{
  sf::ContextSettings cSettings;
  cSettings.antialiasingLevel = 8;
  cSettings.majorVersion = 4;
  cSettings.minorVersion = 3;
  sf::RenderWindow Window(sf::VideoMode(800, 800), "New lighting system", sf::Style::Default, cSettings);
  sf::RenderTexture Scene;
  Scene.create(800, 800);
  
  int light_index = 0;
  int light_index_2 = 0;
  int light_index_3 = 0;
  int light_index_4 = 0;

  LSystem system;
  system.SetWindowHeight(800.f);
  light_index = system.AddLight({ 400, 400 }, .05f, sf::Color(0, 255, 17), 200.f, 30.f, 200.f);
  //light_index_2 = system.AddLight({ 310, 375 }, .05f, sf::Color(255, 0, 0), 200.f, 30.f, 200.f);
  //light_index_3 = system.AddLight({ 450, 520 }, .05f, sf::Color(195, 0, 255), 200.f, 30.f, 200.f);
  light_index_4 = system.AddLight({ 450, 520 }, .05f, sf::Color(246, 255, 0), 200.f, 30.f, 200.f);
  system.CreateShadowedTexture(800, 800);
  system.CreateGlobalLightMap(800, 800);
  //testing shader-side lighting
  sf::Shader SSBOShader;
  SSBOShader.loadFromFile("SSBOLighting.fsh", sf::Shader::Fragment);

  //A couple tester casters

  sf::Vector2f pos = { 180, 200 };
  sf::Vector2f size = { 50, 190 };

  sf::RectangleShape Caster1;
  Caster1.setPosition({ 100, 100 });
  Caster1.setSize({ 150, 75 });

  //So the edges should be
  /*
  (100, 100) ... (250, 100)
  ________
  |
  |
  |
  ...
  |
  |_________
  (100, 175) ... (250, 175)
  */
  std::vector<Edge> Edges;
  Edges.push_back({});
  Edges.back().Start = { pos.x, pos.y };
  Edges.back().End = { pos.x + size.x, pos.y };

  Edges.push_back({});
  Edges.back().Start = { pos.x + size.x, pos.y };
  Edges.back().End = { pos.x + size.x, pos.y + size.y };

  Edges.push_back({});
  Edges.back().Start = { pos.x + size.x, pos.y + size.y };
  Edges.back().End = { pos.x, pos.y + size.y };

  Edges.push_back({});
  Edges.back().Start = { pos.x, pos.y + size.y };
  Edges.back().End = { pos.x, pos.y };
  //system.AddShadowCaster(Edges);

  sf::Vector2f pos2 = { 600, 450 };
  sf::Vector2f size2 = { 75, 250 };

  std::vector<Edge> Edges2;
  Edges2.push_back({});
  Edges2.back().Start = { pos2.x, pos2.y };
  Edges2.back().End = { pos2.x + size2.x, pos2.y };

  Edges2.push_back({});
  Edges2.back().Start = { pos2.x + size2.x, pos2.y };
  Edges2.back().End = { pos2.x + size2.x, pos2.y + size2.y };

  Edges2.push_back({});
  Edges2.back().Start = { pos2.x + size2.x, pos2.y + size2.y };
  Edges2.back().End = { pos2.x, pos2.y + size2.y };

  Edges2.push_back({});
  Edges2.back().Start = { pos2.x, pos2.y + size2.y };
  Edges2.back().End = { pos2.x, pos2.y };
  //system.AddShadowCaster(Edges2);

  sf::RectangleShape Rect1, Rect2;
  Rect1.setPosition(pos);
  Rect1.setSize(size);
  Rect1.setOutlineColor(sf::Color::White);
  Rect1.setOutlineThickness(-1);

  Rect2.setPosition(pos2);
  Rect2.setSize(size2);
  Rect2.setOutlineColor(sf::Color::White);
  Rect2.setOutlineThickness(-1);

  sf::RenderTexture BlendedScene;
  BlendedScene.create(800, 800);
  sf::RectangleShape BlendedRect;
  BlendedRect.setSize({ 800, 800 });
  BlendedRect.setTexture(&Scene.getTexture());

  sf::Texture LogoTexture;
  sf::RectangleShape LogoRect;
  LogoRect.setSize({ 800, 800 });

  LogoTexture.loadFromFile("SFEngineLogoLarge.png");
  LogoRect.setTexture(&LogoTexture);

  system.SetWindowHeight(800.f);

  sf::Event event;
  while (Window.isOpen()) {

    while (Window.pollEvent(event)) {
      if (event.type == sf::Event::Closed)
        Window.close();
    }


    system.MoveLight(light_index, static_cast<sf::Vector2f>(sf::Mouse::getPosition(Window)));

    system.UpdateLights();

    Window.clear(sf::Color::Black);
    BlendedScene.clear(sf::Color::Black);
    Scene.clear(sf::Color::Black);
    Scene.draw(LogoRect);
    //Scene.display();

    system.RenderOntoScene(Scene, BlendedScene);

    Scene.display();
    
    Window.draw(BlendedRect);
    Window.draw(Rect1);
    Window.draw(Rect2);
    
    Window.display();

  }

}
