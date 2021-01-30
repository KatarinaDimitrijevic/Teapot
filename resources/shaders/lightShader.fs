#version 330 core
out vec4 FragColor;

uniform bool spotLightEnabled;

void main()
{
    if(spotLightEnabled == false)
        FragColor = vec4(1.0);
    else
        FragColor = vec4(0.1);
}