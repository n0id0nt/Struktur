# Struktur Engine - C++ Coding Standards

## Table of Contents
1. [Overview](#overview)
2. [Naming Conventions](#naming-conventions)
3. [Code Organization](#code-organization)
4. [Memory Management](#memory-management)
5. [Parameter Passing Philosophy](#parameter-passing-philosophy)
6. [Error Handling & Debugging](#error-handling--debugging)
7. [Architecture Patterns](#architecture-patterns)
8. [Wren Integration](#wren-integration)
9. [Comments & Documentation](#comments--documentation)

---

## Overview

This document defines the coding standards for the Struktur game engine, a C++ engine using Raylib, EnTT, Wren scripting, and ImGui. These standards ensure consistency, maintainability, and seamless integration between C++ and Wren.

**Key Technologies:**
- **Graphics**: Raylib
- **ECS**: EnTT
- **Scripting**: Wren
- **UI**: ImGui (editor), Custom UI system (in-game)
- **Math**: GLM
- **Physics**: Box2D

---

## Naming Conventions

### Namespaces
```cpp
// Use PascalCase for namespaces
namespace Struktur
{
    namespace Dialogue
    {
        // Nested namespaces follow same convention
    }
}

// Do NOT use namespaces in Wren binding files
// ❌ Bad: namespace Struktur::Dialogue in WrenDialogue.cpp
// ✅ Good: No namespace in WrenDialogue.cpp
```

### Classes and Structs
```cpp
// PascalCase for classes and structs
class DialogueManager { };
struct DialogueResult { };
struct WrenDialogueData { };

// Foreign class wrappers use \"Wren\" prefix
struct WrenUIElement { };
struct WrenDialogueData { };
```

### Member Variables
```cpp
class Example
{
private:
    // Use m_ prefix for member variables
    int m_health;
    std::string m_name;
    DialogueNode* m_currentNode;
    std::vector<std::unique_ptr<Command>> m_commands;
    
    // Constants use all caps with underscores
    static constexpr float TEXT_SCROLL_SPEED = 0.02f;
};
```

### Functions and Methods
```cpp
// PascalCase for public methods
void StartDialogue(const std::string& nodeId);
DialogueResult MakeChoice(int choiceIndex);
bool IsDialogueActive() const;

// Getters use \"Get\" prefix
const std::string& GetCurrentNodeId() const;
DialogueRegistry& GetRegistry();

// Setters use \"Set\" prefix
void SetFlag(const std::string& name, bool value);
void SetVisible(bool visible);

// Boolean queries use \"Is\" or \"Has\" prefix
bool IsFocusable() const;
bool HasItem(const std::string& itemName) const;
```

### Wren Binding Functions
```cpp
// Format: wren_ClassNameMethodName
void wren_UIElementSetVisible(WrenVM* vm);
void wren_DialogueManagerStartDialogue(WrenVM* vm);
void wren_FlagManagerGetFlag(WrenVM* vm);

// Allocators and finalizers
void wren_DialogueDataAllocate(WrenVM* vm);
void wren_DialogueDataFinalize(void* data);
```

### Constants and Enums
```cpp
// Enum classes use PascalCase
enum class Status
{
    SUCCESS,           // Enum values use UPPER_CASE
    NO_ACTIVE_NODE,
    INVALID_CHOICE
};

// Global constants use UPPER_CASE
constexpr float TEXT_SCROLL_SPEED = 0.02f;
constexpr int MAX_PLAYERS = 4;
```

### Files
```cpp
// Header files: PascalCase.h
DialogueManager.h
FlagManager.h
UIElement.h

// Implementation files: PascalCase.cpp
DialogueManager.cpp
FlagManager.cpp

// Wren bindings: WrenClassName.h/cpp
WrenDialogue.h
WrenDialogue.cpp
WrenFlagManager.h
WrenFlagManager.cpp
```

---

## Code Organization

### Header File Structure
```cpp
#pragma once

// System includes
#include <string>
#include <vector>
#include <memory>

// Third-party includes
#include \"raylib.h\"
#include \"glm/glm.hpp\"
#include \"entt/entt.hpp\"

// Project includes
#include \"Debug/Assertions.h\"
#include \"Dialogue/Conditions.h\"

namespace Struktur::Dialogue
{
    // Forward declarations
    class DialogueNode;
    class Condition;
    
    // Class declaration
    class DialogueManager
    {
    public:
        // Constructors/Destructor
        DialogueManager();
        ~DialogueManager();
        
        // Delete copy, allow move (if needed)
        DialogueManager(const DialogueManager&) = delete;
        DialogueManager& operator=(const DialogueManager&) = delete;
        DialogueManager(DialogueManager&&) noexcept = default;
        DialogueManager& operator=(DialogueManager&&) noexcept = default;
        
        // Public interface - pass context as parameter
        void LoadDialogueFromMap(GameContext& context, const DialogueDataMap& data);
        DialogueResult StartDialogue(GameContext& context, const std::string& nodeId);
        DialogueResult MakeChoice(GameContext& context, int choiceIndex);
        
        // Getters
        bool IsDialogueActive() const;
        const DialogueNode* GetNode(const std::string& nodeId) const;
        
    private:
        // Private members - NO context reference stored
        std::unordered_map<std::string, std::unique_ptr<DialogueNode>> m_nodes;
        DialogueNode* m_currentNode;
        std::vector<std::string> m_history;
        
        // Private methods - pass context as parameter
        DialogueResult ProcessCurrentNode(GameContext& context);
        bool EvaluateConditions(GameContext& context, 
                               const std::vector<std::unique_ptr<Condition>>& conditions);
    };
}
```

### Implementation File Structure
```cpp
#include \"DialogueManager.h\"

// Additional includes
#include \"GameContext.h\"
#include \"Debug/Assertions.h\"

namespace Struktur::Dialogue
{
    DialogueManager::DialogueManager()
        : m_nodes()
        , m_currentNode(nullptr)
        , m_history()
    {
        DEBUG_INFO(\"DialogueManager initialized\");
    }
    
    DialogueManager::~DialogueManager()
    {
        DEBUG_INFO(\"DialogueManager destroyed\");
    }
    
    DialogueResult DialogueManager::StartDialogue(GameContext& context, const std::string& nodeId)
    {
        DEBUG_INFO(\"Starting dialogue at node: %s\", nodeId.c_str());
        
        auto it = m_nodes.find(nodeId);
        if (it == m_nodes.end())
        {
            DEBUG_ERROR(\"Starting node '%s' not found\", nodeId.c_str());
            return DialogueResult{/*...*/};
        }
        
        m_currentNode = it->second.get();
        return ProcessCurrentNode(context);  // Pass context through
    }
    
    // Implementation...
}
```

### Wren Binding File Structure
```cpp
#include \"WrenDialogue.h\"

#include \"Engine/Scripting/WrenBindingRegistry.h\"
#include \"Engine/GameContext.h\"
#include \"Dialogue/DialogueManager.h\"

// ============================================================================
// DIALOGUE MANAGER BINDINGS
// ============================================================================

void wren_DialogueManagerStartDialogue(WrenVM* vm)
{
    // Get context from Wren VM user data
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    Struktur::Dialogue::DialogueManager& dialogueManager = context->GetDialogueManager();
    
    const char* nodeId = wrenGetSlotString(vm, 1);
    
    // Pass context as parameter (not stored in manager)
    auto result = dialogueManager.StartDialogue(*context, nodeId);
    
    // Return result...
}

// Register static methods
WREN_CLASS_STATIC(\"dialogue\", \"DialogueManager\", \"startDialogue(_)\", 
                  wren_DialogueManagerStartDialogue, \"Start dialogue\");
```

---

## Memory Management

### Pointer Usage
```cpp
// Prefer unique_ptr for ownership
std::unique_ptr<DialogueNode> node = std::make_unique<DialogueNode>(\"node_id\");
std::vector<std::unique_ptr<Command>> commands;

// Use raw pointers for non-owning references
DialogueNode* m_currentNode;  // Points to node in m_nodes map
UIElement* element;           // Points to element owned by UI system

// NEVER use shared_ptr unless absolutely necessary
// ❌ std::shared_ptr<DialogueNode> - avoid this
```

### Foreign Class Ownership
```cpp
// Wren foreign class wrapper pattern
struct WrenDialogueData
{
    Struktur::Dialogue::DialogueDataMap* dataMap;
    bool loadedIntoManager;  // Ownership tracking flag
    
    WrenDialogueData()
        : dataMap(new Struktur::Dialogue::DialogueDataMap())
        , loadedIntoManager(false)
    {
    }
    
    ~WrenDialogueData()
    {
        if (!loadedIntoManager)
        {
            DEBUG_WARNING(\"WrenDialogueData was not loaded - memory may be wasted\");
        }
        delete dataMap;
    }
};

// Transfer ownership pattern
void TransferOwnership()
{
    uiManager.AddElement(std::unique_ptr<UIElement>(uiElement->element));
    uiElement->ownedByWren = false;  // Mark as transferred
}
```

### RAII and Resource Management
```cpp
// Use RAII for resource management
class ResourceManager
{
public:
    ~ResourceManager()
    {
        // Automatic cleanup via RAII
        // unique_ptr members clean themselves up
    }
    
private:
    std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;
    std::vector<std::unique_ptr<UIElement>> m_elements;
};
```

---

## Parameter Passing Philosophy

### **CRITICAL PRINCIPLE: Pass Values as Parameters, Not Stored References**

**Philosophy**: Prefer passing values (especially context objects) as function parameters rather than storing references as member variables. This approach:
- Makes dependencies explicit at call sites
- Improves testability (no hidden state dependencies)
- Reduces coupling between classes
- Makes lifetime management clearer
- Avoids dangling reference issues

### When to Store References vs. Pass as Parameters

#### ✅ **PREFER: Pass as Parameters**
Use this pattern by default for most dependencies:

```cpp
// ✅ GOOD - Pass context as parameter
class DialogueManager
{
public:
    DialogueManager();  // No context needed for construction
    
    // Context passed when needed
    void LoadDialogueFromMap(GameContext& context, const DialogueDataMap& data);
    DialogueResult StartDialogue(GameContext& context, const std::string& nodeId);
    DialogueResult MakeChoice(GameContext& context, int choiceIndex);
    
private:
    // No context reference stored!
    std::unordered_map<std::string, std::unique_ptr<DialogueNode>> m_nodes;
    DialogueNode* m_currentNode;
    
    // Internal methods also receive context
    DialogueResult ProcessCurrentNode(GameContext& context);
    void ExecuteCommands(GameContext& context, const std::vector<std::unique_ptr<Command>>& commands);
};

// Implementation
DialogueResult DialogueManager::StartDialogue(GameContext& context, const std::string& nodeId)
{
    // Use context directly
    context.GetFlagManager().SetFlag(\"dialogue_started\", true);
    
    // Pass context to internal methods
    return ProcessCurrentNode(context);
}

DialogueResult DialogueManager::ProcessCurrentNode(GameContext& context)
{
    // Pass context to command execution
    ExecuteCommands(context, m_currentNode->GetCommands());
    return result;
}
```

#### ⚠️ **USE SPARINGLY: Stored References**
Only store references when passing as parameters becomes impractical:

```cpp
// ⚠️ ACCEPTABLE - but consider alternatives first
class PhysicsSystem
{
public:
    // Only store reference if:
    // 1. Every method needs it
    // 2. System is tightly coupled by design
    // 3. Context lifetime exceeds system lifetime (guaranteed)
    explicit PhysicsSystem(GameContext& context)
        : m_gameContext(context)
    {
    }
    
    // Methods use stored reference
    void Update(float deltaTime);
    void FixedUpdate();
    void Render();
    
private:
    GameContext& m_gameContext;  // Stored because EVERY method needs it
};
```

### Decision Tree: Store Reference or Pass Parameter?

Ask yourself these questions in order:

1. **Does every public method need this dependency?**
   - NO → Pass as parameter
   - YES → Continue to next question

2. **Is the class tightly coupled to this dependency by design?**
   - NO → Pass as parameter
   - YES → Continue to next question

3. **Is the dependency's lifetime guaranteed to exceed this object's lifetime?**
   - NO → Pass as parameter (storing would be unsafe)
   - YES → Continue to next question

4. **Would passing it to every method make the API awkward?**
   - NO → Pass as parameter (better design)
   - YES → Consider storing reference

**If you reach question 4 and answered YES, you may store the reference, but document why.**

### Examples of Good vs. Bad Design

#### ❌ **BAD: Storing Reference Unnecessarily**
```cpp
// BAD - Only one method uses context, but we store it
class ItemPickup
{
public:
    explicit ItemPickup(GameContext& context, const std::string& itemName)
        : m_gameContext(context)  // ❌ Unnecessary storage
        , m_itemName(itemName)
    {
    }
    
    void OnPickup()
    {
        m_gameContext.GetInventory().AddItem(m_itemName);
    }
    
    const std::string& GetItemName() const { return m_itemName; }
    void SetItemName(const std::string& name) { m_itemName = name; }
    
private:
    GameContext& m_gameContext;  // ❌ Only used in one method!
    std::string m_itemName;
};
```

#### ✅ **GOOD: Pass as Parameter**
```cpp
// GOOD - Pass context when needed
class ItemPickup
{
public:
    explicit ItemPickup(const std::string& itemName)
        : m_itemName(itemName)
    {
    }
    
    void OnPickup(GameContext& context)  // ✅ Context passed when needed
    {
        context.GetInventory().AddItem(m_itemName);
    }
    
    const std::string& GetItemName() const { return m_itemName; }
    void SetItemName(const std::string& name) { m_itemName = name; }
    
private:
    // No context reference stored!
    std::string m_itemName;
};
```

#### ❌ **BAD: Destructor Dependency**
```cpp
// BAD - Storing reference just for destructor
class ResourceHandle
{
public:
    ResourceHandle(ResourceManager& manager, const std::string& name)
        : m_manager(manager)  // ❌ Only for destructor
        , m_name(name)
    {
        m_manager.Load(m_name);
    }
    
    ~ResourceHandle()
    {
        m_manager.Unload(m_name);  // ❌ Using stored reference
    }
    
private:
    ResourceManager& m_manager;
    std::string m_name;
};
```

#### ✅ **GOOD: Explicit Dispose Method**
```cpp
// GOOD - Use explicit Dispose instead of destructor
class ResourceHandle
{
public:
    explicit ResourceHandle(const std::string& name)
        : m_name(name)
        , m_disposed(false)
    {
    }
    
    // Explicit initialization and cleanup
    void Initialize(ResourceManager& manager)
    {
        manager.Load(m_name);
    }
    
    void Dispose(ResourceManager& manager)
    {
        if (!m_disposed)
        {
            manager.Unload(m_name);
            m_disposed = true;
        }
    }
    
    ~ResourceHandle()
    {
        ASSERT_MSG(m_disposed, \"ResourceHandle not disposed before destruction!\");
    }
    
private:
    std::string m_name;
    bool m_disposed;
};

// Usage
ResourceHandle handle(\"texture.png\");
handle.Initialize(context.GetResourceManager());
// ... use handle ...
handle.Dispose(context.GetResourceManager());
```

### Destructor Considerations

**Rule**: If you need a dependency in the destructor, consider these alternatives first:

1. **Use explicit Dispose/Cleanup method** (preferred)
   ```cpp
   void Dispose(GameContext& context);
   ~MyClass() { ASSERT_MSG(m_disposed, \"Must call Dispose before destruction\"); }
   ```

2. **Make the operation optional in destructor**
   ```cpp
   ~MyClass()
   {
       if (m_needsCleanup)
       {
           DEBUG_WARNING(\"MyClass destroyed without cleanup - this may leak resources\");
       }
   }
   ```

3. **Use a callback for cleanup**
   ```cpp
   using CleanupFunc = std::function<void()>;
   
   void SetCleanupCallback(CleanupFunc callback)
   {
       m_cleanupCallback = std::move(callback);
   }
   
   ~MyClass()
   {
       if (m_cleanupCallback)
           m_cleanupCallback();
   }
   ```

4. **Only if all else fails**, store the reference with clear documentation:
   ```cpp
   // Reference stored ONLY for destructor cleanup
   // ENSURE context outlives this object
   GameContext& m_gameContext;
   ```

### Benefits of Parameter Passing

```cpp
// ✅ BENEFITS DEMONSTRATED

// 1. Explicit dependencies - clear what each method needs
void ProcessDialogue(GameContext& context);  // Needs context
void GetNodeCount() const;                   // Doesn't need context

// 2. Easy to test with mock/stub contexts
TEST(DialogueManager, ProcessesNodeCorrectly)
{
    MockGameContext mockContext;
    DialogueManager manager;
    
    // Clear what's being passed
    auto result = manager.StartDialogue(mockContext, \"test_node\");
    
    ASSERT_EQ(result.status, DialogueResult::Status::SUCCESS);
}

// 3. No lifetime issues - context passed when available
void SomeFunction()
{
    GameContext context;
    DialogueManager manager;  // Safe construction without context
    
    // Use manager when context is available
    manager.StartDialogue(context, \"node\");
    
    // Context and manager can be destroyed in any order
}

// 4. Can work with different contexts
void ProcessWithDifferentContexts()
{
    GameContext contextA;
    GameContext contextB;
    DialogueManager manager;
    
    manager.StartDialogue(contextA, \"nodeA\");
    manager.StartDialogue(contextB, \"nodeB\");  // Different context!
}
```

### Wren Binding Pattern

Since Wren bindings need GameContext, always fetch it from VM user data:

```cpp
void wren_DialogueManagerStartDialogue(WrenVM* vm)
{
    // Get context from VM (not stored in manager)
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    Struktur::Dialogue::DialogueManager& dialogueManager = context->GetDialogueManager();
    
    const char* nodeId = wrenGetSlotString(vm, 1);
    
    // Pass context as parameter
    auto result = dialogueManager.StartDialogue(*context, nodeId);
    
    // Return result to Wren...
}
```

### Summary

**Default Rule**: Pass GameContext (and other dependencies) as parameters, not as stored references.

**Only store references when:**
1. Every method needs the dependency AND
2. The class is tightly coupled by design AND
3. Lifetime guarantees are clear AND
4. Documented with justification

**For destructor needs:**
1. First, try explicit Dispose methods
2. Second, try cleanup callbacks
3. Last resort: store reference with clear documentation

This approach makes code more modular, testable, and maintainable.

---

## Error Handling & Debugging

### Debug Macros
```cpp
// Use custom debug macros (defined in Debug/Assertions.h)

// Logging
DEBUG_INFO(\"DialogueManager initialized\");
DEBUG_WARNING(\"Node '%s' not found\", nodeId.c_str());
DEBUG_ERROR(\"Invalid choice index %d\", choiceIndex);
DEBUG_FATAL(\"Critical error - cannot continue\");

// Assertions
ASSERT(condition);
ASSERT_MSG(condition, \"Description of requirement\");
ASSERT_MSG(m_currentNode, \"ProcessCurrentNode called with null current node\");
ASSERT_MSG(params.find(\"flag\") != params.end(), \"flag parameter required\");

// Break (for debugging)
BREAK;
BREAK_MSG(\"Critical error at position (%d, %d)\", x, y);

// Formatted logging
DEBUG_INFO(\"Loaded dialogue with %zu nodes from Wren\", dataMap->nodes.size());
DEBUG_ERROR(\"Target node '%s' not found\", targetNodeId.c_str());
```

### Error Checking Patterns
```cpp
// Check conditions early and return/handle
DialogueResult StartDialogue(GameContext& context, const std::string& nodeId)
{
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end())
    {
        DEBUG_ERROR(\"Starting node '%s' not found\", nodeId.c_str());
        return DialogueResult{
            DialogueResult::Status::NODE_NOT_FOUND,
            nodeId, \"\", \"\", {}, true
        };
    }
    
    // Continue with valid state...
}

// Validate parameters in Wren bindings
void wren_DialogueDataAddNode(WrenVM* vm)
{
    if (wrenGetSlotType(vm, 2) != WREN_TYPE_MAP)
    {
        DEBUG_ERROR(\"DialogueData.addNode: nodeData must be a map\");
        return;
    }
    
    // Continue with validation passed...
}

// Check null pointers before use
void SetVisible(bool visible)
{
    if (!element)
    {
        DEBUG_ERROR(\"UIElement.setVisible: element is Null\");
        return;
    }
    element->SetVisible(visible);
}
```

---

## Architecture Patterns

### Registry Pattern (for Extensibility)
```cpp
// Use registry for extensible systems
class DialogueRegistry
{
public:
    using ConditionFactory = std::function<std::unique_ptr<Condition>(
        const std::unordered_map<std::string, std::string>& params
    )>;
    
    void RegisterConditionType(const std::string& typeName, ConditionFactory factory);
    std::unique_ptr<Condition> CreateCondition(
        const std::string& typeName,
        const std::unordered_map<std::string, std::string>& params
    ) const;
    
private:
    std::unordered_map<std::string, ConditionFactory> m_conditionFactories;
};

// Usage - context passed to factory when needed
registry.RegisterConditionType(\"hasItem\", [](const auto& params) {
    std::string itemName = params.at(\"item\");
    
    // Return condition that receives context at evaluation time
    return std::make_unique<CallbackCondition>(
        [itemName](GameContext& context) {  // Context passed here!
            return context.GetInventory().HasItem(itemName);
        },
        \"Has item '\" + itemName + \"'\"
    );
});
```

### Callback Pattern
```cpp
// Use std::function for callbacks
// Context passed when callback is invoked
class CallbackCondition : public Condition
{
public:
    using EvaluatorFunc = std::function<bool(GameContext&)>;
    
    CallbackCondition(EvaluatorFunc evaluator, const std::string& description)
        : m_evaluator(std::move(evaluator))
        , m_description(description)
    {
    }
    
    bool Evaluate(GameContext& context) const override 
    { 
        return m_evaluator(context);  // Pass context to callback
    }
    
private:
    EvaluatorFunc m_evaluator;
    std::string m_description;
};
```

### Data-Driven Design
```cpp
// Separate data structures from runtime structures
struct NodeData  // Input from Wren/JSON
{
    std::string speaker;
    std::string text;
    std::vector<ConditionData> conditions;
    std::vector<CommandData> commands;
};

class DialogueNode  // Runtime optimized
{
public:
    const std::string& GetSpeaker() const { return m_speaker; }
    
    // Conditions evaluated with context passed as parameter
    bool EvaluateConditions(GameContext& context) const;
    
private:
    std::string m_speaker;
    std::vector<std::unique_ptr<Condition>> m_conditions;
};
```

---

## Wren Integration

### Binding Registration Macros
```cpp
// Use provided macros for consistency

// Enum binding
WREN_ENUM(\"ui\", NavigationDirection, \"Navigation direction enum\",
    WREN_ENUM_PAIR(\"UP\", Struktur::UI::NavigationDirection::UP),
    WREN_ENUM_PAIR(\"DOWN\", Struktur::UI::NavigationDirection::DOWN)
);

// Foreign class binding
WREN_FOREIGN_CLASS(\"dialogue\", \"DialogueData\", 
    wren_DialogueDataAllocate, 
    wren_DialogueDataFinalize, 
    \"Container for dialogue data\");

// Static method binding
WREN_CLASS_STATIC(\"dialogue\", \"DialogueManager\", 
    \"startDialogue(_)\", 
    wren_DialogueManagerStartDialogue, 
    \"Start dialogue at a specific node\");

// Instance method binding
WREN_CLASS_METHOD(\"ui\", \"UIElement\", 
    \"setVisible(_)\", 
    wren_UIElementSetVisible, 
    \"Sets UI Element visibility\");

// Constructor binding
WREN_CONSTRUCTOR(\"ui\", \"UILabel\", 
    \"new(_,_,_)\", 
    wren_UILabelNew, 
    \"Create UI Label\");

// Class inheritance
WREN_CLASS_INHERITANCE(\"ui\", \"UILabel\", \"UIElement\");
```

### Wren Binding Patterns
```cpp
// Getting GameContext from Wren
void wren_SomeFunction(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    Struktur::SomeManager& manager = context->GetSomeManager();
    
    // Pass context to manager methods
    manager.DoSomething(*context, parameters);
}

// Getting parameters from Wren
void wren_Example(WrenVM* vm)
{
    // Strings
    const char* text = wrenGetSlotString(vm, 1);
    
    // Numbers (Wren uses doubles)
    int intValue = static_cast<int>(wrenGetSlotDouble(vm, 1));
    float floatValue = static_cast<float>(wrenGetSlotDouble(vm, 1));
    
    // Booleans
    bool boolValue = wrenGetSlotBool(vm, 1);
    
    // Foreign objects
    WrenUIElement* element = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 1));
    WrenVec2* vector = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
}

// Returning values to Wren
void wren_Example(WrenVM* vm)
{
    // Strings
    wrenSetSlotString(vm, 0, \"Hello\");
    
    // Numbers
    wrenSetSlotDouble(vm, 0, 42.0);
    
    // Booleans
    wrenSetSlotBool(vm, 0, true);
    
    // Create foreign object
    wrenGetVariable(vm, \"math\", \"Vec2\", 1);  // Get class into slot 1
    WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
    new (vec2) WrenVec2(glm::vec2{10.0f, 20.0f});
    
    // Create map
    wrenSetSlotNewMap(vm, 0);
    wrenSetSlotString(vm, 1, \"key\");
    wrenSetSlotString(vm, 2, \"value\");
    wrenSetMapValue(vm, 0, 1, 2);
    
    // Create list
    wrenSetSlotNewList(vm, 0);
    wrenSetSlotString(vm, 1, \"item\");
    wrenInsertInList(vm, 0, -1, 1);  // Append to list
}

// Wren callbacks with handles
void wren_UIElementSetOnClick(WrenVM* vm)
{
    WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
    WrenHandle* callback = wrenGetSlotHandle(vm, 1);
    
    uiElement->element->SetOnClick([vm, callback](UIElement* sender, const glm::vec2& mousePos) {
        wrenEnsureSlots(vm, 5);
        wrenSetSlotHandle(vm, 0, callback);
        
        // Set up parameters...
        WrenHandle* method = wrenMakeCallHandle(vm, \"call(_,_)\");
        wrenCall(vm, method);
        wrenReleaseHandle(vm, method);
    });
    
    // Clean up handle when element is destroyed
    uiElement->element->SetOnClickDispose([vm, callback](UIElement* sender, GameContext& context) {
        wrenReleaseHandle(vm, callback);
    });
}
```

### Foreign Class Wrapper Pattern
```cpp
// Header
struct WrenDialogueData
{
    Struktur::Dialogue::DialogueDataMap* dataMap;
    bool loadedIntoManager;
    
    WrenDialogueData();
    ~WrenDialogueData();
};

// Implementation
WrenDialogueData::WrenDialogueData()
    : dataMap(new Struktur::Dialogue::DialogueDataMap())
    , loadedIntoManager(false)
{
}

WrenDialogueData::~WrenDialogueData()
{
    if (!loadedIntoManager)
    {
        DEBUG_WARNING(\"WrenDialogueData not loaded into manager\");
    }
    delete dataMap;
}
```

---

## Comments & Documentation

### File Headers
```cpp
// DialogueManager.h
// Manages dialogue flow, node processing, and condition/command execution
// Part of the Struktur dialogue system

#pragma once
```

### Section Comments
```cpp
// ============================================================================
// DIALOGUE MANAGER BINDINGS
// ============================================================================

// ===== NPC DIALOGUES =====

// --- Helper Functions ---
```

### Function Documentation
```cpp
// Brief description for complex functions
// ProcessCurrentNode evaluates conditions, executes commands, and returns
// the current dialogue state with available choices
// @param context - Game context providing access to managers
DialogueResult ProcessCurrentNode(GameContext& context);

// For Wren bindings, describe the Wren signature
// DialogueManager.startDialogue(nodeId) -> Map
// Returns: { \"status\": num, \"text\": string, \"choices\": list, \"hasEnded\": bool }
void wren_DialogueManagerStartDialogue(WrenVM* vm);
```

### Inline Comments
```cpp
// Comment WHY, not WHAT (code should be self-documenting for WHAT)

// ✅ Good - explains reasoning
// We need to copy to /mnt/user-data/outputs/ so the user can download it
CopyToOutputDirectory(filePath);

// ❌ Bad - just describes what code does
// Copy the file to the outputs directory
CopyToOutputDirectory(filePath);

// Explain non-obvious behavior
m_currentNode = it->second.get();  // Non-owning pointer to node in m_nodes map

// Mark TODOs clearly
// TODO: Implement camera shake for dialogue events
// TODO(username): Add support for multiple concurrent dialogues
```

### Justification Comments for Stored References
```cpp
class PhysicsSystem
{
public:
    // Reference stored because:
    // - Every public method requires context
    // - System is tightly coupled to context by design
    // - Context guaranteed to outlive system (owned by Game class)
    explicit PhysicsSystem(GameContext& context)
        : m_gameContext(context)
    {
    }
    
private:
    GameContext& m_gameContext;
};
```

---

## Additional Guidelines

### Formatting
```cpp
// Use tabs for indentation
// Opening braces on same line for functions/classes
void Function()
{
    if (condition)
    {
        DoSomething();
    }
}

// Max line length: ~120 characters (soft limit)
// Break long function calls:
dialogueManager.RegisterConditionType(\"hasItem\", 
    [](const auto& params) {
        return std::make_unique<ItemCondition>(params.at(\"item\"));
    });
```

### Modern C++ Features
```cpp
// Use auto where type is obvious
auto it = m_nodes.find(nodeId);
auto result = dialogueManager.StartDialogue(context, nodeId);

// Range-based for loops
for (const auto& [nodeId, node] : m_nodes)
{
    ValidateNode(node.get());
}

for (const auto& condition : conditions)
{
    if (!condition->Evaluate(context))
        return false;
}

// Structured bindings (C++17)
auto [success, value] = TryGetValue(key);

// std::optional for maybe values
std::optional<DialogueNode*> FindNode(const std::string& id);
```

### String Formatting
```cpp
// Use std::format (C++20) for complex strings
DEBUG_ERROR(\"Invalid choice index %d (available: %zu)\", choiceIndex, choices.size());
ASSERT_MSG(it != m_nodes.end(), std::format(\"Node '{}' not found\", nodeId).c_str());

// Simple concatenation for basic cases
std::string description = \"Flag '\" + flagName + \"' == \" + (value ? \"true\" : \"false\");
```

---

## Summary Checklist

When writing code for Struktur, ensure:

- ✅ **Pass dependencies as parameters by default** (especially GameContext)
- ✅ Only store references when justified and documented
- ✅ Use explicit Dispose methods instead of destructor dependencies
- ✅ Member variables use `m_` prefix
- ✅ Public methods use PascalCase
- ✅ Wren bindings use `wren_ClassName` format and NO namespaces
- ✅ Use `unique_ptr` for ownership, raw pointers for references
- ✅ Use DEBUG_* macros and ASSERT_MSG for all error handling
- ✅ Foreign class wrappers track ownership with bool flag
- ✅ Use provided WREN_* macros for binding registration
- ✅ Get GameContext in Wren bindings via `wrenGetUserData(vm)`
- ✅ Document complex logic with WHY comments
- ✅ Follow the established architecture patterns

---

**Document Version**: 1.1  
**Last Updated**: 2026-01-27  
**Engine**: Struktur Game Engine
