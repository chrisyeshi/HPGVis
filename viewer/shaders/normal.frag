#version 330

uniform int nBins;
uniform vec2 vp;
uniform vec2 invVP;
uniform int layer;
uniform sampler2DArray deparr;

in VertexData {
    vec2 texCoord;
} FragIn;

layout(location = 0) out vec4 normal0;

void main()
{
    // 3 2 1
    // 4 C 0
    // 5 6 7
    vec3 coordsCtr = vec3(FragIn.texCoord, 0.0);
    vec3 coords[8];
    coords[0] = vec3(FragIn.texCoord + vec2(+invVP.x,       0), 0.0);
    coords[1] = vec3(FragIn.texCoord + vec2(+invVP.x,+invVP.y), 0.0);
    coords[2] = vec3(FragIn.texCoord + vec2(       0,+invVP.y), 0.0);
    coords[3] = vec3(FragIn.texCoord + vec2(-invVP.x,+invVP.y), 0.0);
    coords[4] = vec3(FragIn.texCoord + vec2(-invVP.x,       0), 0.0);
    coords[5] = vec3(FragIn.texCoord + vec2(-invVP.x,-invVP.y), 0.0);
    coords[6] = vec3(FragIn.texCoord + vec2(       0,-invVP.y), 0.0);
    coords[7] = vec3(FragIn.texCoord + vec2(+invVP.x,-invVP.y), 0.0);
    coordsCtr.z = texture(deparr, vec3(coordsCtr.xy, float(layer))).x;
    for (int i = 0; i < 8; ++i)
        coords[i].z = texture(deparr, vec3(coords[i].xy, float(layer))).x;
    vec3 dirs[8];
    for (int i = 0; i < 8; ++i)
        dirs[i] = coords[i] - coordsCtr;
    vec3 sum = vec3(0.0);
    for (int i = 0; i < 7; ++i)
    {
        float delta = 0.001f;
        vec3 normal;
        if (dirs[i].z > delta || dirs[i+1].z > delta)
            normal = vec3(0.0);
        else
            normal = cross(dirs[i], dirs[i+1]);
        sum += normal;
    }
    vec3 theNormal;
    if (sum == vec3(0.0))
        theNormal = vec3(0.0, 0.0, 1.0);
    else
        theNormal = normalize(sum);
    normal0 = vec4((theNormal + 1.f) * 0.5f, 1.0);
}
