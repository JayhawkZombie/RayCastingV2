#version 430

layout (std430, binding = 2) buffer light_shader_data
{
  vec4 light_color;
  vec2 light_position; //16-byte alignment
  float light_attenuation;
  float light_intensity;
  int num_edges;
};

layout (std430, binding = 3) buffer edge_shader_data
{
  vec4 Edges[];
};

  /**
  *  RA(x2, y2)   * T (x4, y4)        L1: x1 + t * (x2 - x1)
  *      *        |                       y1 + t * (y2 - y1)
  *         *     | L2                 
  *            *  |                   L2: x3 + v * (x4 - x3)
  *              (*)                      y3 + v * (y4 - y3)
  *               |  * L1
  *               |     *
  *               |        * 
  *               B (x3, y3)  * R0 (x1, y1)
  *
  *  Simultaneous Equations:
  *       x1 + t * (x2 - x1) = x3 + v * (x4 - x3)
  *       y1 + t * (y2 - y1) = y3 + v * (y4 - y3)
  *
  *  Into matrix ==>  [ (x2 - x1)   -(x4 - x3)   |   (x3 - x1) ]  ==>  [ X21  -X43 | X31 ]
  *                   [ (y2 - y1)   -(y4 - y3)   |   (y3 - y1) ]       [ Y21  -Y43 | Y31 ]
  */

vec2 Solve2x2(float a, float b, float c, float d, float e, float f)
{
  float _det = (a * d) - (c * b);
  float factor = c / a;
  float r2c = factor * a;  c -= r2c;
  float r2d = factor * b;  d -= r2d;
  float r2f = factor * e;  f -= r2f;

    /**
  *  [  a          b          |          e         ] == became ==> [ a b | e ]
  *  [  0   d - (c / a) * b   |   f - (c / a) * e  ]               [ 0 d | f ]
  *
  * v = [ f - (c / d) * e ] / [ d - (c / a) * b ]
  */

  float _f = f / d; d = 1;
  /**
  *  [  a  b  |  e  ]
  *  [  0  1  |  f  ]
  *
  *  b -= b * 1;
  *  e -= b * f; <-- do first
  */
  float _e = e - b * _f; b = 1;
  /**
  *  [  a  0  |  e  ]
  *  [  0  1  |  f  ]
  */
  _e /= a; a = 1;
  /**
  *  [  1  0  |  e  ]
  *  [  0  1  |  f  ]
  *
  *   t = e
  *   v = f
  */

  return vec2(_e, _f);
}

vec2 CastRay(vec2 lPos, vec2 mePos, vec2 edgeStart, vec2 edgeEnd)
{
  float X21 = mePos.x - lPos.x;
  float X43 = edgeStart.x - edgeEnd.x;
  float X31 = edgeEnd.x - lPos.x;

  float Y21 = mePos.y - lPos.y;
  float Y43 = edgeStart.y - edgeEnd.y;
  float Y31 = edgeEnd.y - lPos.y;

  vec2 solve = Solve2x2(X21, -X43, Y21, -Y43, X31, Y31);

  float x = lPos.x + abs(solve.x) * X21;
  float y = lPos.y + abs(solve.y) * Y21;

  return vec2(x, y);
}

void main()
{
  
}