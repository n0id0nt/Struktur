#pragma once

#include <string>

namespace Struktur::Input
{
	class Input;

	/**
	 * InputConfigLoader - Handles loading input bindings from JSON files
	 * Uses nlohmann/json library for parsing
	 * Separated from Input class for better separation of concerns
	 */
	class InputConfigLoader
	{
	public:
		/**
		 * Load input bindings from a JSON file into an Input instance
		 * @param input The Input instance to configure
		 * @param filepath Path to the JSON configuration file
		 * @return True if successful, false otherwise
		 */
		static bool LoadFromFile(Input& input, const std::string& filepath);

	private:
		InputConfigLoader() = delete;  // Static class, no instances
	};
}
