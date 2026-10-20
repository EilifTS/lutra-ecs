#pragma once
#include <filesystem>
#include <fstream>
#include <string>

/* A directory of its own per case, gone again afterwards. */
struct ScratchDir
{
	std::filesystem::path root;

	explicit ScratchDir(const std::string& name)
		: root(std::filesystem::temp_directory_path() / ("lutra-ecs-" + name))
	{
		std::filesystem::remove_all(root);
		std::filesystem::create_directories(root);
	}

	~ScratchDir() { std::filesystem::remove_all(root); }

	std::filesystem::path Write(const std::string& relative, const std::string& contents) const
	{
		const std::filesystem::path path = root / relative;
		std::filesystem::create_directories(path.parent_path());
		std::ofstream(path) << contents;
		return path;
	}
};
