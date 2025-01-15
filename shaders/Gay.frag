#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec2 TexCoord2;

uniform sampler2D ourTexture;
uniform sampler2D topTexture;
uniform vec4 tintColor;
uniform vec3 lightColor;

void main()
{
   // Sample both textures
   vec4 baseColor = texture(ourTexture, TexCoord);
   vec4 topColor = texture(topTexture, TexCoord2) * tintColor;
   vec4 color = mix(baseColor, topColor, topColor.a);
   // Use the top texture if it is not fully transparent, otherwise use the base texture
   FragColor = color;
   if (FragColor.a == 0.0) {
      discard;
   }
   FragColor = vec4(FragColor.x * lightColor.x, FragColor.y * lightColor.y, FragColor.z * lightColor.z, FragColor.a);
   //FragColor = texture(topTexture, TexCoord2);
}