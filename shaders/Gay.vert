#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

// Instanced attributes
layout (location = 2) in vec2 aTexOffset;
layout (location = 3) in vec2 aTexOffsetOverlay;
layout (location = 4) in vec4 model1;
layout (location = 5) in vec4 model2;
layout (location = 6) in vec4 model3;
layout (location = 7) in vec4 model4;
layout (location = 8) in float visible;

out vec2 TexCoord;

uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord2;

void main()
{
    if (visible == 0.0)
    {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0); // Move vertices far off-screen
        return;
    }
    mat4 model;
    model[0] = model1;
    model[1] = model2;
    model[2] = model3;
    model[3] = model4;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord + (aTexOffset * 0.0625);
    TexCoord2 = aTexCoord + (aTexOffsetOverlay * 0.0625);
}