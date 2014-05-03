#version 330

//uniform int nBins;
uniform vec2 viewport;

uniform sampler1D tf;
uniform sampler2D raf0;
uniform sampler2D raf1;
uniform sampler2D raf2;
uniform sampler2D raf3;
uniform sampler1D rafa;
uniform sampler2D dep0;
uniform sampler2D dep1;
uniform sampler2D dep2;
uniform sampler2D dep3;

in VertexData {
    vec2 texCoord;
} FragIn;

out vec4 fragColor;

float step;
float intensity[16];
vec3 colors[16];
float binValue[16];
vec3 binLightMultiplier[16];
float depthValue[16];
int depthOrder[16];
vec3 normalValue[16];
float oldA[16];
float newA[16];
float K;
int nBins;
vec2 invVP;

void main()
{
    invVP.x = 1.0 / viewport.x;
    invVP.y = 1.0 / viewport.y;
    nBins = 16;
    K = 1.0;
    step = 1.0 / nBins;
    // colors
    for (int i = 0; i < 16; ++i)
    {
        intensity[i] = step * (i + 0.5);
        vec4 temp = texture(tf, intensity[i]);
        colors[i] = temp.rgb;
        newA[i] = temp.a;
        oldA[i] = texture(rafa, intensity[i]).r;
    }
    // binValues
    vec4 temp;
    temp = texture(raf0, FragIn.texCoord);
    binValue[0] = temp.x;
    binValue[1] = temp.y;
    binValue[2] = temp.z;
    binValue[3] = temp.w;
    temp = texture(raf1, FragIn.texCoord);
    binValue[4] = temp.x;
    binValue[5] = temp.y;
    binValue[6] = temp.z;
    binValue[7] = temp.w;
    temp = texture(raf2, FragIn.texCoord);
    binValue[8] = temp.x;
    binValue[9] = temp.y;
    binValue[10] = temp.z;
    binValue[11] = temp.w;
    temp = texture(raf3, FragIn.texCoord);
    binValue[12] = temp.x;
    binValue[13] = temp.y;
    binValue[14] = temp.z;
    binValue[15] = temp.w;

    temp = texture(dep0, FragIn.texCoord);
    depthValue[0] = temp.x;
    depthValue[1] = temp.y;
    depthValue[2] = temp.z;
    depthValue[3] = temp.w;
    temp = texture(dep1, FragIn.texCoord);
    depthValue[4] = temp.x;
    depthValue[5] = temp.y;
    depthValue[6] = temp.z;
    depthValue[7] = temp.w;
    temp = texture(dep2, FragIn.texCoord);
    depthValue[8] = temp.x;
    depthValue[9] = temp.y;
    depthValue[10] = temp.z;
    depthValue[11] = temp.w;
    temp = texture(dep3, FragIn.texCoord);
    depthValue[12] = temp.x;
    depthValue[13] = temp.y;
    depthValue[14] = temp.z;
    depthValue[15] = temp.w;

    vec2 xDelta = vec2(invVP.x, 0.0);
    vec2 yDelta = vec2(0.0, invVP.y);
    vec2 xPlus = FragIn.texCoord + xDelta;
    vec2 xLess = FragIn.texCoord - xDelta;
    vec2 yPlus = FragIn.texCoord + yDelta;
    vec2 yLess = FragIn.texCoord - yDelta;
    vec4 xDeltaDepth, yDeltaDepth;
    xDeltaDepth = texture(dep0, xPlus.xy) - texture(dep0, xLess.xy);
    yDeltaDepth = texture(dep0, yPlus.xy) - texture(dep0, yLess.xy);
    normalValue[0] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.x), vec3(yPlus - yLess, yDeltaDepth.x)));
    normalValue[1] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.y), vec3(yPlus - yLess, yDeltaDepth.y)));
    normalValue[2] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.z), vec3(yPlus - yLess, yDeltaDepth.z)));
    normalValue[3] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.w), vec3(yPlus - yLess, yDeltaDepth.w)));
    xDeltaDepth = texture(dep1, xPlus.xy) - texture(dep1, xLess.xy);
    yDeltaDepth = texture(dep1, yPlus.xy) - texture(dep1, yLess.xy);
    normalValue[4] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.x), vec3(yPlus - yLess, yDeltaDepth.x)));
    normalValue[5] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.y), vec3(yPlus - yLess, yDeltaDepth.y)));
    normalValue[6] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.z), vec3(yPlus - yLess, yDeltaDepth.z)));
    normalValue[7] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.w), vec3(yPlus - yLess, yDeltaDepth.w)));
    xDeltaDepth = texture(dep2, xPlus.xy) - texture(dep2, xLess.xy);
    yDeltaDepth = texture(dep2, yPlus.xy) - texture(dep2, yLess.xy);
    normalValue[8] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.x), vec3(yPlus - yLess, yDeltaDepth.x)));
    normalValue[9] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.y), vec3(yPlus - yLess, yDeltaDepth.y)));
    normalValue[10] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.z), vec3(yPlus - yLess, yDeltaDepth.z)));
    normalValue[11] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.w), vec3(yPlus - yLess, yDeltaDepth.w)));
    xDeltaDepth = texture(dep3, xPlus.xy) - texture(dep3, xLess.xy);
    yDeltaDepth = texture(dep3, yPlus.xy) - texture(dep3, yLess.xy);
    normalValue[12] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.x), vec3(yPlus - yLess, yDeltaDepth.x)));
    normalValue[13] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.y), vec3(yPlus - yLess, yDeltaDepth.y)));
    normalValue[14] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.z), vec3(yPlus - yLess, yDeltaDepth.z)));
    normalValue[15] = normalize(cross(vec3(xPlus - xLess, xDeltaDepth.w), vec3(yPlus - yLess, yDeltaDepth.w)));

    //In the Phong lighting model, we have four constants: ambient, specular, and diffuse reflections, and shininess (hardcoded)

    float KA = 0.1; //Ambient reflection
    float KS = 0.2; //Specular reflection
    float KD = 0.4; //Diffuse reflection

    const int numLights = 1;
    vec3 lightPositions[numLights];
    vec3 lightDiffuseIntensities[numLights];
    vec3 lightSpecularIntensities[numLights];

    vec3 AmbientIntensity = vec3(1, 1, 1);

    lightPositions[0] = vec3(1, 3, -.1);
    lightDiffuseIntensities[0] = vec3(1, 1, 1);
    lightSpecularIntensities[0] = vec3(1, 1, 1);


    // propagate alphas
    for (int i = 0; i < nBins; ++i)
    {        
        //Calculate local normal...


        //Get depth offsets...
//        float Amplifier = 1;
//        float zOffsetX = (depthValueOffsetX[i] - depthValue[i]) * Amplifier;
//        float zOffsetY = (depthValueOffsetY[i] - depthValue[i]) * Amplifier;

//        //Get two vectors along surface from these offsets:
//        vec3 surfaceX = vec3(offsetX.xy, zOffsetX);
//        vec3 surfaceY = vec3(offsetY.xy, zOffsetY);
//        normalValue[i] = normalize(cross(surfaceY, surfaceX));

        // if (normalValue[i].z > 0.0)
        //     normalValue[i] = vec3(1.0) - normalValue[i];

        vec3 pos = vec3(FragIn.texCoord.xy, depthValue[i]);

        //Add ambient...
        binLightMultiplier[i] = AmbientIntensity * KA;

        //For each light, add to the light multiplier according to the Phong model.
        for(int iCtr = 0; iCtr < numLights; iCtr++)
        {
            vec3 LM = lightPositions[iCtr] - pos;
            vec3 RM = reflect(normalize(-LM), normalValue[i]);

            //First diffuse...
            binLightMultiplier[i] += clamp(KD * dot(LM, normalValue[i]) * lightDiffuseIntensities[iCtr], 0, 1);

            //Next specular...
            binLightMultiplier[i] += clamp(vec3(pow(dot(RM, normalize(-pos)), 4)), 0, 1); //The magic number is shininess
        }

        //Keep the light multiplier in a reasonable range.
//        binLightMultiplier[i] = clamp(binLightMultiplier[i], 0, 2);
//        if(depthValue[i] == 1 || abs(zOffsetY) > 0.1 || abs(zOffsetX) > 0.1)
//            binLightMultiplier[i] = vec3(0); //Don't light the backplane or REALLY tangential ray hits.



        //If zOffsetX == 0 and zOffsetY == 0, the normal is directly at the camera.
        //If zOffsetX > 0, the normal is pointing toward the right side (if zOffsetX is larger than about 0.01, it's directly right)
        //If zOffsetY > 0, the normal is pointing toward the bottom...
        //If zOffsetX == 0.0001, the normal is at a 45 degree angle toward the right.


        if (abs(oldA[i] - newA[i]) < 0.0001)
            continue;
        if (oldA[i] < 0.0001)
        {
            binValue[i] = 0.0;
            continue;
        }
        binValue[i] = (newA[i] + 1.0 - pow(1.0 - newA[i], K + 1.0))
               / (oldA[i] + 1.0 - pow(1.0 - oldA[i], K + 1.0)) * binValue[i];
//        for (int j = i + 1; j < nBins; ++j)
//            binValue[j] = clamp(pow(1.0 - newA[i], K) / pow(1.0 - oldA[i], K) * binValue[j], 0, 1);
    }

    // sum

    //Sort the bins by depth (of first encounter). TODO: This probably shouldn't be insertion sort :/
    for(int iCtr = 0; iCtr < 16; iCtr++)
        depthOrder[iCtr] = iCtr;
    for(int iCtr = 0; iCtr < 16; iCtr++)
    {
        for(int jCtr = iCtr + 1; jCtr < 16; jCtr++)
        {
            if(depthValue[depthOrder[iCtr]] > depthValue[depthOrder[jCtr]])
            {
                int Temp = depthOrder[iCtr];
                depthOrder[iCtr] = depthOrder[jCtr];
                depthOrder[jCtr] = Temp;
            }
        }
    }

    vec4 sum = vec4(0.0);

    //sum = vec4(binLightMultiplier[5], 1);

    float AccumulatedOpacity = 0;

    for (int i = 0; i < 16; ++i)
    {
//        sum += vec4(binLightMultiplier[i], binValue[i]);

        sum += vec4(vec3(colors[i] * binLightMultiplier[i]) * binValue[i] * 2, binValue[i]);


        //AccumulatedOpacity += abs(binValue[i] / pow((float(i) + 0.5), 1));
    }

    fragColor = vec4(sum.rgb, 1);
}
