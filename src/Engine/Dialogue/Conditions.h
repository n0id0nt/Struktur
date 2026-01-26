#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>

namespace Struktur::Dialogue
{
	// Base condition interface
	class Condition
	{
	public:
		virtual ~Condition() = default;
		virtual bool Evaluate() const = 0;
		virtual std::string GetDescription() const = 0;
	};

	// Callback-based condition - evaluates via registered function
	class CallbackCondition : public Condition
	{
	public:
		using EvaluatorFunc = std::function<bool()>;

		CallbackCondition(EvaluatorFunc evaluator, const std::string& description);
		~CallbackCondition() override = default;

		bool Evaluate() const override;
		std::string GetDescription() const override;

	private:
		EvaluatorFunc m_evaluator;
		std::string m_description;
	};

	// Composite conditions
	class AndCondition : public Condition
	{
	public:
		AndCondition(std::vector<std::unique_ptr<Condition>> conditions);
		~AndCondition() override = default;

		bool Evaluate() const override;
		std::string GetDescription() const override;

	private:
		std::vector<std::unique_ptr<Condition>> m_conditions;
	};

	class OrCondition : public Condition
	{
	public:
		OrCondition(std::vector<std::unique_ptr<Condition>> conditions);
		~OrCondition() override = default;

		bool Evaluate() const override;
		std::string GetDescription() const override;

	private:
		std::vector<std::unique_ptr<Condition>> m_conditions;
	};

	class NotCondition : public Condition
	{
	public:
		NotCondition(std::unique_ptr<Condition> condition);
		~NotCondition() override = default;

		bool Evaluate() const override;
		std::string GetDescription() const override;

	private:
		std::unique_ptr<Condition> m_condition;
	};
}
