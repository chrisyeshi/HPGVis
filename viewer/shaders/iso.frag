#version 330

uniform vec2 invVP;
uniform sampler1D tf;
uniform sampler2DArray deparr;
uniform sampler2DArray nmlarr;
uniform vec3 lightDir;

in VertexData {
    vec2 texCoord;
} FragIn;

layout(location = 0, index = 0) out vec4 fragColor;

int nBins;
float stepp;
vec4 colors[16];
vec3 normals[16];
float depths[16];

float computeLighting(vec3 normal)
{
    vec3 N = normalize(normal);
    vec3 L = normalize(lightDir);
    vec3 E = normalize(vec3(0.0, 0.0, -1.0));
    vec3 R = normalize(reflect(-L,N));
    // ambient
    float Iamb = 0.10;
    // diffuse
    float Idiff = max(dot(-L,N), 0.0);
    Idiff = clamp(Idiff, 0.0, 1.0);
    // specular
    float Ispec = pow(max(dot(R,E),0.0),15);
    Ispec = clamp(Ispec, 0.0, 1.0);
    // combined
    return Iamb + Idiff + Ispec;
}

float alphaModify(float alpha, vec3 normal)
{
    vec3 E = normalize(vec3(0.0, 0.0, -1.0));
    vec3 N = normalize(normal);
    float EdotN = max(dot(-E,N), 0.0);
    float factor = (1.0 - EdotN) * 2.0 * (1.0 - alpha) + alpha;
    return alpha * pow(factor, 1.5);
}

vec4 iso()
{
    vec4 final = vec4(0.0);
    for (int i = 0; i < 16; ++i)
    {
        float l = computeLighting(normals[i]);
        vec4 c = colors[i];
        c.a = alphaModify(c.a, normals[i]);
        c.rgb = c.rgb * l * c.a;
        final += (1.0 - final.a) * c;
    }
    return final;
}

void main()
{
    nBins = 16;
    stepp = 1.0 / nBins;
    // colors
    for (int i = 0; i < 16; ++i)
        colors[i] = texture(tf, stepp * (float(i) + 0.5));
    // normals
    for (int i = 0; i < 16; ++i)
        normals[i] = texture(nmlarr, vec3(FragIn.texCoord, float(i))).xyz * 2.f - 1.f;
    // depths
    for (int i = 0; i < 16; ++i)
    {
        depths[i] = texture(deparr, vec3(FragIn.texCoord, float(i))).x;
        if (depths[i] < 0.001 || depths[i] > 0.999)
            colors[i] = vec4(0.0);
    }

    fragColor = iso();
    return;
}
