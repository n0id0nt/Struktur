class OperatorFunctions {
    static evaluate(operator, lhs, rhs) {
        if (operator == "==") return lhs == rhs
        if (operator == "!=") return lhs != rhs
        if (operator == "<") return lhs < rhs
        if (operator == "<=") return lhs <= rhs
        if (operator == ">") return lhs > rhs
        if (operator == ">=") return lhs >= rhs
        
        // For 'is' operator (type checking)
        if (operator == "is") return lhs is rhs
        
        Fiber.abort("Unknown operator: %(operator)")
    }
}
