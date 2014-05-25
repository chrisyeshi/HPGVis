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

layout(location = 0, index = 0) out vec4 fragColor;

float stepp;
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

//
//
// d02 d12 d22
// d01 d11 d21
// d00 d10 d20
//
//
vec3 estimateNormal(vec3 d11, vec3 d01, vec3 d21, vec3 d10, vec3 d12)
{
    vec3 xLess, xPlus, yLess, yPlus;
    xLess = d01;
    xPlus = d21;
    yLess = d10;
    yPlus = d12;
    // x
    if (dot(normalize(d01 - d11), normalize(d21 - d11)) > -0.5)
    {
//        return vec3(1.0, 0.0, 0.0);
        float delta01 = abs(d11.z - d01.z);
        float delta12 = abs(d11.z - d21.z);
        if (delta01 > delta12)
        {
            xLess = d11;
            xPlus = d21;
        } else
        {
            xLess = d01;
            xPlus = d11;
        }
    }
    // y
    if (dot(normalize(d10 - d11), normalize(d12 - d11)) > -0.5)
    {
//        return vec3(0.0, 1.0, 0.0);
        float delta01 = abs(d11.z - d10.z);
        float delta12 = abs(d11.z - d12.z);
        if (delta01 > delta12)
        {
            yLess = d11;
            yPlus = d12;
        } else
        {
            yLess = d10;
            yPlus = d11;
        }
    }
    return normalize(cross(xPlus - xLess, yPlus - yLess));
//    return vec3(0.0, 0.0, 0.0);
}


vec3[16] FullEstimateNormal(vec2 coord)
{
    vec2 xDelta = vec2(invVP.x, 0.0);
    vec2 yDelta = vec2(0.0, invVP.y);
    vec2 xPlus = coord + xDelta;
    vec2 xLess = coord - xDelta;
    vec2 yPlus = coord + yDelta;
    vec2 yLess = coord - yDelta;
    vec4 xDepthLess, xDepthPlus, yDepthLess, yDepthPlus;
    vec3 outNormal[16];

    float CutOff = 1000;

    for(int iCtr = 0; iCtr < 16 / 4; iCtr++)
    {
        switch(iCtr)
	{
        case 0:
	        xDepthLess = texture(dep0, xLess);
	        xDepthPlus = texture(dep0, xPlus);
	        yDepthLess = texture(dep0, yLess);
	        yDepthPlus = texture(dep0, yPlus);
                break;
        case 1:
	        xDepthLess = texture(dep1, xLess);
	        xDepthPlus = texture(dep1, xPlus);
	        yDepthLess = texture(dep1, yLess);
	        yDepthPlus = texture(dep1, yPlus);
                break;
        case 2:
	        xDepthLess = texture(dep2, xLess);
	        xDepthPlus = texture(dep2, xPlus);
	        yDepthLess = texture(dep2, yLess);
	        yDepthPlus = texture(dep2, yPlus);
                break;
        case 3:
	        xDepthLess = texture(dep3, xLess);
	        xDepthPlus = texture(dep3, xPlus);
	        yDepthLess = texture(dep3, yLess);
	        yDepthPlus = texture(dep3, yPlus);
                break;
        }

        vec4 xDiff = abs(xDepthPlus - xDepthLess);
        vec4 yDiff = abs(yDepthPlus - yDepthLess);
        if(xDiff.x + yDiff.x < CutOff)
        {
            outNormal[iCtr * 4 + 0] = estimateNormal(vec3(FragIn.texCoord, depthValue[0]), vec3(xLess, xDepthLess.x), vec3(xPlus, xDepthPlus.x), vec3(yLess, yDepthLess.x), vec3(yPlus, yDepthPlus.x));
        }
        else outNormal[iCtr * 4 + 0] = vec3(0);
        if(xDiff.y + yDiff.y < CutOff)
        {
            outNormal[iCtr * 4 + 1] = estimateNormal(vec3(FragIn.texCoord, depthValue[1]), vec3(xLess, xDepthLess.y), vec3(xPlus, xDepthPlus.y), vec3(yLess, yDepthLess.y), vec3(yPlus, yDepthPlus.y));
        }
        else outNormal[iCtr * 4 + 1] = vec3(0);
        if(xDiff.z + yDiff.z < CutOff)
        {
            outNormal[iCtr * 4 + 2] = estimateNormal(vec3(FragIn.texCoord, depthValue[2]), vec3(xLess, xDepthLess.z), vec3(xPlus, xDepthPlus.z), vec3(yLess, yDepthLess.z), vec3(yPlus, yDepthPlus.z));
        }
        else outNormal[iCtr * 4 + 2] = vec3(0);
        if(xDiff.w + yDiff.w < CutOff)
        {
            outNormal[iCtr * 4 + 3] = estimateNormal(vec3(FragIn.texCoord, depthValue[3]), vec3(xLess, xDepthLess.w), vec3(xPlus, xDepthPlus.w), vec3(yLess, yDepthLess.w), vec3(yPlus, yDepthPlus.w));
        }
        else outNormal[iCtr * 4 + 3] = vec3(0);
    }
    return outNormal;
}


vec3[16] BlurredEstimateNormal(vec2 coord)
{
    //Inverse distance sampling near coordinate:
    //Don't add samples from ridiculous distances (except for the sample right at this coord).

    vec3 SummedNormals[16];

    //Add the sample from this point:
    int MaxSamples = 20;
    int Wraps = 3;
    float MinRadius = 1 * invVP.x;
    float MaxRadius = 4 * invVP.x;
    SummedNormals = FullEstimateNormal(coord);

    //Spiral out, testing local normals.
    for(int iCtr = 0; iCtr < MaxSamples; iCtr++)
    {
        float frac =  float(iCtr) / float(MaxSamples - 1);
        float Radius = mix(MinRadius, MaxRadius, frac);
        float Angle = mix(0, Wraps * 3.14159 * 2, frac); 
        vec2 vec = Radius * vec2(cos(Angle), sin(Angle));
        vec2 newCoord = coord + vec;
        vec3 newNormals[16] = FullEstimateNormal(newCoord);
        for(int jCtr = 0; jCtr < 16; jCtr++)
            SummedNormals[jCtr] += newNormals[jCtr]; 
    }

    for(int iCtr = 0; iCtr < 16; iCtr++)
        SummedNormals[iCtr] = normalize(SummedNormals[iCtr]);

    return SummedNormals;
}


void main()
{
    invVP.x = 1.1 / viewport.x;
    invVP.y = 1.1 / viewport.x;
    nBins = 16;
    K = 1.0;
    stepp = 1.0 / nBins;
    // colors
    for (int i = 0; i < 16; ++i)
    {
        intensity[i] = stepp * (i + 0.5);
        vec4 temp = texture(tf, intensity[i]);
        colors[i] = temp.rgb;
        newA[i] = temp.a;
        oldA[i] = 1.0 / 2.0; //texture(rafa, intensity[i]).r;
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

    normalValue = BlurredEstimateNormal(FragIn.texCoord);

//    fragColor = vec4(vec3(depthValue[8]), 1.0);
//    return;

    //Uncomment to get normal representation...
//    fragColor = vec4(normalValue[8], 1.0);
//    return;

    //In the Phong lighting model, we have four constants: ambient, specular, and diffuse reflections, and shininess (hardcoded)

    float KA = 0.1; //Ambient reflection
    float KS = 0.5; //Specular reflection
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

//        if (normalValue[i].z > 0.0)
//            normalValue[i] = vec3(1.0) - normalValue[i];

        vec3 pos = vec3(FragIn.texCoord.xy, depthValue[i]);

        //Add ambient...
        binLightMultiplier[i] = AmbientIntensity * KA;

        //For each light, add to the light multiplier according to the Phong model.
        for(int iCtr = 0; iCtr < numLights; iCtr++)
        {
            vec3 LM = lightPositions[iCtr] - pos;
            vec3 RM = reflect(normalize(-LM), normalValue[i]);

            //First diffuse...
            binLightMultiplier[i] += clamp(KD * dot(RM, normalValue[i]) * lightDiffuseIntensities[iCtr], 0, 1);

            //Next specular...
            binLightMultiplier[i] += clamp(KS * vec3(pow(dot(RM, normalize(-pos)), 4)), 0, 1); //The magic number is shininess
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
        for (int j = i + 1; j < nBins; ++j)
            binValue[j] = clamp(pow(1.0 - newA[i], K) / pow(1.0 - oldA[i], K) * binValue[j], 0, 1);
    }

    // sum

    //Sort the bins by depth (of first encounter). TODO: This probably shouldn't be insertion sort :/
    for(int iCtr = 0; iCtr < 16; iCtr++)
        depthOrder[iCtr] = iCtr;
//    for(int iCtr = 0; iCtr < 16; iCtr++)
//    {
//        for(int jCtr = iCtr + 1; jCtr < 16; jCtr++)
//        {
//            if(depthValue[depthOrder[iCtr]] > depthValue[depthOrder[jCtr]])
//            {
//                int Temp = depthOrder[iCtr];
//                depthOrder[iCtr] = depthOrder[jCtr];
//                depthOrder[jCtr] = Temp;
//            }
//        }
//    }

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
