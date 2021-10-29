#version 330

// VS locations
#define POSITION    0
#define TEXCOORD    1

// FS locations
#define COORDINATE 0

in block
{
    vec2 TexCoord;
} FS_In;

out vec4 FragColor;

uniform sampler2D texsampler;

void main()
{
    //FragColor = vec4(FS_In.TexCoord, 0.0, 1.0);
    //FragColor = uvec4(1, 0, 1, 1);
    
    FragColor = texture(texsampler, FS_In.TexCoord);
}
