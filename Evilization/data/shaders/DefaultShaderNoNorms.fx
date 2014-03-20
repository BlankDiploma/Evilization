float4x4 matWorld;
float4x4 matProj;
float4x4 matView;

float4 eye;
Texture shaderTexture;

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
	float2 TexA : TEXCOORD0;
	float2 TexB : TEXCOORD1;
};

struct ApplicationToVertex2D
{
    float4 Position : POSITION0;
	float4 Color : COLOR0;
	float2 Tex : TEXCOORD0;
};

struct VertexToPixel
{
	float4 Position : POSITION0;
	float4 Color : COLOR0;
	float2 TexA : TEXCOORD0;
	float2 TexB : TEXCOORD1;
};

struct VertexToPixel2D
{
	float4 Position : POSITION0;
	float4 Color : COLOR0;
	float2 Tex : TEXCOORD0;
};

VertexToPixel VertexShaderFunction(ApplicationToVertex input)
{
    VertexToPixel output;

    float4 worldPosition = mul(input.Position, matWorld);
    float4 viewPosition = mul(worldPosition, matView);
    output.Position = mul(viewPosition, matProj);

	output.Color = input.Color;

	output.TexA = input.TexA;
	output.TexB = input.TexB;

    return output;
}

VertexToPixel2D VertexShaderFunction2D(ApplicationToVertex2D input)
{
	VertexToPixel2D output;
	output.Position = input.Position;
	output.Color = input.Color;
	output.Tex = input.Tex;
	return output;
}

float4 PixelShaderFunction(VertexToPixel input) : COLOR0
{
	float4 textureColor, color;
	textureColor = tex2D(Sampler, input.TexA);

	color = saturate(textureColor * input.Color);
	return color;
}

float4 PixelShaderFunction2D(VertexToPixel2D input) : COLOR0
{
	float4 textureColor, color;
	textureColor = tex2D(Sampler, input.Tex);

	color = saturate(textureColor * input.Color);
	return color;
}

float4 PixelShader2xBlendFunction(VertexToPixel input) : COLOR0
{
	float4 color;
	
	color = tex2D(Sampler, input.TexA);
	color = lerp(color,tex2D(Sampler, input.TexB),input.Color.r);
	color.a = input.Color.a;
	
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

        FillMode = SOLID;
        CullMode = CCW;
		ZEnable = TRUE;
		ZWriteEnable = FALSE;

		MagFilter[0] = LINEAR;
		MinFilter[0] = LINEAR;
		MipFilter[0] = LINEAR;
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

technique TranslucentBlend3D
{
	pass Pass1
	{
        VertexShader = compile vs_3_0 VertexShaderFunction();
        PixelShader = compile ps_3_0 PixelShader2xBlendFunction();

        FillMode = SOLID;
        CullMode = CCW;
		ZEnable = TRUE;
		ZWriteEnable = FALSE;

		MagFilter[0] = LINEAR;
		MinFilter[0] = LINEAR;
		MipFilter[0] = LINEAR;
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

technique TextureBlend3D
{
	pass Pass1
	{
        VertexShader = compile vs_3_0 VertexShaderFunction();
        PixelShader = compile ps_3_0 PixelShader2xBlendFunction();

        FillMode = SOLID;
        CullMode = CCW;
		ZEnable = TRUE;
		ZWriteEnable = TRUE;

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