#version 440

// Karaoke sweep: recolour the rendered glyphs left to right according to the
// continuously changing `progress` uniform. The glyph shape comes entirely
// from the text texture, so the transparent background is never touched.

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
    float progress;     // 0..1 horizontal highlight position
    float edge;         // width of the soft boundary, in line widths
    float dim;          // 1 = active line, <1 = dimmed inactive line
    float glow;         // strength of the bright band at the playhead
    vec4  baseColor;    // colour of the part that has not been sung yet
    vec4  sungColorA;   // colour of the sung part at the left end of the line
    vec4  sungColorB;   // colour of the sung part at the right end of the line
    vec4  glowColor;    // colour of the bright band at the playhead
};

layout(binding = 1) uniform sampler2D source;

void main()
{
    vec4 txt = texture(source, qt_TexCoord0);
    float x = qt_TexCoord0.x;

    // 0 = already sung, 1 = not sung yet. Smoothstep gives the boundary a soft
    // gradient instead of a hard cut; the ends are clamped so a line that has
    // not started stays fully dim and a finished one stays fully highlighted.
    float unsung;
    if (progress <= 0.0)
        unsung = 1.0;
    else if (progress >= 1.0)
        unsung = 0.0;
    else
        unsung = smoothstep(progress - edge, progress + edge, x);
    // float unsung = step(progress, x);

    // The sung part carries a gradient across the whole line plus a bright band
    // centred on the current playback position.
    // vec3 sung = mix(sungColorA.rgb, sungColorB.rgb, clamp(x, 0.0, 1.0));
    vec3 sung = sungColorA.rgb;
    if (progress > 0.0 && progress < 1.0) {
        float d = (x - progress) / max(edge, 1e-4);
        sung = mix(sung, glowColor.rgb, exp(-d * d) * glow);
    }

    vec3 rgb = mix(sung, baseColor.rgb, unsung);

    // Keep the glyph alpha so only text pixels are affected, and output
    // premultiplied colour as Qt Quick expects.
    float a = txt.a * dim * qt_Opacity;
    fragColor = vec4(rgb * a, a);
}
