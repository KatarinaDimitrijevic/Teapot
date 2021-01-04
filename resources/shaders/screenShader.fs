#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform int SCR_WIDTH;
uniform int SCR_HEIGHT;
float offset = 1.0 / 300.0;

uniform bool blurEnabled;

void main()
{
    ivec2 viewportDim = ivec2(SCR_WIDTH, SCR_HEIGHT);
    ivec2 coords = ivec2(viewportDim * TexCoords);

    if(blurEnabled){
        vec2 offsets[9] = vec2[](
            vec2(-offset, offset),
            vec2(0.0f, offset),
            vec2(offset, offset),
            vec2(-offset, 0.0f),
            vec2(0.0f, 0.0f),
            vec2(0.0f, offset),
            vec2(-offset, -offset),
            vec2(0.0f, -offset),
            vec2(offset, -offset)
        );

        float kernel[9] = float[](
            1.0/16, 2.0/16, 1.0/16,
            2.0/16, 4.0/16, 2.0/16,
            1.0/16, 2.0/16, 1.0/16
        );

         vec3 sampleTex[9];
         for(int i = 0; i < 9;  i++){
            sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
         }

         vec3 col = vec3(0.0);
         for(int i=0; i<9 ;i++){
            col += sampleTex[i] * kernel[i];
         }

        FragColor = vec4(col, 1.0);
    }else{
        vec3 col = texture(screenTexture, TexCoords).rgb;
        FragColor = vec4(col, 1.0);
    }
}
