#pragma once

#include <unordered_map>
#include <string>
#include "raylib.h"

namespace Struktur::Input
{
	/**
	 * InputMaps - Static mappings between string identifiers and Raylib input codes
	 * Separated from the Input class for better organization and maintainability
	 */
	class InputMaps
	{
	public:
		// Get the singleton instance
		static InputMaps& Instance();

		// String to code conversion
		::KeyboardKey GetKeycodeFromString(const std::string& input) const;
		::GamepadButton GetControllerButtonFromString(const std::string& input) const;
		::GamepadAxis GetControllerAxisFromString(const std::string& input) const;

		// Code to string conversion (useful for debugging/UI)
		std::string GetStringFromKeycode(::KeyboardKey key) const;
		std::string GetStringFromControllerButton(::GamepadButton button) const;
		std::string GetStringFromControllerAxis(::GamepadAxis axis) const;

	private:
		InputMaps();
		~InputMaps() = default;

		// Delete copy/move constructors for singleton
		InputMaps(const InputMaps&) = delete;
		InputMaps& operator=(const InputMaps&) = delete;
		InputMaps(InputMaps&&) = delete;
		InputMaps& operator=(InputMaps&&) = delete;

		void InitializeKeycodeMap();
		void InitializeControllerButtonMap();
		void InitializeControllerAxisMap();

		std::unordered_map<std::string, ::KeyboardKey> m_keycodeMap;
		std::unordered_map<std::string, ::GamepadButton> m_controllerButtonMap;
		std::unordered_map<std::string, ::GamepadAxis> m_controllerAxisMap;

		// Reverse lookup maps (for debugging)
		std::unordered_map<::KeyboardKey, std::string> m_keycodeReverseMap;
		std::unordered_map<::GamepadButton, std::string> m_controllerButtonReverseMap;
		std::unordered_map<::GamepadAxis, std::string> m_controllerAxisReverseMap;
	};
}
