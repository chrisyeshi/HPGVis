#version 330

uniform vec2 invVP;
uniform sampler1D tf;
uniform sampler2DArray rafarr;
uniform sampler2DArray deparr;
uniform sampler2DArray nmlarr;
uniform vec3 lightDir;
uniform sampler2D features;
uniform float featureID;
uniform int featureEnable;
uniform int enableDepthaware;
uniform int enableIso;
uniform int enableOpacityMod;

in VertexData {
    vec2 texCoord;
} FragIn;

layout(location = 0, index = 0) out vec4 fragColor;

int nBins;
float stepp;
vec4 colors[16];
vec3 normals[16];
float depths[16];
float litFactors[16];
float binValues[16];

//
//
// Common Functions
//
//

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
    return alpha * pow(factor, 2.5);
}

vec2 getGradient(int layer)
{
    float xp = texture(rafarr, vec3(FragIn.texCoord + vec2( invVP.x,     0.0), float(layer))).r;
    float xm = texture(rafarr, vec3(FragIn.texCoord + vec2(-invVP.x,     0.0), float(layer))).r;
    float yp = texture(rafarr, vec3(FragIn.texCoord + vec2(     0.0, invVP.y), float(layer))).r;
    float ym = texture(rafarr, vec3(FragIn.texCoord + vec2(     0.0,-invVP.y), float(layer))).r;
    return vec2((xp-xm)/2.0, (yp-ym)/2.0);
}

vec4 iso()
{
    vec4 final = vec4(0.0);
    for (int i = 0; i < 16; ++i)
    {
        float l = litFactors[i];
        vec4 c = colors[i];
        c.a = alphaModify(c.a, normals[i]);
        c.rgb = c.rgb * l * c.a;
        final += (1.0 - final.a) * c;
    }
    return final;
}

vec4 raf()
{
    vec4 sum = vec4(0.0);
    for (int i = 0; i < 16; ++i)
    {
        vec4 layerColor = vec4(vec3(colors[i].rgb) * binValues[i], binValues[i]);
        if (enableDepthaware == 1 && enableIso == 1)
        {
            float ro = 9.0 * pow(length(getGradient(i)), 0.5);
            sum += (1.0 - ro) * (1.0 - colors[i].a) * layerColor;
        } else if (enableDepthaware == 1)
        {
            float ro = 9.0 * pow(length(getGradient(i)), 0.5);
            sum += (1.0 - ro) * layerColor;
        } else if (enableIso == 1)
        {
            sum += (1.0 - colors[i].a) * layerColor;
        } else
        {
            sum += layerColor;
        }
    }
    return sum;
}

vec4 isoraf()
{
    vec4 final = vec4(0.0);
    for (int i = 0; i < 16; ++i)
    {
        float l = litFactors[i];
        vec4 cIso = colors[i];
        cIso.a = alphaModify(cIso.a, normals[i]);
        cIso.rgb = cIso.rgb * l * cIso.a;
        vec4 layerColor = vec4(vec3(colors[i].rgb) * binValues[i], 1.0);
        float ro = 9.0 * pow(length(getGradient(i)), 0.5);
        vec4 cRaf = (1.0 - ro) * layerColor;
        vec4 c = vec4(cIso.rgb + (1.0 - colors[i].a) * cRaf.rgb, cIso.a);
        final += (1.0 - final.a) * c;
    }
    return final;
}

//
//
// main()
//
//

void main()
{
    nBins = 16;
    stepp = 1.0 / nBins;
    // colors
    for (int i = 0; i < 16; ++i)
        colors[i] = texture(tf, stepp * (float(i) + 0.5));
    // normals
    for (int i = 0; i < 16; ++i)
    {
        normals[i] = normalize(texture(nmlarr, vec3(FragIn.texCoord, float(i))).xyz * 2.f - 1.f);
        if (normals[i].z < 0.0)
            colors[i] = vec4(0.0);
    }
    // depths
    for (int i = 0; i < 16; ++i)
    {
        depths[i] = texture(deparr, vec3(FragIn.texCoord, float(i))).x;
        if (depths[i] < 0.001 || depths[i] > 0.999)
            colors[i] = vec4(0.0);
    }
    // lights
    for (int i = 0; i < 16; ++i)
        litFactors[i] = computeLighting(normals[i]);
    // bin values
    for (int i = 0; i < 16; ++i)
    {
        float K = 1.0;
        float oldA = 1.0 / 16.0;
        binValues[i] = texture(rafarr, vec3(FragIn.texCoord, float(i))).r;
        if (enableOpacityMod == 1)
        {
            binValues[i] = (colors[i].a + 1.0 - pow(1.0 - colors[i].a, K + 1.0))
                         / (oldA + 1.0 - pow(1.0 - oldA, K + 1.0)) * binValues[i];
        }
    }

    vec4 cIso = iso();
    vec4 cRaf = raf();
    if (enableIso == 1)
        fragColor = vec4(cIso.rgb + /*(1.0 - cIso.a) * */cRaf.rgb, 1.0);
    else
        fragColor = vec4(cRaf.rgb, 1.0);

    // feature tracking
    if (featureEnable == 1)
    {
        float id = texture(features, FragIn.texCoord).x;
        if (abs(id - featureID) > 1.0 / 10000.0)
            fragColor = vec4(fragColor.rgb, 0.4);
        else
            fragColor = vec4(fragColor.rg, 1.0, 1.0);
    }

    return;
}
