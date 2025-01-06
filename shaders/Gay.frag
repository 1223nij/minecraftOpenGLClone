#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;
in vec2 TexCoord2;

uniform sampler2D ourTexture;
uniform sampler2D topTexture;
uniform vec4 tintColor;

void main()
{
   // Sample both textures
   vec4 baseColor = texture(ourTexture, TexCoord);
   vec4 topColor = texture(topTexture, TexCoord2) * tintColor;

   // Use the top texture if it is not fully transparent, otherwise use the base texture
   FragColor = mix(baseColor, topColor, topColor.a);
   //FragColor = texture(topTexture, TexCoord2);
}