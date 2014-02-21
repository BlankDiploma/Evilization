float4x4 matWorld;
float4x4 matProj;
float4x4 matView;
//float4x4 matWorldInvTrans;

float4 eye;
Texture shaderTexture;

float3 LightDirection = float3(-0.5, -0.5, -0.5);
float4 DiffuseColor = float4(1, 1, 1, 1);
//float DiffuseIntensity = 1.0;

float4 AmbientColor= float4(1, 1, 1, 1);
float AmbientIntensity = 1.0f;

//float Shininess = 2000;
float4 SpecularColor = float4(1, 1, 1, 1);    
float SpecularIntensity = 5.0f;
//float4 vecView;



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
	//float4 Color : COLOR0;
	//float4 Specular : COLOR1;
	float2 Tex : TEXCOORD0;
	float3 Normal : NORMAL0;
};

struct VertexToPixel
{
    float4 Position : POSITION0;
	//float4 Color : COLOR0;
	float2 Tex : TEXCOORD0;
	float3 Normal : NORMAL0;
	float3 ViewDirection : TEXCOORD1;
	//float3 Normal : TEXCOORD0;
};

VertexToPixel VertexShaderFunction(ApplicationToVertex input)
{
    VertexToPixel output;

    float4 worldPosition = mul(input.Position, matWorld);
    float4 viewPosition = mul(worldPosition, matView);
    output.Position = mul(viewPosition, matProj);

	float3 normal = mul(input.Normal, (float3x3)matWorld);
	output.Normal = normalize(normal);

	output.Tex = input.Tex;



    //float lightIntensity = dot(normal, DiffuseLightDirection);
    //output.Color = saturate(DiffuseColor * DiffuseIntensity * lightIntensity);

	output.ViewDirection = normalize(eye.xyz - worldPosition.xyz);

    return output;
}

float4 PixelShaderFunction(VertexToPixel input) : COLOR0
{
	//float4 textureColor;
	//float3 lightDir;
	//float lightIntensity;
	//float4 color;
	//float3 reflection;
	//float4 specular;




	//float3 light = normalize(DiffuseLightDirection);
 //   float3 normal = normalize(input.Normal);
 //   float3 r = normalize(2 * dot(light, normal) * normal - light);
 //   float3 v = normalize(mul(normalize(vecView), matWorld));

 //   float dotProduct = dot(r, v);
 //   float4 specular = SpecularIntensity * SpecularColor * max(pow(abs(dotProduct), Shininess), 0) * length(input.Color);

 //   return saturate(input.Color + AmbientColor * AmbientIntensity + specular);

	float4 textureColor;
	float3 lightDir;
	float lightIntensity;
	float4 color;
	float3 reflection;
	float4 specular;


	// Sample the pixel color from the texture using the sampler at this texture coordinate location.
	textureColor = tex2D(Sampler, input.Tex);

	// Set the default output color to the ambient light value for all pixels.
    color = AmbientColor * AmbientIntensity;

	// Initialize the specular color.
	specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

	// Invert the light direction for calculations.
    lightDir = -LightDirection;

    // Calculate the amount of light on this pixel.
    lightIntensity = saturate(dot(input.Normal, lightDir));
	
	if(lightIntensity > 0.0f)
    {
        // Determine the final diffuse color based on the diffuse color and the amount of light intensity.
        color += (DiffuseColor * lightIntensity);

	    // Saturate the ambient and diffuse color.
	    color = saturate(color);

		// Calculate the reflection vector based on the light intensity, normal vector, and light direction.
        reflection = normalize(2 * lightIntensity * input.Normal - lightDir); 

		// Determine the amount of specular light based on the reflection vector, viewing direction, and specular power.
        specular = pow(saturate(dot(reflection, input.ViewDirection)), SpecularIntensity);
    }

    // Multiply the texture pixel and the input color to get the textured result.
    color = color * textureColor;

	// Add the specular component last to the output color.
    color = saturate(color + specular);

//	color = AmbientColor;
    return color;


}

technique Ambient
{
    pass Pass1
    {
        VertexShader = compile vs_3_0 VertexShaderFunction();
        PixelShader = compile ps_3_0 PixelShaderFunction();
    //    ShadeMode = FLAT; // flat color interpolation across triangles
    //    FillMode = SOLID; // no wireframes, no point drawing.
    //    CullMode = CCW; // cull any counter-clockwise polygons.
	//	ZEnable = TRUE;
    }
}