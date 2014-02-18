float4x4 matWorld;
float4x4 matProj;
float4x4 matView;

float4 eye;
Texture shaderTexture;

//float4 AmbientColor= float4(1, 1, 1, 1);
//float AmbientIntensity = 1.0f;

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
	//float3 ViewDirection : TEXCOORD1;
};

VertexToPixel VertexShaderFunction(ApplicationToVertex input)
{
    VertexToPixel output;

    float4 worldPosition = mul(input.Position, matWorld);
    float4 viewPosition = mul(worldPosition, matView);
    output.Position = mul(viewPosition, matProj);

	output.Color = input.Color;

	output.Tex = input.Tex;

	//output.ViewDirection = normalize(eye.xyz - worldPosition.xyz);

    return output;
}

VertexToPixel VertexShaderFunction2D(ApplicationToVertex input)
{
	VertexToPixel output;
	output.Position = input.Position;
	output.Color = input.Color;
	output.Tex = input.Tex;
	return output;
}

float4 PixelShaderFunction(VertexToPixel input) : COLOR0
{
	float4 textureColor, color;
	textureColor = tex2D(Sampler, input.Tex);

	if (textureColor.r || textureColor.b || textureColor.g)
		color = saturate(textureColor * input.Color);
	else
		color = saturate(input.Color * textureColor.a);
	return color;
}

float4 PixelShaderFunction2D(VertexToPixel input) : COLOR0
{
	float4 textureColor, color;
	textureColor = tex2D(Sampler, input.Tex);

	if (textureColor.r || textureColor.b || textureColor.g)
		color = saturate(textureColor);
	else
		color = saturate(input.Color * textureColor.a);
	//color = textureColor;
	return color;
}

technique Default3D
{
    pass Pass1
    {
        VertexShader = compile vs_3_0 VertexShaderFunction();
        PixelShader = compile ps_3_0 PixelShaderFunction();

        //ShadeMode = FLAT;
        FillMode = SOLID;
        CullMode = CCW;
		ZEnable = TRUE;
		ZWriteEnable = TRUE;

		MagFilter[0] = NONE;
		MinFilter[0] = NONE;
		MipFilter[0] = NONE;
		MaxAnisotropy[0] = 8;

		Sampler[0] = (Sampler);
		SrcBlend = SRCALPHA;
		DestBlend = INVSRCALPHA;
		BlendOp = ADD;

		AlphaBlendEnable = true;
		AlphaFunc = GREATEREQUAL;
		AlphaRef = 1;
    }
}

technique Default2D
{
    pass Pass1
    {
        VertexShader = compile vs_3_0 VertexShaderFunction2D();
        PixelShader = compile ps_3_0 PixelShaderFunction2D();

        //ShadeMode = FLAT;
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

technique Wireframe3D
{
	pass Pass1
	{
        VertexShader = compile vs_3_0 VertexShaderFunction();
        PixelShader = compile ps_3_0 PixelShaderFunction();
        FillMode = WIREFRAME;
        CullMode = CCW;
	}
}

technique Translucent3D
{
	pass Pass1
	{
        VertexShader = compile vs_3_0 VertexShaderFunction();
        PixelShader = compile ps_3_0 PixelShaderFunction();

        //ShadeMode = FLAT;
        FillMode = SOLID;
        CullMode = CCW;
		ZEnable = TRUE;
		ZWriteEnable = FALSE;

		MagFilter[0] = NONE;
		MinFilter[0] = NONE;
		MipFilter[0] = NONE;
		MaxAnisotropy[0] = 8;

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

	}
}