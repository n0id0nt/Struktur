#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>

namespace Struktur::Dialogue
{
	// Base command interface
	class Command
	{
	public:
		virtual ~Command() = default;
		virtual void Execute() = 0;
		virtual std::string GetDescription() const = 0;
	};

	// Callback-based command - executes via registered function
	class CallbackCommand : public Command
	{
	public:
		using ExecutorFunc = std::function<void()>;

		CallbackCommand(ExecutorFunc executor, const std::string& description);
		~CallbackCommand() override = default;

		void Execute() override;
		std::string GetDescription() const override;

	private:
		ExecutorFunc m_executor;
		std::string m_description;
	};

	// Composite command for executing multiple commands
	class SequenceCommand : public Command
	{
	public:
		SequenceCommand(std::vector<std::unique_ptr<Command>> commands);
		~SequenceCommand() override = default;

		void Execute() override;
		std::string GetDescription() const override;

	private:
		std::vector<std::unique_ptr<Command>> m_commands;
	};
}
