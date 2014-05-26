#version 330

uniform vec2 invVP;
uniform sampler1D tf;
uniform sampler1D rafa;
uniform sampler2DArray rafarr;
uniform sampler2DArray deparr;
uniform sampler2DArray nmlarr;
uniform sampler2D featureID;
uniform float selectedID;
uniform int featureHighlight;

in VertexData {
    vec2 texCoord;
} FragIn;

layout(location = 0, index = 0) out vec4 fragColor;

float K;
float stepp;
vec4 colors[16];
float oldA[16];
float newA[16];
float binValue[16];
vec3 normalValue[16];
int nBins;
vec3 lightMult[16];
float KA, KS, KD;
vec3 LM;

vec3 getNormal(int layer)
{
    return texture(nmlarr, vec3(FragIn.texCoord + vec2(     0.0,     0.0), float(layer))).xyz * 2.f - 1.f;
//    // 6 7 8
//    // 3 4 5
//    // 0 1 2
//    vec3 sum = vec3(0.0);
//    sum += texture(nmlarr, vec3(FragIn.texCoord + vec2(-invVP.x,-invVP.y), float(layer))).xyz * 2.f - 1.f;
//    sum += texture(nmlarr, vec3(FragIn.texCoord + vec2(     0.0,-invVP.y), float(layer))).xyz * 2.f - 1.f;
//    sum += texture(nmlarr, vec3(FragIn.texCoord + vec2( invVP.x,-invVP.y), float(layer))).xyz * 2.f - 1.f;
//    sum += texture(nmlarr, vec3(FragIn.texCoord + vec2(-invVP.x,     0.0), float(layer))).xyz * 2.f - 1.f;
//    sum += texture(nmlarr, vec3(FragIn.texCoord + vec2(     0.0,     0.0), float(layer))).xyz * 2.f - 1.f;
//    sum += texture(nmlarr, vec3(FragIn.texCoord + vec2( invVP.x,     0.0), float(layer))).xyz * 2.f - 1.f;
//    sum += texture(nmlarr, vec3(FragIn.texCoord + vec2(-invVP.x, invVP.y), float(layer))).xyz * 2.f - 1.f;
//    sum += texture(nmlarr, vec3(FragIn.texCoord + vec2(     0.0, invVP.y), float(layer))).xyz * 2.f - 1.f;
//    sum += texture(nmlarr, vec3(FragIn.texCoord + vec2( invVP.x, invVP.y), float(layer))).xyz * 2.f - 1.f;
//    return normalize(sum);
}

void main()
{
    nBins = 16;
    stepp = 1.0 / nBins;
    K = 1.0;
    // colors
    for (int i = 0; i < nBins; ++i)
    {
        float intensity = stepp * (float(i) + 0.5);
        colors[i] = texture(tf, intensity);
        newA[i] = colors[i].a;
        oldA[i] = 1.0 / 2.0;
    }
    // bin values
    for (int i = 0; i < nBins; ++i)
        binValue[i] = texture(rafarr, vec3(FragIn.texCoord, float(i))).r;
    // normal values
    for (int i = 0; i < nBins; ++i)
        normalValue[i] = getNormal(i);
    // lighting parameters
    KA = 0.1; //Ambient reflection
    KS = 0.2; //Specular reflection
    KD = 0.4; //Diffuse reflection
    LM = normalize(vec3(1, 1, 1));
    // propagate alphas
    for (int i = 0; i < nBins; ++i)
    {
        // lighting
        lightMult[i] = vec3(1.0) * KA;
        vec3 RM = normalize(reflect(LM, normalValue[i]));
        lightMult[i] += clamp(KD * dot(LM, normalValue[i]), 0, 1);
        lightMult[i] += clamp(KS * vec3(pow(dot(RM, vec3(0.0,0.0,-1.0)), 4)), 0, 1); //The magic number is shininess
        // propagation
        if (abs(oldA[i] - newA[i]) < 0.0001)
            continue;
        if (oldA[i] < 0.0001)
        {
            binValue[i] = 0.0;
            continue;
        }
        binValue[i] = (newA[i] + 1.0 - pow(1.0 - newA[i], K + 1.0))
               / (oldA[i] + 1.0 - pow(1.0 - oldA[i], K + 1.0)) * binValue[i];
        for (int j = i + 1; j < nBins; ++j)
            binValue[j] = clamp(pow(1.0 - newA[i], K) / pow(1.0 - oldA[i], K) * binValue[j], 0, 1);
    }
    // final coloring
    vec4 sum = vec4(0.0);
    float AccumulatedOpacity = 0;
    for (int i = 0; i < nBins; ++i)
    {
        sum += vec4(vec3(colors[i].rgb * lightMult[i]) * binValue[i] * 2, binValue[i]);
    }
    // feature extraction/tracking
    vec4 ID = texture(featureID, FragIn.texCoord);
    if (featureHighlight == 0)
    {
        fragColor = vec4(sum.rgb, 1.0);
        return;
    }
    if (abs(ID.x - selectedID) > 1.0/10000.0) {
        fragColor = vec4(sum.rgb, 0.4);
    } else {
        fragColor = vec4(sum.r, sum.g, 1.0, 1.0);
    }
}
