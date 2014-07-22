#version 330

//uniform int nBins;
uniform vec2 invVP;
uniform sampler1D tf;
uniform sampler1D rafa;
uniform sampler2DArray rafarr;
uniform sampler2DArray deparr;
uniform sampler2DArray nmlarr;
uniform sampler2D featureID;
uniform float selectedID;
uniform int featureHighlight;
uniform vec3 lightDir;

in VertexData {
    vec2 texCoord;
} FragIn;

layout(location = 0, index = 0) out vec4 fragColor;

//vec2 invVP;
int nBins;
float stepp;
float K;
vec4 colors[16];
float oldA[16];
float newA[16];
float binValue[16];
vec2 gradients[16];

vec2 getGradient(int layer)
{
    float xp = texture(rafarr, vec3(FragIn.texCoord + vec2( invVP.x,     0.0), float(layer))).r;
    float xm = texture(rafarr, vec3(FragIn.texCoord + vec2(-invVP.x,     0.0), float(layer))).r;
    float yp = texture(rafarr, vec3(FragIn.texCoord + vec2(     0.0, invVP.y), float(layer))).r;
    float ym = texture(rafarr, vec3(FragIn.texCoord + vec2(     0.0,-invVP.y), float(layer))).r;
    return vec2((xp-xm)/2.0, (yp-ym)/2.0);
}

void main()
{
//    invVP = 1.0 * vec2(1.0 / 1024.0);
    nBins = 16;
    stepp = 1.0 / nBins;
    K = 1.0;
    // colors
    for (int i = 0; i < nBins; ++i)
    {
        float intensity = stepp * (float(i) + 0.5);
        colors[i] = texture(tf, intensity);
        newA[i] = colors[i].a;
        oldA[i] = 1.0 / 64.0; //texture(rafa, intensity).r; // NEED TEST
    }
    // bin values
    for (int i = 0; i < nBins; ++i)
    {
        binValue[i] = texture(rafarr, vec3(FragIn.texCoord, float(i))).r;
    }
    // scale bin values according to the difference between new and old alphas
    for (int i = 0; i < nBins; ++i)
    {
        binValue[i] = (newA[i] + 1.0 - pow(1.0 - newA[i], K + 1.0))
                    / (oldA[i] + 1.0 - pow(1.0 - oldA[i], K + 1.0)) * binValue[i];
    }
    // gradients
    for (int i = 0; i < nBins; ++i)
    {
        gradients[i] = getGradient(i);
    }
    // final coloring
    vec4 sum = vec4(0.0);
    for (int i = 0; i < nBins; ++i)
    {
        vec4 layerColor = vec4(vec3(colors[i].rgb) * binValue[i], binValue[i]);
        float ro = 9.0 * pow(length(gradients[i]), 0.5);
        sum += (1.0 - ro) * layerColor;
    }
//    fragColor = vec4(0.5 * (1.0 + (100.0 * vec2(gradients[8]))), 0.0, 1.0);
//    fragColor = vec4(vec3(binValue[8]), 1.0);
//    return;
    // output fragment color
    fragColor = vec4(sum.rgb, 1.0);
    return;
}
