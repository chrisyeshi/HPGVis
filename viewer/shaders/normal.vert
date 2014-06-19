#version 330

in vec2 vertex;
in vec2 texCoord;

out VertexData {
    vec2 texCoord;
} VertexOut;

void main()
{
    gl_Position = vec4(vertex, 1.0, 1.0);
    VertexOut.texCoord = texCoord;
}
