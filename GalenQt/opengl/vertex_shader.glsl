#version 330 core

uniform mat4 mvp_matrix;
uniform bool hasTexture;

in vec3 vertexPosition;
in vec2 vertexUV;
in vec4 vertexColour;

out vec2 fragmentUV;
out vec4 fragmentColour;

void main()
{
    if (hasTexture)
    {
        // Calculate vertex position in screen space
        gl_Position = mvp_matrix * vec4(vertexPosition, 1.0);

        // Pass texture coordinate to fragment shader
        // Value will be automatically interpolated to fragments inside polygon faces
        fragmentUV = vertexUV;
    }
    else
    {
        // Calculate vertex position in screen space
        gl_Position = mvp_matrix * vec4(vertexPosition, 1.0);

        // Pass vertex colour
        fragmentColour = vertexColour;
    }
}

