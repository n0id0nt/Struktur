#include "Conditions.h"
#include "Debug/Assertions.h"

namespace Struktur::Dialogue
{
	// CallbackCondition
	CallbackCondition::CallbackCondition(EvaluatorFunc evaluator, const std::string& description)
		: m_evaluator(std::move(evaluator))
		, m_description(description)
	{
		ASSERT_MSG(m_evaluator != nullptr, "CallbackCondition evaluator cannot be null");
	}

	bool CallbackCondition::Evaluate() const
	{
		return m_evaluator();
	}

	std::string CallbackCondition::GetDescription() const
	{
		return m_description;
	}

	// AndCondition
	AndCondition::AndCondition(std::vector<std::unique_ptr<Condition>> conditions)
		: m_conditions(std::move(conditions))
	{
		ASSERT_MSG(!m_conditions.empty(), "AndCondition requires at least one condition");
	}

	bool AndCondition::Evaluate() const
	{
		for (const auto& condition : m_conditions)
		{
			if (!condition->Evaluate())
				return false;
		}
		return true;
	}

	std::string AndCondition::GetDescription() const
	{
		std::string desc = "AND(";
		for (size_t i = 0; i < m_conditions.size(); ++i)
		{
			if (i > 0)
				desc += ", ";
			desc += m_conditions[i]->GetDescription();
		}
		desc += ")";
		return desc;
	}

	// OrCondition
	OrCondition::OrCondition(std::vector<std::unique_ptr<Condition>> conditions)
		: m_conditions(std::move(conditions))
	{
		ASSERT_MSG(!m_conditions.empty(), "OrCondition requires at least one condition");
	}

	bool OrCondition::Evaluate() const
	{
		for (const auto& condition : m_conditions)
		{
			if (condition->Evaluate())
				return true;
		}
		return false;
	}

	std::string OrCondition::GetDescription() const
	{
		std::string desc = "OR(";
		for (size_t i = 0; i < m_conditions.size(); ++i)
		{
			if (i > 0)
				desc += ", ";
			desc += m_conditions[i]->GetDescription();
		}
		desc += ")";
		return desc;
	}

	// NotCondition
	NotCondition::NotCondition(std::unique_ptr<Condition> condition)
		: m_condition(std::move(condition))
	{
		ASSERT_MSG(m_condition != nullptr, "NotCondition requires a valid condition");
	}

	bool NotCondition::Evaluate() const
	{
		return !m_condition->Evaluate();
	}

	std::string NotCondition::GetDescription() const
	{
		return "NOT(" + m_condition->GetDescription() + ")";
	}
}
