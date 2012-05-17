#version 330

uniform vec3 ambientColor = vec3(0.0, 0.0, 0.0);
uniform vec3 diffuseColor;
uniform vec3 specularColor = vec3(1.0, 1.0, 1.0);
uniform vec3 lightColor[LIGHT_COUNT];
uniform float shininess = 80.0;

in vec3 transformedNormal;
in vec3 lightDirection[LIGHT_COUNT];
in vec3 cameraDirection;

out vec4 color;

void main() {
    /* Ambient color */
    color.rgb = ambientColor;

    vec3 normalizedTransformedNormal = normalize(transformedNormal);

    for(int i = 0; i != LIGHT_COUNT; ++i) {
        vec3 normalizedLightDirection = normalize(lightDirection[i]);

        /* Add diffuse color */
        float intensity = max(0.0, dot(normalizedTransformedNormal, normalizedLightDirection));
        color.rgb += diffuseColor*lightColor[i]*intensity;

        /* Add specular color, if needed */
        if(intensity >= 0.0) {
            vec3 reflection = reflect(-normalizedLightDirection, normalizedTransformedNormal);
            float specularity = pow(max(0.0, dot(normalize(cameraDirection), reflection)), shininess);
            color.rgb += specularColor*specularity;
        }
    }

    /* Force alpha to 1 */
    color.a = 1.0;
}
