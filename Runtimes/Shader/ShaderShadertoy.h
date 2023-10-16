#pragma once
#include "ShaderBase.h"

class ShaderShadertoyFS :public ShaderBase
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

void main() 
{
    out_FragColor = vec4(uv.x, uv.y, 0.0f, 0.0f);
    //out_FragColor = textureBindless2D(pc.tex, 0, uv);
}
)";
    }
};