#pragma once
#include <string>
#include <filesystem>
#include <cstdio>
#include <cstdlib>

struct FFileHelper
{
    static inline std::string PATH_CONTENT = "Contents/";
    static inline std::string PATH_FONT = "Fonts/";
    inline static std::string GetBasePath()
	{
        std::filesystem::path Subdir = PATH_CONTENT;
        std::filesystem::path Dir = std::filesystem::current_path();
        // find the content somewhere above our current build directory
        while (Dir != std::filesystem::current_path().root_path() && !exists(Dir / Subdir))
        {
            Dir = Dir.parent_path();
        }
        return Dir.string();
	}
    inline static std::string GetContentPath()
	{
        std::filesystem::path Subdir = PATH_CONTENT;
        std::filesystem::path Dir = GetBasePath();
        if (!exists(Dir / Subdir)) 
        {
            printf("Cannot find the Contents directory.\n");
        }
        return (Dir / Subdir).string();
	}
    inline static std::string GetFontPath()
	{
        std::filesystem::path Subdir = PATH_FONT;
        std::filesystem::path Dir = GetContentPath();
        if (!exists(Dir / Subdir))
        {
            printf("Cannot find the Fonts directory.\n");
        }
        return (Dir / Subdir).string();
	}
};