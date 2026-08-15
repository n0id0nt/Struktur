#include "InspectorWindow.h"

#include <cmath>
#include <unordered_map>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Debug/Editor/PreviewRenderers/PreviewHelpers.h"
#include "Debug/Editor/Windows/HierarchyWindow.h"
#include "Debug/Editor/Windows/PreviewWindow.h"
#include "Debug/Editor/Windows/UIHierarchyWindow.h"
#include "Engine/ECS/Component/Active.h"
#include "Engine/Animation/SpriteAnimation.h"
#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/Level.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Shader.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/World.h"
#include "Engine/ECS/Component/WrenScript.h"
#include "Engine/ECS/System/AnimationSystem.h"
#include "Engine/ECS/System/HierarchySystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/FileLoading/LevelParser.h"
#include "Engine/GameContext.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/World/RenderLayer.h"
#include "Engine/UI/UIElement.h"

namespace Struktur::Debug
{
InspectorWindow::InspectorWindow(HierarchyWindow* hierarchyWindow, UIHierarchyWindow* uiHierarchyWindow,
                                 PreviewWindow* previewWindow)
    : EditorWindow("Inspector"),
      m_hierarchyWindow(hierarchyWindow),
      m_uiHierarchyWindow(uiHierarchyWindow),
      m_previewWindow(previewWindow),
      m_displayType(InspectorDisplayType::None),
      m_selectedUIElement(nullptr),
      m_selectedEntity(entt::null)
{
	// Register default component renderers
	RegisterDefaultRenderers();
}

void InspectorWindow::Render(GameContext& context)
{
	if (!m_isVisible)
	{
		return;
	}

	ImGui::Begin(m_name.c_str(), &m_isOpen);

	// Check if we have a UI element selected
	UI::UIElement* selectedUIElement = m_uiHierarchyWindow->GetSelectedElement();
	if (selectedUIElement != m_selectedUIElement)
	{
		m_selectedUIElement = selectedUIElement;
		if (m_selectedUIElement)
		{
			m_displayType = InspectorDisplayType::UIElement;
			m_hierarchyWindow->SetSelectedEntity(entt::null);
		}
		else if (m_selectedEntity != entt::null)
		{
			m_displayType = InspectorDisplayType::Entity;
		}
		else
		{
			m_displayType = InspectorDisplayType::None;
		}
	}

	// Check if we have an entity selected
	entt::entity selectedEntity = m_hierarchyWindow->GetSelectedEntity();
	if (selectedEntity != m_selectedEntity)
	{
		m_selectedEntity = selectedEntity;
		if (m_selectedEntity != entt::null)
		{
			m_displayType = InspectorDisplayType::Entity;
			m_uiHierarchyWindow->SetSelectedElement(nullptr);
		}
		else if (m_selectedUIElement)
		{
			m_displayType = InspectorDisplayType::UIElement;
		}
		else
		{
			m_displayType = InspectorDisplayType::None;
		}
	}

	// Determine what to display based on most recent selection
	// UI elements take precedence if both are selected
	switch (m_displayType)
	{
		case InspectorDisplayType::UIElement:
		{
			RenderUIElementInspector(context, m_selectedUIElement);
		}
		case InspectorDisplayType::Entity:
		{
			RenderEntityInspector(context, m_selectedEntity);
		}
		case InspectorDisplayType::None:
		default:
			ImGui::Text("No selection.");
			ImGui::Text("Select an entity in the Hierarchy window");
			ImGui::Text("or a UI element in the UI Hierarchy window.");
	}

	ImGui::End();
}

void InspectorWindow::RegisterDefaultRenderers()
{
	// Register LocalTransform renderer
	RegisterComponentRenderer<Component::Transform>(
	    "LocalTransform",
	    [this](GameContext& context, Component::Transform& transform, entt::registry& registry,
	           entt::entity entity) { RenderLocalTransformComponent(context, transform, registry, entity); });

	// Register Sprite renderer
	RegisterComponentRenderer<Component::Sprite>("Sprite", [this](GameContext& context, Component::Sprite& sprite,
	                                                              entt::registry& registry, entt::entity entity)
	                                             { RenderSpriteComponent(context, sprite, registry, entity); });
}

// ====================================================================
// Entity Inspector Rendering
// ====================================================================

void InspectorWindow::RenderEntityInspector(GameContext& context, entt::entity entity)
{
	entt::registry& registry = context.GetRegistry();

	if (!registry.valid(entity))
	{
		ImGui::Text("Selected entity is invalid.");
		return;
	}

	// Render entity header
	RenderEntityHeader(context, entity);

	ImGui::Separator();

	// Render all components
	RenderComponents(context, entity);
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

	// Render Active if exists
	if (auto* active = registry.try_get<Component::Active>(entity))
	{
		if (ImGui::CollapsingHeader("Active", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("Active");
			RenderActiveComponent(context, *active, registry, entity);
			ImGui::PopID();
		}
	}

	// Render LocalTransform if exists
	if (auto* transform = registry.try_get<Component::Transform>(entity))
	{
		if (ImGui::CollapsingHeader("Local Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("LocalTransform");
			RenderLocalTransformComponent(context, *transform, registry, entity);
			ImGui::PopID();
		}
	}

	// Render Sprite if exists
	if (auto* sprite = registry.try_get<Component::Sprite>(entity))
	{
		if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("Sprite");
			RenderSpriteComponent(context, *sprite, registry, entity);
			ImGui::PopID();
		}
	}

	// Render Shader if exists
	if (auto* shader = registry.try_get<Component::Shader>(entity))
	{
		if (ImGui::CollapsingHeader("Shader", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("Shader");
			RenderShaderComponent(context, *shader, registry, entity);
			ImGui::PopID();
		}
	}

	// Render Camera if exists
	if (auto* camera = registry.try_get<Component::Camera>(entity))
	{
		if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("Camera");
			RenderCameraComponent(context, *camera, registry, entity);
			ImGui::PopID();
		}
	}

	// Render PhysicsBody if exists
	if (auto* physicsBody = registry.try_get<Component::PhysicsBody>(entity))
	{
		if (ImGui::CollapsingHeader("Physics Body", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("PhysicsBody");
			RenderPhysicsBodyComponent(context, *physicsBody, registry, entity);
			ImGui::PopID();
		}
	}

	// Render SpriteAnimation if exists
	if (auto* spriteAnimation = registry.try_get<Component::SpriteAnimation>(entity))
	{
		if (ImGui::CollapsingHeader("Sprite Animation", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("SpriteAnimation");
			RenderSpriteAnimationComponent(context, *spriteAnimation, registry, entity);
			ImGui::PopID();
		}
	}

	// Render Level if exists
	if (auto* level = registry.try_get<Component::Level>(entity))
	{
		if (ImGui::CollapsingHeader("Level"))
		{
			ImGui::PushID("Level");
			RenderLevelComponent(context, *level, registry, entity);
			ImGui::PopID();
		}
	}

	// Render World if exists
	if (auto* world = registry.try_get<Component::World>(entity))
	{
		if (ImGui::CollapsingHeader("World"))
		{
			ImGui::PushID("World");
			RenderWorldComponent(context, *world, registry, entity);
			ImGui::PopID();
		}
	}

	// Render TileMap if exists
	if (auto* tileMap = registry.try_get<Component::TileMap>(entity))
	{
		if (ImGui::CollapsingHeader("Tile Map", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("TileMap");
			RenderTileMapComponent(context, *tileMap, registry, entity);
			ImGui::PopID();
		}
	}

	// Render WrenScript if exists
	if (auto* script = registry.try_get<Component::WrenScript>(entity))
	{
		if (ImGui::CollapsingHeader("Wren Script", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("WrenScript");
			RenderWrenScriptComponent(context, *script, registry, entity);
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

// ====================================================================
// UI Element Inspector Rendering
// ====================================================================

void InspectorWindow::RenderUIElementInspector(GameContext& context, UI::UIElement* element)
{
	if (!element)
	{
		ImGui::Text("Selected UI element is invalid.");
		return;
	}

	// Render UI element header
	RenderUIElementHeader(element);

	ImGui::Separator();

	// Render UI element properties
	RenderUIElementProperties(element);
}

void InspectorWindow::RenderUIElementHeader(UI::UIElement* element)
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
	ImGui::Text("UI Element: %s", element->GetId().empty() ? "[Unnamed]" : element->GetId().c_str());
	ImGui::PopStyleColor();

	ImGui::Text("Type: UI Element");
}

void InspectorWindow::RenderUIElementProperties(UI::UIElement* element)
{
	// Basic Properties
	if (ImGui::CollapsingHeader("Basic Properties", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PushID("BasicProperties");

		// ID (editable)
		char idBuffer[256];
		strncpy(idBuffer, element->GetId().c_str(), sizeof(idBuffer) - 1);
		idBuffer[sizeof(idBuffer) - 1] = '\0';
		if (ImGui::InputText("ID", idBuffer, sizeof(idBuffer)))
		{
			element->SetId(idBuffer);
		}

		// State flags
		bool visible = element->IsVisible();
		if (ImGui::Checkbox("Visible", &visible))
		{
			element->SetVisible(visible);
		}

		bool enabled = element->IsEnabled();
		if (ImGui::Checkbox("Enabled", &enabled))
		{
			element->SetEnabled(enabled);
		}

		bool focusable = element->IsFocusable();
		if (ImGui::Checkbox("Focusable", &focusable))
		{
			element->SetFocusable(focusable);
		}

		ImGui::PopID();
	}

	// Transform
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PushID("Transform");

		// Position
		glm::vec2 pos = element->GetPosition();
		if (RenderVec2("Position", pos))
		{
			// Note: UIElement doesn't have a direct SetPosition(vec2) method
			// You might need to calculate absolute/relative positions
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Position editing requires absolute/relative split");
		}

		// Size
		glm::vec2 size = element->GetSize();
		if (RenderVec2("Size", size))
		{
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Size editing requires absolute/relative split");
		}

		// Bounds (read-only)
		Util::Math::Rect bounds = element->GetBounds();
		ImGui::Text("Bounds:");
		ImGui::Indent();
		ImGui::Text("X: %.1f, Y: %.1f", bounds.x, bounds.y);
		ImGui::Text("W: %.1f, H: %.1f", bounds.width, bounds.height);
		ImGui::Unindent();

		ImGui::PopID();
	}

	// Appearance
	if (ImGui::CollapsingHeader("Appearance"))
	{
		ImGui::PushID("Appearance");

		// Background color
		Util::Color bgColor = element->GetBackgroundColor();
		if (RenderColor("Background Color", bgColor))
		{
			element->SetBackgroundColor(bgColor);
		}

		// Border color
		Util::Color borderColor = element->GetBorderColor();
		if (RenderColor("Border Color", borderColor))
		{
			element->SetBorderColor(borderColor);
		}

		// Border width
		float borderWidth = element->GetBorderWidth();
		if (ImGui::DragFloat("Border Width", &borderWidth, 0.1f, 0.0f, 20.0f))
		{
			element->SetBorderWidth(borderWidth);
		}

		ImGui::PopID();
	}

	// Layout
	if (ImGui::CollapsingHeader("Layout"))
	{
		ImGui::PushID("Layout");

		// Z-Index
		int zIndex = element->GetZIndex();
		if (ImGui::DragInt("Z-Index", &zIndex, 1.0f, -100, 100))
		{
			element->SetZIndex(zIndex);
		}

		// Tab Index
		int tabIndex = element->GetTabIndex();
		if (ImGui::DragInt("Tab Index", &tabIndex, 1.0f, -1, 100))
		{
			element->SetTabIndex(tabIndex);
		}

		ImGui::PopID();
	}

	// Hierarchy
	if (ImGui::CollapsingHeader("Hierarchy"))
	{
		ImGui::PushID("Hierarchy");

		// Parent
		ImGui::Text("Parent: %s", element->GetParent() ? element->GetParent()->GetId().c_str() : "[None]");

		// Children
		const auto& children = element->GetChildren();
		ImGui::Text("Children: %zu", children.size());
		if (!children.empty())
		{
			ImGui::Indent();
			for (size_t i = 0; i < children.size(); ++i)
			{
				const auto& child = children[i];
				ImGui::Text("%zu: %s", i, child->GetId().empty() ? "[Unnamed]" : child->GetId().c_str());
			}
			ImGui::Unindent();
		}

		ImGui::PopID();
	}

	// Navigation
	if (ImGui::CollapsingHeader("Navigation"))
	{
		ImGui::PushID("Navigation");

		const char* directions[] = {"Up", "Down", "Left", "Right"};
		for (int i = 0; i < 4; ++i)
		{
			auto* neighbor = element->GetNavigationNeighbor(static_cast<UI::NavigationDirection>(i));
			ImGui::Text("%s: %s", directions[i],
			            neighbor ? (neighbor->GetId().empty() ? "[Unnamed]" : neighbor->GetId().c_str()) : "[None]");
		}

		ImGui::PopID();
	}

	// Callbacks (read-only info)
	if (ImGui::CollapsingHeader("Callbacks"))
	{
		ImGui::PushID("Callbacks");
		ImGui::TextDisabled("Callback information is not directly accessible");
		ImGui::Text("Callbacks may be set via code");
		ImGui::PopID();
	}
}

// ====================================================================
// Component Renderers
// ====================================================================

void InspectorWindow::RenderActiveComponent(GameContext& context, Component::Active& active, entt::registry& registry,
                                            entt::entity entity)
{
	bool isActive = active.activeState == Component::Active::ActiveState::Active;
	bool isInactiveParent = active.activeState == Component::Active::ActiveState::InactiveParent;

	// Disabled when inactive purely because an ancestor is inactive - can't override that from here
	ImGui::BeginDisabled(isInactiveParent);
	if (ImGui::Checkbox("Active", &isActive))
	{
		auto& hierarchySystem = context.GetSystemManager().GetSystem<System::HierarchySystem>();
		if (isActive)
		{
			hierarchySystem.ActivevateEntity(context, entity);
		}
		else
		{
			hierarchySystem.DeactivevateEntity(context, entity);
		}
	}
	ImGui::EndDisabled();

	if (isInactiveParent)
	{
		ImGui::TextDisabled("Inactive because a parent is inactive");
	}
}

void InspectorWindow::RenderCameraComponent(GameContext& context, Component::Camera& camera, entt::registry& registry,
                                            entt::entity entity)
{
	ImGui::DragInt("Priority", &camera.cameraPriority);
	ImGui::DragFloat("Zoom", &camera.zoom, 0.01f, 0.01f, 100.0f);
	ImGui::DragFloat("Angle", &camera.angle, 0.5f);

	ImGui::Spacing();

	RenderVec2("Offset", camera.offset);
	RenderVec2("Dead Zone", camera.deadZone);
	RenderVec2("Damping", camera.damping);
	ImGui::Checkbox("Force Position", &camera.forcePosition);

	ImGui::Spacing();

	if (ImGui::TreeNode("Screen Shake"))
	{
		ImGui::DragFloat("Trauma", &camera.trauma, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Trauma Time", &camera.traumaTime, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Max Offset", &camera.maxOffset, 0.01f);
		ImGui::DragFloat("Max Angle", &camera.maxAngle, 0.01f);
		ImGui::DragFloat("Shake Amplitude", &camera.shakeAmplitude, 0.01f);

		ImGui::TreePop();
	}
}

void InspectorWindow::RenderPhysicsBodyComponent(GameContext& context, Component::PhysicsBody& physicsBody,
                                                 entt::registry& registry, entt::entity entity)
{
	if (!physicsBody.body)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No underlying Box2D body");
		return;
	}

	// Everything below reads/writes the live b2Body directly - this is the runtime physics state,
	// not just the ECS component, so edits here take effect immediately in the simulation.
	b2Body* body = physicsBody.body;

	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
	const float ppm            = physicsWorld.GetPixelsPerMeter();
	const float metersPerPixel = 1.0f / ppm;

	static const char* k_bodyTypeNames[] = {"Static", "Kinematic", "Dynamic"};
	int bodyTypeIndex = static_cast<int>(body->GetType());
	if (ImGui::Combo("Body Type", &bodyTypeIndex, k_bodyTypeNames, IM_ARRAYSIZE(k_bodyTypeNames)))
	{
		body->SetType(static_cast<b2BodyType>(bodyTypeIndex));
	}

	ImGui::Spacing();

	b2Vec2 physPosition = body->GetPosition();
	glm::vec2 position(physPosition.x * ppm, physPosition.y * ppm);
	if (RenderVec2("Position", position))
	{
		body->SetTransform(b2Vec2(position.x * metersPerPixel, position.y * metersPerPixel), body->GetAngle());
	}

	float angleDegrees = glm::degrees(body->GetAngle());
	if (ImGui::DragFloat("Angle", &angleDegrees, 0.5f))
	{
		body->SetTransform(body->GetPosition(), glm::radians(angleDegrees));
	}

	ImGui::Spacing();

	b2Vec2 physVelocity = body->GetLinearVelocity();
	glm::vec2 linearVelocity(physVelocity.x, physVelocity.y);
	if (RenderVec2("Linear Velocity", linearVelocity))
	{
		body->SetLinearVelocity(b2Vec2(linearVelocity.x, linearVelocity.y));
	}

	float angularVelocity = body->GetAngularVelocity();
	if (ImGui::DragFloat("Angular Velocity", &angularVelocity, 0.1f))
	{
		body->SetAngularVelocity(angularVelocity);
	}

	ImGui::Spacing();

	float linearDamping = body->GetLinearDamping();
	if (ImGui::DragFloat("Linear Damping", &linearDamping, 0.01f, 0.0f, 10.0f))
	{
		body->SetLinearDamping(linearDamping);
	}

	float angularDamping = body->GetAngularDamping();
	if (ImGui::DragFloat("Angular Damping", &angularDamping, 0.01f, 0.0f, 10.0f))
	{
		body->SetAngularDamping(angularDamping);
	}

	float gravityScale = body->GetGravityScale();
	if (ImGui::DragFloat("Gravity Scale", &gravityScale, 0.01f))
	{
		body->SetGravityScale(gravityScale);
	}

	ImGui::Spacing();

	bool fixedRotation = body->IsFixedRotation();
	if (ImGui::Checkbox("Fixed Rotation", &fixedRotation))
	{
		body->SetFixedRotation(fixedRotation);
	}

	bool bullet = body->IsBullet();
	if (ImGui::Checkbox("Bullet", &bullet))
	{
		body->SetBullet(bullet);
	}

	bool sleepingAllowed = body->IsSleepingAllowed();
	if (ImGui::Checkbox("Sleeping Allowed", &sleepingAllowed))
	{
		body->SetSleepingAllowed(sleepingAllowed);
	}

	bool awake = body->IsAwake();
	if (ImGui::Checkbox("Awake", &awake))
	{
		body->SetAwake(awake);
	}

	bool enabled = body->IsEnabled();
	if (ImGui::Checkbox("Enabled", &enabled))
	{
		body->SetEnabled(enabled);
	}

	ImGui::Spacing();

	if (ImGui::TreeNode("Mass Data"))
	{
		ImGui::Text("Mass: %.3f kg", body->GetMass());
		ImGui::Text("Inertia: %.3f", body->GetInertia());
		ImGui::TreePop();
	}
}

void InspectorWindow::RenderSpriteAnimationComponent(GameContext& context, Component::SpriteAnimation& spriteAnimation,
                                                     entt::registry& registry, entt::entity entity)
{
	auto& animationSystem = context.GetSystemManager().GetSystem<System::AnimationSystem>();

	ImGui::Text("Current Animation: %s",
	           spriteAnimation.curAnimation.empty() ? "[None]" : spriteAnimation.curAnimation.c_str());

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Text("Animations (%zu)", spriteAnimation.animations.size());

	// Editing the fields below writes straight into the component's animation map - the fields are
	// plain scalars so there's no need for a copy/apply step like Transform's quaternion conversion.
	for (auto& [name, animation] : spriteAnimation.animations)
	{
		ImGui::PushID(name.c_str());

		bool isCurrent = spriteAnimation.curAnimation == name;
		if (isCurrent)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
		}
		bool open = ImGui::TreeNodeEx(name.c_str(), isCurrent ? ImGuiTreeNodeFlags_DefaultOpen : 0, "%s%s",
		                             name.c_str(), isCurrent ? " (Current)" : "");
		if (isCurrent)
		{
			ImGui::PopStyleColor();
		}

		if (open)
		{
			int startFrame = static_cast<int>(animation.startFrame);
			if (ImGui::DragInt("Start Frame", &startFrame, 1.0f, 0, 10000))
			{
				animation.startFrame = static_cast<unsigned int>(startFrame);
			}

			int endFrame = static_cast<int>(animation.endFrame);
			if (ImGui::DragInt("End Frame", &endFrame, 1.0f, 0, 10000))
			{
				animation.endFrame = static_cast<unsigned int>(endFrame);
			}

			ImGui::DragFloat("Duration (s)", &animation.animationTime, 0.01f, 0.01f, 60.0f);
			ImGui::Checkbox("Loop", &animation.loop);

			if (ImGui::Button("Play"))
			{
				animationSystem.PlayAnimation(context, entity, name);
			}

			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	ImGui::Spacing();
	ImGui::Separator();

	if (ImGui::TreeNode("New Animation"))
	{
		ImGui::InputText("Name", m_newAnimationNameBuffer, sizeof(m_newAnimationNameBuffer));
		ImGui::DragInt("Start Frame##New", &m_newAnimationStartFrame, 1.0f, 0, 10000);
		ImGui::DragInt("End Frame##New", &m_newAnimationEndFrame, 1.0f, 0, 10000);
		ImGui::DragFloat("Duration (s)##New", &m_newAnimationTime, 0.01f, 0.01f, 60.0f);
		ImGui::Checkbox("Loop##New", &m_newAnimationLoop);

		bool nameEmpty = m_newAnimationNameBuffer[0] == '\0';
		bool nameTaken =
		    !nameEmpty && spriteAnimation.animations.find(m_newAnimationNameBuffer) != spriteAnimation.animations.end();

		ImGui::BeginDisabled(nameEmpty || nameTaken);
		if (ImGui::Button("Create Animation"))
		{
			Animation::SpriteAnimation newAnimation;
			newAnimation.startFrame    = static_cast<unsigned int>(m_newAnimationStartFrame);
			newAnimation.endFrame      = static_cast<unsigned int>(m_newAnimationEndFrame);
			newAnimation.animationTime = m_newAnimationTime;
			newAnimation.loop          = m_newAnimationLoop;

			animationSystem.AddAnimation(context, entity, m_newAnimationNameBuffer, newAnimation);

			m_newAnimationNameBuffer[0] = '\0';
			m_newAnimationStartFrame    = 0;
			m_newAnimationEndFrame      = 1;
			m_newAnimationTime          = 1.0f;
			m_newAnimationLoop          = true;
		}
		ImGui::EndDisabled();

		if (nameEmpty)
		{
			ImGui::TextDisabled("Enter a name to create an animation");
		}
		else if (nameTaken)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "An animation with this name already exists");
		}

		ImGui::TreePop();
	}
}

void InspectorWindow::RenderLevelComponent(GameContext& context, Component::Level& level, entt::registry& registry,
                                           entt::entity entity)
{
	// Sourced from the loaded level file (see World::Level::LoadLevelEntities) - not meaningful to edit here
	ImGui::Text("Index: %d", level.index);
	ImGui::Text("Iid: %s", level.Iid.c_str());
	ImGui::Text("Size: %d x %d px", level.width, level.height);
	ImGui::TextDisabled("Read-only - sourced from the loaded level file");
}

void InspectorWindow::RenderWorldComponent(GameContext& context, Component::World& world, entt::registry& registry,
                                           entt::entity entity)
{
	const auto& worldMap = world.worldMap;

	// Sourced from the loaded LDtk world file (see World::Level::CreateWorldEntity) - not meaningful to edit here
	ImGui::Text("Iid: %s", worldMap.Iid.c_str());
	ImGui::Text("File: %s", worldMap.filePath.c_str());
	ImGui::Text("Levels: %zu", worldMap.levels.size());
	ImGui::TextDisabled("Read-only - sourced from the loaded level file");

	ImGui::Spacing();

	static const char* k_layerTypeNames[] = {"Entities", "Int Grid", "Auto Layer", "Tiles"};

	for (size_t i = 0; i < worldMap.levels.size(); i++)
	{
		const auto& level = worldMap.levels[i];

		ImGui::PushID((int)i);
		if (ImGui::TreeNode(level.identifier.c_str(), "[%zu] %s", i, level.identifier.c_str()))
		{
			ImGui::Text("Iid: %s", level.Iid.c_str());
			ImGui::Text("World Position: (%d, %d)", level.worldX, level.worldY);
			ImGui::Text("Size: %d x %d px", level.pxWid, level.pxHei);
			ImGui::Text("Layers: %zu", level.layers.size());

			if (!level.neighbours.empty() && ImGui::TreeNode("Neighbours"))
			{
				for (const auto& neighbour : level.neighbours)
				{
					ImGui::BulletText("%s", neighbour.c_str());
				}
				ImGui::TreePop();
			}

			if (!level.layers.empty() && ImGui::TreeNode("Layers"))
			{
				for (const auto& layer : level.layers)
				{
					int typeIndex        = static_cast<int>(layer.type);
					const char* typeName = (typeIndex >= 0 && typeIndex < IM_ARRAYSIZE(k_layerTypeNames))
					                           ? k_layerTypeNames[typeIndex]
					                           : "Unknown";
					ImGui::BulletText("%s (%s)", layer.identifier.c_str(), typeName);
				}
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}
		ImGui::PopID();
	}
}

void InspectorWindow::RenderTileMapComponent(GameContext& context, Component::TileMap& tileMap,
                                             entt::registry& registry, entt::entity entity)
{
	if (tileMap.texture)
	{
		ImGui::Text("Texture: Loaded");
	}
	else
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Texture: None");
	}
	ImGui::Text("Grid: %d x %d cells (%d px tiles)", tileMap.width, tileMap.height, tileMap.tileSize);
	ImGui::Text("Visible Tiles: %zu", tileMap.gridTiles.size());
	ImGui::TextDisabled("The dense collision grid is baked into physics at load and isn't re-generated here");

	static const char* k_renderLayerNames[] = {"Background Far",    "Background Mid", "Entities",
	                                           "Background Overlay", "Foreground",     "UI"};
	int layerIndex = static_cast<int>(tileMap.layer);
	if (ImGui::Combo("Render Layer", &layerIndex, k_renderLayerNames, IM_ARRAYSIZE(k_renderLayerNames)))
	{
		tileMap.layer = static_cast<World::RenderLayer>(layerIndex);
	}
	ImGui::DragFloat("Order In Layer", &tileMap.orderInLayer, 1.0f, -1000.0f, 1000.0f);

	ImGui::Spacing();
	ImGui::Separator();

	const int cellCount = tileMap.width * tileMap.height;
	if (tileMap.width <= 0 || tileMap.height <= 0 || tileMap.tileSize <= 0)
	{
		ImGui::TextDisabled("No grid data");
		return;
	}
	if (cellCount > 4096)
	{
		ImGui::TextDisabled("Grid too large to edit here (%d cells)", cellCount);
		return;
	}

	// gridTiles only holds the tiles that are actually drawn (sparse), keyed by tile-local pixel position -
	// rebuild a cell -> gridTiles index lookup each frame rather than caching it, since edits below can
	// add/remove entries.
	std::unordered_map<int, int> cellToTileIndex;
	for (int i = 0; i < (int)tileMap.gridTiles.size(); i++)
	{
		const World::TileMap::GridTile& tile = tileMap.gridTiles[i];
		int col = (int)std::round(tile.position.x / tileMap.tileSize);
		int row = (int)std::round(tile.position.y / tileMap.tileSize);
		if (col >= 0 && col < tileMap.width && row >= 0 && row < tileMap.height)
		{
			cellToTileIndex[row * tileMap.width + col] = i;
		}
	}

	ImGui::Text("Click a cell to select it (blue = tile present)");
	ImGui::BeginChild("TileGrid", ImVec2(0, 300), true, ImGuiWindowFlags_HorizontalScrollbar);

	const float cellButtonSize = 18.0f;
	for (int row = 0; row < tileMap.height; row++)
	{
		ImGui::PushID(row);
		for (int col = 0; col < tileMap.width; col++)
		{
			int cellIndex = row * tileMap.width + col;
			bool hasTile  = cellToTileIndex.find(cellIndex) != cellToTileIndex.end();
			bool selected = m_selectedTileMapCell == cellIndex;

			ImVec4 color = selected ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f)
			                       : (hasTile ? ImVec4(0.3f, 0.6f, 0.9f, 1.0f) : ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

			ImGui::PushID(col);
			ImGui::PushStyleColor(ImGuiCol_Button, color);
			if (ImGui::Button("##cell", ImVec2(cellButtonSize, cellButtonSize)))
			{
				m_selectedTileMapCell = cellIndex;
			}
			ImGui::PopStyleColor();
			ImGui::PopID();

			if (col < tileMap.width - 1)
			{
				ImGui::SameLine(0.0f, 2.0f);
			}
		}
		ImGui::PopID();
	}

	ImGui::EndChild();

	ImGui::Spacing();
	ImGui::Separator();

	if (m_selectedTileMapCell < 0 || m_selectedTileMapCell >= cellCount)
	{
		ImGui::TextDisabled("No cell selected");
		return;
	}

	int selectedRow = m_selectedTileMapCell / tileMap.width;
	int selectedCol = m_selectedTileMapCell % tileMap.width;
	ImGui::Text("Selected Cell: (%d, %d)", selectedCol, selectedRow);

	auto it = cellToTileIndex.find(m_selectedTileMapCell);
	if (it == cellToTileIndex.end())
	{
		ImGui::TextDisabled("Empty cell");
		if (ImGui::Button("Add Tile"))
		{
			World::TileMap::GridTile newTile;
			newTile.position       = glm::vec2(selectedCol * tileMap.tileSize, selectedRow * tileMap.tileSize);
			newTile.sourcePosition = glm::vec2(0.0f, 0.0f);
			newTile.flipBit        = World::TileMap::FlipBit::NONE;
			tileMap.gridTiles.push_back(newTile);
			tileMap.chunksBuilt = false;
		}
		return;
	}

	World::TileMap::GridTile& tile = tileMap.gridTiles[it->second];
	bool changed                  = false;

	int atlasCol = (int)std::round(tile.sourcePosition.x / tileMap.tileSize);
	int atlasRow = (int)std::round(tile.sourcePosition.y / tileMap.tileSize);
	if (ImGui::DragInt("Atlas Column", &atlasCol, 1.0f, 0, 1000))
	{
		changed = true;
	}
	if (ImGui::DragInt("Atlas Row", &atlasRow, 1.0f, 0, 1000))
	{
		changed = true;
	}
	if (changed)
	{
		tile.sourcePosition = glm::vec2(atlasCol * tileMap.tileSize, atlasRow * tileMap.tileSize);
	}

	static const char* k_flipNames[] = {"None", "Horizontal", "Vertical", "Both"};
	int flipIndex = static_cast<int>(tile.flipBit);
	if (ImGui::Combo("Flip", &flipIndex, k_flipNames, IM_ARRAYSIZE(k_flipNames)))
	{
		tile.flipBit = static_cast<World::TileMap::FlipBit>(flipIndex);
		changed      = true;
	}

	if (ImGui::Button("Clear Tile"))
	{
		tileMap.gridTiles.erase(tileMap.gridTiles.begin() + it->second);
		changed = true;
	}

	if (changed)
	{
		tileMap.chunksBuilt = false;
	}
}

void InspectorWindow::RenderWrenScriptComponent(GameContext& context, Component::WrenScript& script,
                                                entt::registry& registry, entt::entity entity)
{
	ImGui::Text("Class: %s", script.className.c_str());

#ifdef DEBUG
	if (!script.filePath.empty())
	{
		ImGui::TextDisabled("File: %s", script.filePath.c_str());
	}
#endif

	if (script.hasError)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: %s", script.errorMessage.c_str());
	}
	else if (script.isInitialised)
	{
		ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Initialised");
	}
	else
	{
		ImGui::TextDisabled("Not yet initialised");
	}

	ImGui::Spacing();

	if (script.constructorArgs.empty())
	{
		ImGui::TextDisabled("No constructor arguments");
		return;
	}

	if (ImGui::TreeNode("Constructor Args"))
	{
		for (const Wren::WrenItem& arg : script.constructorArgs)
		{
			switch (arg.type)
			{
				case WREN_TYPE_NUM:
					ImGui::Text("%s: %.4g", arg.identifier.c_str(), std::any_cast<double>(arg.value));
					break;
				case WREN_TYPE_BOOL:
					ImGui::Text("%s: %s", arg.identifier.c_str(), std::any_cast<bool>(arg.value) ? "true" : "false");
					break;
				case WREN_TYPE_STRING:
					ImGui::Text("%s: \"%s\"", arg.identifier.c_str(),
					           std::any_cast<std::string>(arg.value).c_str());
					break;
				case WREN_TYPE_NULL:
					ImGui::Text("%s: null", arg.identifier.c_str());
					break;
				default:
					ImGui::Text("%s: <unsupported type>", arg.identifier.c_str());
					break;
			}
		}
		ImGui::TreePop();
	}
}

void InspectorWindow::RenderLocalTransformComponent(GameContext& context, Component::Transform& transform,
                                                    entt::registry& registry, entt::entity entity)
{
	bool modified = false;

	glm::vec3 position = transform.localPosition;
	glm::vec3 scale     = transform.localScale;
	glm::quat rotation  = transform.localRotation;

	// Position
	if (RenderVec3("Position", position))
	{
		modified = true;
	}

	// Rotation (as Euler angles for easier editing)
	glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(rotation));
	if (RenderVec3("Rotation", eulerAngles))
	{
		// Convert back to quaternion
		rotation = glm::quat(glm::radians(eulerAngles));
		modified = true;
	}

	// Scale
	if (RenderVec3("Scale", scale))
	{
		modified = true;
	}

	ImGui::Spacing();

	// Show matrix (read-only for now)
	if (ImGui::TreeNode("Transform Matrix"))
	{
		const glm::mat4& matrix = transform.localMatrix;
		ImGui::Text("[ %.2f, %.2f, %.2f, %.2f ]", matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0]);
		ImGui::Text("[ %.2f, %.2f, %.2f, %.2f ]", matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1]);
		ImGui::Text("[ %.2f, %.2f, %.2f, %.2f ]", matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2]);
		ImGui::Text("[ %.2f, %.2f, %.2f, %.2f ]", matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3]);
		ImGui::TreePop();
	}

	ImGui::Spacing();

	// Reset buttons
	if (ImGui::Button("Reset Position"))
	{
		position = glm::vec3(0.0f);
		modified = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset Rotation"))
	{
		rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		modified = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset Scale"))
	{
		scale    = glm::vec3(1.0f);
		modified = true;
	}

	if (modified)
	{
		context.GetSystemManager().GetSystem<System::TransformSystem>().SetLocalTransform(context, entity, position,
		                                                                                  scale, rotation);
	}
}

void InspectorWindow::RenderSpriteComponent(GameContext& context, Component::Sprite& sprite, entt::registry& registry,
                                            entt::entity entity)
{
	// Preview button
	if (ImGui::Button("Preview Sprite"))
	{
		if (m_previewWindow)
		{
			PreviewSprite(m_previewWindow, &sprite, "Sprite Component");
		}
	}

	// Texture info
	if (sprite.texture)
	{
		ImGui::Text("Texture: Loaded");
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

	// Render layer + order
	static const char* k_renderLayerNames[] = {"Background Far",    "Background Mid", "Entities",
	                                           "Background Overlay", "Foreground",     "UI"};
	int layerIndex = static_cast<int>(sprite.layer);
	if (ImGui::Combo("Render Layer", &layerIndex, k_renderLayerNames, IM_ARRAYSIZE(k_renderLayerNames)))
	{
		sprite.layer = static_cast<World::RenderLayer>(layerIndex);
	}
	ImGui::DragFloat("Order In Layer", &sprite.orderInLayer, 1.0f, -1000.0f, 1000.0f);
	if (sprite.layer == World::RenderLayer::Entities)
	{
		ImGui::TextDisabled("(world Y is added automatically for y-sort on this layer)");
	}

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

void InspectorWindow::RenderShaderComponent(GameContext& context, Component::Shader& shader, entt::registry& registry,
                                            entt::entity entity)
{
	// Preview button
	if (ImGui::Button("Preview Shader"))
	{
		if (m_previewWindow)
		{
			PreviewShader(m_previewWindow, &shader, "Shader Component");
		}
	}

	ImGui::Separator();

	ImGui::Text("Shader ID: %d", shader.shader->shader.idx);
	ImGui::Separator();

	// Float uniforms
	if (ImGui::TreeNode("Float Uniforms"))
	{
		if (shader.floatUniforms.empty())
		{
			ImGui::TextDisabled("No float uniforms");
		}
		else
		{
			for (auto& [name, value] : shader.floatUniforms)
			{
				ImGui::PushID(name.c_str());
				ImGui::DragFloat(name.c_str(), &value, 0.01f);
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}

	// Int uniforms
	if (ImGui::TreeNode("Int Uniforms"))
	{
		if (shader.intUniforms.empty())
		{
			ImGui::TextDisabled("No int uniforms");
		}
		else
		{
			for (auto& [name, value] : shader.intUniforms)
			{
				ImGui::PushID(name.c_str());
				ImGui::DragInt(name.c_str(), &value);
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}

	// Vec2 uniforms
	if (ImGui::TreeNode("Vector2 Uniforms"))
	{
		if (shader.vec2Uniforms.empty())
		{
			ImGui::TextDisabled("No vec2 uniforms");
		}
		else
		{
			for (auto& [name, value] : shader.vec2Uniforms)
			{
				ImGui::PushID(name.c_str());
				float v[2] = {value.x, value.y};
				if (ImGui::DragFloat2(name.c_str(), v, 0.01f))
				{
					value.x = v[0];
					value.y = v[1];
				}
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}

	// Vec3 uniforms
	if (ImGui::TreeNode("Vector3 Uniforms"))
	{
		if (shader.vec3Uniforms.empty())
		{
			ImGui::TextDisabled("No vec3 uniforms");
		}
		else
		{
			for (auto& [name, value] : shader.vec3Uniforms)
			{
				ImGui::PushID(name.c_str());
				float v[3] = {value.x, value.y, value.z};
				if (ImGui::DragFloat3(name.c_str(), v, 0.01f))
				{
					value.x = v[0];
					value.y = v[1];
					value.z = v[2];
				}
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}

	// Vec4 uniforms
	if (ImGui::TreeNode("Vector4 Uniforms"))
	{
		if (shader.vec4Uniforms.empty())
		{
			ImGui::TextDisabled("No vec4 uniforms");
		}
		else
		{
			for (auto& [name, value] : shader.vec4Uniforms)
			{
				ImGui::PushID(name.c_str());
				float v[4] = {value.x, value.y, value.z, value.w};
				if (ImGui::DragFloat4(name.c_str(), v, 0.01f))
				{
					value.x = v[0];
					value.y = v[1];
					value.z = v[2];
					value.w = v[3];
				}
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}

	// Matrix uniforms (read-only for now)
	if (ImGui::TreeNode("Matrix Uniforms"))
	{
		if (shader.matrixUniforms.empty())
		{
			ImGui::TextDisabled("No matrix uniforms");
		}
	}
}

// ====================================================================
// Helper functions for rendering common data types
// ====================================================================

bool InspectorWindow::RenderVec2(const char* label, glm::vec2& vec)
{
	float values[2] = {vec.x, vec.y};
	bool modified   = ImGui::DragFloat2(label, values, 0.1f);
	if (modified)
	{
		vec.x = values[0];
		vec.y = values[1];
	}
	return modified;
}

bool InspectorWindow::RenderVec3(const char* label, glm::vec3& vec)
{
	float values[3] = {vec.x, vec.y, vec.z};
	bool modified   = ImGui::DragFloat3(label, values, 0.1f);
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
	float values[4] = {vec.x, vec.y, vec.z, vec.w};
	bool modified   = ImGui::DragFloat4(label, values, 0.1f);
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
	float values[3] = {euler.x, euler.y, euler.z};
	bool modified   = ImGui::DragFloat3(label, values, 0.5f);
	if (modified)
	{
		euler.x = values[0];
		euler.y = values[1];
		euler.z = values[2];
		quat    = glm::quat(glm::radians(euler));
	}
	return modified;
}

bool InspectorWindow::RenderColor(const char* label, Util::Color& color)
{
	float col[4] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};

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

}  // namespace Struktur::Debug
