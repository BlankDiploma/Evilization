float4x4 matWorld;
float4x4 matProj;
float4x4 matView;

float4 eye;
Texture shaderTexture;

float4 AmbientColor= float4(1, 1, 1, 1);
float AmbientIntensity = 1.0f;

sampler2D Sampler =
sampler_state
{
    Texture = <shaderTexture>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
};

float2 TexelSize : TEXELSIZE;

struct ApplicationToVertex
{
    float4 Position : POSITION0;
	float4 Color : COLOR0;
	float2 Tex : TEXCOORD0;
};

struct VertexToPixel
{
    float4 Position : POSITION0;
	float4 Color : COLOR0;
	float2 Tex : TEXCOORD0;
	float3 ViewDirection : TEXCOORD1;
};

VertexToPixel VertexShaderFunction(ApplicationToVertex input)
{
    VertexToPixel output;

    float4 worldPosition = mul(input.Position, matWorld);
    float4 viewPosition = mul(worldPosition, matView);
    output.Position = mul(viewPosition, matProj);

	output.Color = input.Color;

	output.Tex = input.Tex;

	output.ViewDirection = normalize(eye.xyz - worldPosition.xyz);

    return output;
}

float4 PixelShaderFunction(VertexToPixel input) : COLOR0
{

	float4 textureColor;
	float4 color;

	// Sample the pixel color from the texture using the sampler at this texture coordinate location.
	textureColor = tex2D(Sampler, input.Tex);
	
	if (TexelSize[0] != 0 && TexelSize[1] != 0)
		color = saturate(AmbientColor * AmbientIntensity * textureColor);
	else
		color = saturate(AmbientColor * AmbientIntensity * input.Color );

    return color;
}

technique Default3D
{
    pass Pass1
    {
        VertexShader = compile vs_3_0 VertexShaderFunction();
        PixelShader = compile ps_3_0 PixelShaderFunction();

        ShadeMode = FLAT;
        FillMode = SOLID;
        CullMode = CCW;
		ZEnable = TRUE;
		ZWriteEnable = TRUE;

		MagFilter[0] = NONE;
		MinFilter[0] = NONE;
		MipFilter[0] = NONE;
		MaxAnisotropy[0] = 8;
    }
}

technique Default2D
{
    pass Pass1
    {
        VertexShader = compile vs_3_0 VertexShaderFunction();
        PixelShader = compile ps_3_0 PixelShaderFunction();

        ShadeMode = FLAT;
        FillMode = SOLID;
        CullMode = NONE;
		ZEnable = FALSE;
		ZWriteEnable = FALSE;

		AlphaBlendEnable = true;
		AlphaFunc = GREATEREQUAL;
		AlphaRef = 1;

		Sampler[0] = (Sampler);
		AlphaOp[0] = MODULATE;
		AlphaArg1[0] = TEXTURE;
		AlphaArg2[0] = DIFFUSE;

		SrcBlend = SRCALPHA;
		DestBlend = INVSRCALPHA;
		BlendOp = ADD;

		MagFilter[0] = NONE;
		MinFilter[0] = NONE;
		MipFilter[0] = NONE;
		MaxAnisotropy[0] = 1;
    }
}
