// Dialogue/DialogueInterpreter.wren
// The dialogue-graph interpreter: executes node commands, evaluates conditions/targets, advances
// through nodes, and does {var} token substitution/formatting. Extracted from what used to be
// States/InteractState.wren's embedded DialogueManagerHelper class - pure logic, zero UI
// dependency, now alongside this project's other Dialogue/ modules.
import "debug" for Debug
import "dialogue" for DialogueRegistry, DialogueManager, DialogueResult, VariableSubstitution

class DialogueInterpreter {
    static executeCommands(commands) {
        for (command in commands) {
            var params = command.params
            command.callback.call(params)
        }
    }

    static evaluateConditions(conditions) {
        for (condition in conditions) {
            var params = condition.params
            if (!condition.callback.call(params)) {
                return false
            }
        }
        return true
    }

    static evaluateTargets(targets) {
        for (target in targets) {
            if (!target.hasConditions()) {
                return target.targetNode
            }

            if (target.conditions.count == 0) {
                return target.targetNode
            }

            if (evaluateConditions(target.conditions)) {
                return target.targetNode
            }
        }

        return null
    }

    static processNode(nodeId) {
        Debug.info("Processing dialogue at node: %(nodeId)")
        var node = DialogueManager.setActiveNode(nodeId)
        if (!node) {
            Debug.breakpointMsg("Node not found '%(nodeId)'")
            return DialogueResult.nodeNotFound(node)
        }
        var commands = node.commands
        if (commands) {
            executeCommands(commands)
        }
        if (node.hasTargets()) {
            var targetNodeId = evaluateTargets(node.targets)
            if (targetNodeId) {
                return processNode(targetNodeId)
            }
            Debug.warning("No target conditions matched in node '%(nodeId)', ending dialogue")
            DialogueManager.clearDialogue()
            return DialogueResult.endDialogue(node)
        }
        if (node.hasChoices()) {
            return DialogueResult.choices(node)
        }
        if (node.hasNext()) {
            return DialogueResult.advance(node)
        }
        return DialogueResult.endDialogue(node)
    }

    static startDialogue(startNodeId) {
        DialogueManager.clearDialogue()
        return processNode(startNodeId)
    }

    static endDialogue() {
        return DialogueManager.clearDialogue()
    }

    static processString(text) {
        return processString(text, null)
    }

    static processString(text, overrides) {
        if (text == null || text == "") {
            return text
        }
        var result = text
        var offset = 0

        while (offset < result.count) {
            var startIdx = result.indexOf("{", offset)
            if (startIdx == -1) break

            var endIdx = result.indexOf("}", startIdx)
            if (endIdx == -1) break

            var expression = result[startIdx + 1...endIdx]

            if (expression == "") {
                offset = endIdx + 1
                continue
            }

            var pipeParts = expression.split("|")
            var nameAndParams = pipeParts[0]
            var modifiers = pipeParts.count > 1 ? pipeParts[1..-1].join("|") : ""

            var colonIdx = nameAndParams.indexOf(":")
            var varName
            var params = {}
            if (colonIdx != -1) {
                varName = nameAndParams[0...colonIdx].trim()
                var paramStr = nameAndParams[colonIdx + 1..-1]
                var paramPairs = paramStr.split(",")
                for (pair in paramPairs) {
                    var eqIdx = pair.indexOf("=")
                    if (eqIdx != -1) {
                        var key = pair[0...eqIdx].trim()
                        var val = pair[eqIdx + 1..-1].trim()
                        params[key] = val
                    }
                }
            } else {
                varName = nameAndParams.trim()
            }

            var value
            if (overrides != null && overrides.containsKey(varName)) {
                value = overrides[varName]
            } else {
                value = DialogueRegistry.getRegisteredVariable(varName).call(params)
            }
            if (value == null) {
                value = varName
            }

            var formatted = VariableSubstitution.applyModifiers(value, modifiers)

            result = result[0...startIdx] + formatted + result[endIdx + 1..-1]
            offset = startIdx + formatted.count
        }

        return result
    }

    static continueDialogue() {
        var currentNode = DialogueManager.currentNode
        if (!currentNode) {
            Debug.warning("Continue called but no active dialogue node")
            return DialogueResult.noActiveNode()
        }
        if (!currentNode.hasNext()) {
            Debug.warning("Continue called but current node has no 'next'")
            return DialogueResult.noActiveNode()
        }
        var nextNodeId = currentNode.next
        Debug.info("Continuing to node: %(nextNodeId)")
        return processNode(nextNodeId)
    }

    static makeChoice(choiceIndex) {
        var currentNode = DialogueManager.currentNode
        if (!currentNode) {
            Debug.warning("makeChoice called but no active dialogue node")
            return DialogueResult.noActiveNode()
        }
        var choices = currentNode.choices
        if (!choices) {
            Debug.error("makeChoice called but no choices on node %(currentNode.id)")
            return DialogueResult.invalidChoice()
        }
        if (choiceIndex < 0 || choiceIndex >= choices.count) {
            Debug.error("Invalid choice index %(choiceIndex) (available: %(choices.count)) for node %(currentNode.id)")
            return DialogueResult.invalidChoice()
        }
        var targetNodeId = choices[choiceIndex].targetNodeId
        Debug.info("Player chose option %(choiceIndex), jumping to node: %(targetNodeId)")
        return processNode(targetNodeId)
    }
}
