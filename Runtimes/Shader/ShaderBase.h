// Meso Engine 2024
#pragma once
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>
class ShaderBase 
{
public:
    lvk::Holder<lvk::ShaderModuleHandle> SMHandle;
    
    virtual ~ShaderBase() = default;

    virtual std::string GetShaderString() const 
    {
        return R"(Example Shader)";
    }

    void PrintShaderString() const
    {
        std::cout << GetShaderString() << std::endl;
    }

    std::string GetShaderMD5() const
    {
        boost::uuids::detail::md5 Hash;
        boost::uuids::detail::md5::digest_type Digest;

        Hash.process_bytes(GetShaderString().data(), GetShaderString().length());
        Hash.get_digest(Digest);

        const auto CharDigest = reinterpret_cast<const char*>(&Digest);
        std::string Result;
        boost::algorithm::hex(CharDigest, CharDigest + sizeof(boost::uuids::detail::md5::digest_type), std::back_inserter(Result));
        return Result;
    }
    virtual lvk::ShaderStage GetShaderStage() const
    {
        return lvk::Stage_Vert;
    }
    virtual std::string GetShaderDebugInfo() const
    {
        return std::string("Shader Module: fullscreen (vert)");
    }
    void UpdateShaderHandle(lvk::IContext* Context)
    {
        //TODO handle shader cache
        SMHandle = Context->createShaderModule({ GetShaderString().c_str(), GetShaderStage(), GetShaderDebugInfo().c_str() });
    }
    static std::string DefineConstant(uint32_t Value, std::string DefineName)
    {
        return "#define " + DefineName + " " + std::to_string(Value) + "\n";
    }
};