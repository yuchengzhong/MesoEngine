// Meso Engine 2024
#pragma once
#include "ShaderBase.h"


class ShaderFullScreenVS :public ShaderBase
{
public:
    lvk::ShaderStage GetShaderStage() const override
    {
        return lvk::Stage_Vert;
    }
    std::string GetShaderDebugInfo() const override
    {
        return std::string("Shader Module: fullscreen (vert)");
    }
    std::string GetShaderString() const override
    {
        return R"(
layout (location=0) out vec2 uv;
void main() 
{
    uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(uv * vec2(2, -2) + vec2(-1, 1), 0.0, 1.0);
}
)";
    }
};
class ShaderFullScreenFS :public ShaderBase
{
public:
    lvk::ShaderStage GetShaderStage() const override
    {
        return lvk::Stage_Frag;
    }
    std::string GetShaderDebugInfo() const override
    {
        return std::string("Shader Module: fullscreen (frag)");
    }
    std::string GetShaderString() const override
    {
        return R"(
layout (location=0) in vec2 uv;
layout (location=0) out vec4 out_FragColor;

layout(push_constant) uniform constants 
{
	uint tex;
} pc;

void main() 
{
    //out_FragColor = vec4(uv.x, uv.y, 0.0f, 0.0f);
    out_FragColor = textureBindless2D(pc.tex, 0, uv);
}
)";
    }
};