#include "InspectorWindow.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp> 

#include "HierarchyWindow.h"
#include "Engine/GameContext.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/Sprite.h"


namespace Struktur::Debug
{
    void InspectorWindow::Render(GameContext& context)
    {
        if (!m_isVisible)
            return;
        
        ImGui::Begin(m_name.c_str(), &m_isOpen);
        
        if (!m_hierarchyWindow)
        {
            ImGui::Text("No hierarchy window connected.");
            ImGui::End();
            return;
        }
        
        entt::entity selectedEntity = m_hierarchyWindow->GetSelectedEntity();
        
        if (selectedEntity == entt::null)
        {
            ImGui::Text("No entity selected.");
            ImGui::Text("Select an entity in the Hierarchy window.");
            ImGui::End();
            return;
        }
        
        entt::registry& registry = context.GetRegistry();
        
        if (!registry.valid(selectedEntity))
        {
            ImGui::Text("Selected entity is invalid.");
            ImGui::End();
            return;
        }
        
        // Render entity header
        RenderEntityHeader(context, selectedEntity);
        
        ImGui::Separator();
        
        // Render all components
        RenderComponents(context, selectedEntity);
        
        ImGui::End();
    }
    
    void InspectorWindow::RegisterDefaultRenderers()
    {
        // Register LocalTransform renderer
        RegisterComponentRenderer<Component::LocalTransform>(
            "LocalTransform",
            [this](Component::LocalTransform& transform, entt::registry& registry, entt::entity entity)
            {
                RenderLocalTransformComponent(transform, registry, entity);
            }
        );
        
        // Register Sprite renderer
        RegisterComponentRenderer<Component::Sprite>(
            "Sprite",
            [this](Component::Sprite& sprite, entt::registry& registry, entt::entity entity)
            {
                RenderSpriteComponent(sprite, registry, entity);
            }
        );
    }
    
    void InspectorWindow::RenderEntityHeader(GameContext& context, entt::entity entity)
    {
        entt::registry& registry = context.GetRegistry();
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
        ImGui::Text("Entity: %d", (int)entity);
        ImGui::PopStyleColor();
        
        auto* identifier = registry.try_get<Component::Identifier>(entity);
        if (identifier)
        {
            ImGui::Text("Type: %s", identifier->type.c_str());
        }
    }
    
    void InspectorWindow::RenderComponents(GameContext& context, entt::entity entity)
    {
        entt::registry& registry = context.GetRegistry();
        
        // Render LocalTransform if exists
        if (auto* transform = registry.try_get<Component::LocalTransform>(entity))
        {
            if (ImGui::CollapsingHeader("Local Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushID("LocalTransform");
                RenderLocalTransformComponent(*transform, registry, entity);
                ImGui::PopID();
            }
        }
        
        // Render Sprite if exists
        if (auto* sprite = registry.try_get<Component::Sprite>(entity))
        {
            if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushID("Sprite");
                RenderSpriteComponent(*sprite, registry, entity);
                ImGui::PopID();
            }
        }
        
        // Render Identifier if exists
        if (auto* identifier = registry.try_get<Component::Identifier>(entity))
        {
            if (ImGui::CollapsingHeader("Identifier"))
            {
                ImGui::PushID("Identifier");
                ImGui::Text("Type: %s", identifier->type.c_str());
                ImGui::PopID();
            }
        }
        
        // Render Parent if exists
        if (auto* parent = registry.try_get<Component::Parent>(entity))
        {
            if (ImGui::CollapsingHeader("Parent"))
            {
                ImGui::PushID("Parent");
                ImGui::Text("Parent Entity: %d", (int)parent->entity);
                ImGui::PopID();
            }
        }
        
        // Render Children if exists
        if (auto* children = registry.try_get<Component::Children>(entity))
        {
            if (ImGui::CollapsingHeader("Children"))
            {
                ImGui::PushID("Children");
                ImGui::Text("Child Count: %zu", children->entities.size());
                for (size_t i = 0; i < children->entities.size(); i++)
                {
                    ImGui::Text("  Child %zu: %d", i, (int)children->entities[i]);
                }
                ImGui::PopID();
            }
        }
    }
    
    void InspectorWindow::RenderLocalTransformComponent(Component::LocalTransform& transform, 
                                                       entt::registry& registry, 
                                                       entt::entity entity)
    {
        bool modified = false;
        
        // Position
        if (RenderVec3("Position", transform.position))
        {
            modified = true;
        }
        
        // Rotation (as Euler angles for easier editing)
        glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(transform.rotation));
        if (RenderVec3("Rotation", eulerAngles))
        {
            // Convert back to quaternion
            transform.rotation = glm::quat(glm::radians(eulerAngles));
            modified = true;
        }
        
        // Scale
        if (RenderVec3("Scale", transform.scale))
        {
            modified = true;
        }
        
        // If anything was modified, recalculate the matrix
        if (modified)
        {
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), transform.position);
            glm::mat4 rotationMatrix = glm::toMat4(transform.rotation);
            glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), transform.scale);
            
            transform.matrix = translationMatrix * rotationMatrix * scaleMatrix;
        }
        
        ImGui::Spacing();
        
        // Show matrix (read-only for now)
        if (ImGui::TreeNode("Transform Matrix"))
        {
            ImGui::Text("[ %.2f, %.2f, %.2f, %.2f ]", 
                transform.matrix[0][0], transform.matrix[1][0], 
                transform.matrix[2][0], transform.matrix[3][0]);
            ImGui::Text("[ %.2f, %.2f, %.2f, %.2f ]", 
                transform.matrix[0][1], transform.matrix[1][1], 
                transform.matrix[2][1], transform.matrix[3][1]);
            ImGui::Text("[ %.2f, %.2f, %.2f, %.2f ]", 
                transform.matrix[0][2], transform.matrix[1][2], 
                transform.matrix[2][2], transform.matrix[3][2]);
            ImGui::Text("[ %.2f, %.2f, %.2f, %.2f ]", 
                transform.matrix[0][3], transform.matrix[1][3], 
                transform.matrix[2][3], transform.matrix[3][3]);
            ImGui::TreePop();
        }
        
        ImGui::Spacing();
        
        // Reset buttons
        if (ImGui::Button("Reset Position"))
        {
            transform.position = glm::vec3(0.0f);
            modified = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Rotation"))
        {
            transform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            modified = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Scale"))
        {
            transform.scale = glm::vec3(1.0f);
            modified = true;
        }
        
        if (modified)
        {
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), transform.position);
            glm::mat4 rotationMatrix = glm::toMat4(transform.rotation);
            glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), transform.scale);
            
            transform.matrix = translationMatrix * rotationMatrix * scaleMatrix;
        }
    }
    
    void InspectorWindow::RenderSpriteComponent(Component::Sprite& sprite, 
                                               entt::registry& registry, 
                                               entt::entity entity)
    {
        // Texture info
        if (sprite.texture)
        {
            ImGui::Text("Texture: Loaded");
            // TODO: Show texture preview
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Texture: None");
        }
        
        ImGui::Spacing();
        
        // Color tint
        RenderColor("Color Tint", sprite.color);
        
        // Offset
        RenderVec2("Offset", sprite.offset);
        
        ImGui::Spacing();
        
        // Sprite sheet properties
        ImGui::Text("Sprite Sheet:");
        ImGui::DragInt("Columns", &sprite.columns, 1.0f, 1, 100);
        ImGui::DragInt("Rows", &sprite.rows, 1.0f, 1, 100);
        ImGui::DragInt("Index", &sprite.index, 1.0f, 0, sprite.columns * sprite.rows - 1);
        
        ImGui::Spacing();
        
        // Flipped
        ImGui::Checkbox("Flipped", &sprite.flipped);
        
        // Render priority
        ImGui::DragInt("Render Priority", &sprite.renderPriority, 1.0f, -100, 100);
        
        ImGui::Spacing();
        
        // Preview calculated values
        if (ImGui::TreeNode("Calculated Values"))
        {
            int totalFrames = sprite.columns * sprite.rows;
            ImGui::Text("Total Frames: %d", totalFrames);
            
            if (totalFrames > 0)
            {
                int row = sprite.index / sprite.columns;
                int col = sprite.index % sprite.columns;
                ImGui::Text("Current Frame: Row %d, Col %d", row, col);
            }
            
            ImGui::TreePop();
        }
    }
    
    // ====================================================================
    // Helper functions for rendering common data types
    // ====================================================================
    
    bool InspectorWindow::RenderVec2(const char* label, glm::vec2& vec)
    {
        float values[2] = { vec.x, vec.y };
        bool modified = ImGui::DragFloat2(label, values, 0.1f);
        if (modified)
        {
            vec.x = values[0];
            vec.y = values[1];
        }
        return modified;
    }
    
    bool InspectorWindow::RenderVec3(const char* label, glm::vec3& vec)
    {
        float values[3] = { vec.x, vec.y, vec.z };
        bool modified = ImGui::DragFloat3(label, values, 0.1f);
        if (modified)
        {
            vec.x = values[0];
            vec.y = values[1];
            vec.z = values[2];
        }
        return modified;
    }
    
    bool InspectorWindow::RenderVec4(const char* label, glm::vec4& vec)
    {
        float values[4] = { vec.x, vec.y, vec.z, vec.w };
        bool modified = ImGui::DragFloat4(label, values, 0.1f);
        if (modified)
        {
            vec.x = values[0];
            vec.y = values[1];
            vec.z = values[2];
            vec.w = values[3];
        }
        return modified;
    }
    
    bool InspectorWindow::RenderQuat(const char* label, glm::quat& quat)
    {
        // Display as Euler angles for easier editing
        glm::vec3 euler = glm::degrees(glm::eulerAngles(quat));
        float values[3] = { euler.x, euler.y, euler.z };
        bool modified = ImGui::DragFloat3(label, values, 0.5f);
        if (modified)
        {
            euler.x = values[0];
            euler.y = values[1];
            euler.z = values[2];
            quat = glm::quat(glm::radians(euler));
        }
        return modified;
    }
    
    bool InspectorWindow::RenderColor(const char* label, ::Color& color)
    {
        float col[4] = {
            color.r / 255.0f,
            color.g / 255.0f,
            color.b / 255.0f,
            color.a / 255.0f
        };
        
        bool modified = ImGui::ColorEdit4(label, col);
        
        if (modified)
        {
            color.r = (unsigned char)(col[0] * 255.0f);
            color.g = (unsigned char)(col[1] * 255.0f);
            color.b = (unsigned char)(col[2] * 255.0f);
            color.a = (unsigned char)(col[3] * 255.0f);
        }
        
        return modified;
    }
}
