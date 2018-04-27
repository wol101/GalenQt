#version 330 core

in vec2 fragmentUV;
in vec4 fragmentColour;

uniform bool hasTexture;
uniform sampler2D textureSamplerRed; // note these are GL_RED single channel textures
uniform sampler2D textureSamplerGreen; // note these are GL_RED single channel textures
uniform sampler2D textureSamplerBlue; // note these are GL_RED single channel textures
uniform float redMin;
uniform float redMax;
uniform float redGamma;
uniform float redZebra;
uniform bool redLog;
uniform float greenMin;
uniform float greenMax;
uniform float greenGamma;
uniform float greenZebra;
uniform bool greenLog;
uniform float blueMin;
uniform float blueMax;
uniform float blueGamma;
uniform float blueZebra;
uniform bool blueLog;

out vec4 outputColour;

void main()
{
    if (hasTexture)
    {
        vec3 localColor;
        float v;

        if (redLog)
        {
            v = texture(textureSamplerRed, fragmentUV).r; // Set fragment color from texture (r contains the grey scale value)
            v = log(clamp(v, redMin, intBitsToFloat(2139095039))); // intBitsToFloat(2139095039) produces FLT_MAX which isn't defined
            v = clamp((v - log(redMin)) / (log(redMax) - log(redMin)), 0.0f, 0.99999f); // get the range to 0 to <1
            v = mod(v * redZebra, 1.0f); // this does the zebra
            v = pow(v, redGamma);
            localColor.r = v;
        }
        else
        {
            v = texture(textureSamplerRed, fragmentUV).r; // Set fragment color from texture (r contains the grey scale value)
            v = clamp((v - redMin) / (redMax - redMin), 0.0f, 0.99999f); // get the range to 0 to <1
            v = mod(v * redZebra, 1.0f); // this does the zebra
            v = pow(v, redGamma);
            localColor.r = v;
        }

        if (greenLog)
        {
            v = texture(textureSamplerGreen, fragmentUV).r; // Set fragment color from texture (r contains the grey scale value)
            v = log(clamp(v, greenMin, intBitsToFloat(2139095039))); // intBitsToFloat(2139095039) produces FLT_MAX which isn't defined
            v = clamp((v - log(greenMin)) / (log(greenMax) - log(greenMin)), 0.0f, 0.99999f); // get the range to 0 to <1
            v = mod(v * greenZebra, 1.0f); // this does the zebra
            v = pow(v, greenGamma);
            localColor.g = v;
        }
        else
        {
            v = texture(textureSamplerGreen, fragmentUV).r; // Set fragment color from texture (r contains the grey scale value)
            v = clamp((v - greenMin) / (greenMax - greenMin) , 0.0f, 0.99999f); // get the range to 0 to <1
            v = mod(v * greenZebra, 1.0f); // this does the zebra
            v = pow(v, greenGamma);
            localColor.g = v;
        }

        if (blueLog)
        {
            v = texture(textureSamplerBlue, fragmentUV).r; // Set fragment color from texture (r contains the grey scale value)
            v = log(clamp(v, blueMin, intBitsToFloat(2139095039))); // intBitsToFloat(2139095039) produces FLT_MAX which isn't defined
            v = clamp((v - log(blueMin)) / (log(blueMax) - log(blueMin)), 0.0f, 0.99999f); // get the range to 0 to <1
            v = mod(v * blueZebra, 1.0f); // this does the zebra
            v = pow(v, blueGamma);
            localColor.b = v;
        }
        else
        {
            v = texture(textureSamplerBlue, fragmentUV).r; // Set fragment color from texture (r contains the grey scale value)
            v = clamp((v - blueMin) / (blueMax - blueMin), 0.0f, 0.99999f); // get the range to 0 to <1
            v = mod(v * blueZebra, 1.0f); // this does the zebra
            v = pow(v, blueGamma);
            localColor.b = v;
        }

        outputColour = vec4(localColor, 1.0f);
    }
    else
    {
        outputColour = fragmentColour;
    }
}

