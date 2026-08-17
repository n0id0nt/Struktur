#pragma once

#include <string>

#include "entt/entt.hpp"

namespace Struktur
{
class GameContext;

namespace System
{
class GameObjectManager
{
public:
	GameObjectManager() {}
	~GameObjectManager() {}

	void CreateObjectCallBack(GameContext& context);
	void Shutdown(GameContext& context);

	entt::entity CreateGameObject(GameContext& context, const std::string& identifier,
	                              entt::entity parent = entt::null);

	void SafeDeleteGameObject(GameContext& context, entt::entity entity);
	void DeleteGameObjectsInSafeToDeleteQueue(GameContext& context);
	void UpdateGameObjectsActiveStateQueue(GameContext& context);
	void DestroyGameObject(GameContext& context, entt::entity entity);

private:
	void OnChildrenDestroy(entt::registry& reg, entt::entity entity);
	void OnPhysicsBodyDestory(entt::registry& reg, entt::entity entity);
	void OnScriptConstruct(entt::registry& reg, entt::entity entity);
	void OnScriptDestory(entt::registry& reg, entt::entity entity);
	void OnActiveStateChanged(entt::registry& reg, entt::entity entity);

	GameContext* m_context = nullptr;
	std::vector<entt::entity> m_queueOfObjectsToSafeDelete;
	std::vector<entt::entity> m_queueOfObjectsToUpdateActiveState;
};
}  // namespace System
}  // namespace Struktur
