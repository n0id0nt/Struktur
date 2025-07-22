#pragma once
#include <string>

namespace Struktur
{
	namespace Core
	{
        class ResourcePool;

		class IResource
		{
		public:
			IResource(ResourcePool* resourcePool, const std::string& path) 
				: m_resoursePool(resoursePool), m_path(path) {}
		
            virtual bool IsLoaded() const = 0

		private:
			ResourcePool* m_resoursePool;
			std::string m_path;
		};
	}
}
