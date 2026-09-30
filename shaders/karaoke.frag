#version 440

// Karaoke sweep: recolour the rendered glyphs along the reading order
// according to the continuously changing `progress` uniform. The glyph shape
// comes entirely from the text texture, so the transparent background is never
// touched.
//
// `coords` is a second texture holding, in its red channel, each fragment's
// normalised position along the reading order (left to right, then the next
// row). Using it instead of qt_TexCoord0.x keeps the sweep correct when a line
// wraps onto several rows.

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
    float progress;     // 0..1 reading-order highlight position
    float edge;         // width of the soft boundary, in line widths
    float dim;          // 1 = active line, <1 = dimmed inactive line
    float inactiveDim;  // the minimum of dim can get
    float glow;         // strength of the bright band at the playhead
    vec4  baseColor;    // colour of the part that has not been sung yet
    vec4  sungColor;    // colour of the part that has been sung
    vec4  glowColor;    // colour of the bright band at the playhead
};

layout(binding = 1) uniform sampler2D source;
layout(binding = 2) uniform sampler2D coords;

void main()
{
    vec4 txt = texture(source, qt_TexCoord0);
    float x = texture(coords, qt_TexCoord0).r;

    // 0 = already sung, 1 = not sung yet. Smoothstep gives the boundary a soft
    // gradient instead of a hard cut; the clamps keep a line that has not
    // started entirely unsung and a finished one entirely sung.
    float unsung;
    if (progress <= 0.0)
        unsung = 1.0;
    else if (progress >= 1.0)
        unsung = 0.0;
    else
        unsung = smoothstep(progress - edge, progress + edge, x);

    // The sung part carries a bright band centred on the playback position.
    vec3 sung = sungColor.rgb;
    if (progress > 0.0 && progress < 1.0) {
        float d = (x - progress) / max(edge, 1e-4);
        sung = mix(sung, glowColor.rgb, exp(-d * d) * glow);
    }

    vec3 rgb = mix(sung, baseColor.rgb, unsung);
    float opacity = mix(dim, inactiveDim, unsung);

    // Keep the glyph alpha so only text pixels are affected, and output
    // premultiplied colour as Qt Quick expects.
    float a = txt.a * opacity * qt_Opacity;
    fragColor = vec4(rgb * a, a);
}
