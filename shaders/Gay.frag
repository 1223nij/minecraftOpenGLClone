#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec2 TexCoord2;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D ourTexture;
uniform sampler2D topTexture;
uniform vec4 tintColor;
uniform vec3 lightColor;
uniform vec3 lightPos;

void main()
{
   vec3 sunDirection = normalize(vec3(0.5, 1.0, 0.0)); // Example sun direction
   vec3 lightDir = normalize(lightPos - FragPos);
   float diff = max(dot(normalize(Normal), sunDirection), 0.0);
   vec3 diffuse = diff * lightColor;
   float ambientStrength = 0.3;
   vec3 ambient = ambientStrength * lightColor;
   // Sample both textures
   vec4 baseColor = texture(ourTexture, TexCoord);
   vec4 topColor = texture(topTexture, TexCoord2) * tintColor;
   vec4 color = mix(baseColor, topColor, topColor.a);
   // Use the top texture if it is not fully transparent, otherwise use the base texture
   FragColor = color;
   if (FragColor.a == 0.0) {
      discard;
   }
   vec3 result = (ambient + diffuse);
   FragColor = vec4(FragColor.xyz * result, FragColor.a);
   //FragColor = texture(topTexture, TexCoord2);
}