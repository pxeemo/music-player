#version 440

// Standard Qt Quick ShaderEffect vertex shader: pass the texture coordinates
// through and apply the item's combined transformation matrix.
//
// NOTE: the uniform block must be byte-for-byte identical to the one in
// karaoke.frag, including the members that are not used here. Both stages
// share a single uniform buffer bound at binding point 0.

layout(location = 0) in vec4 qt_Vertex;
layout(location = 1) in vec2 qt_MultiTexCoord0;

layout(location = 0) out vec2 qt_TexCoord0;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
    float progress;     // 0..1 horizontal highlight position
    float edge;         // width of the soft boundary, in line widths
    float dim;          // 1 = active line, <1 = dimmed inactive line
    float glow;         // strength of the bright band at the playhead
    vec4  baseColor;    // colour of the part that has not been sung yet
    vec4  sungColor;    // colour of the part that has been sung
    vec4  glowColor;    // colour of the bright band at the playhead
};

void main()
{
    qt_TexCoord0 = qt_MultiTexCoord0;
    gl_Position = qt_Matrix * qt_Vertex;
}
