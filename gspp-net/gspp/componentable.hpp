//
//  componentable.hpp
//  gspp-net
//
//  Created by Nikita Ivanov on 24.09.2020.
//  Copyright � 2020 osdever. All rights reserved.
//

#pragma once
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace gspp
{
	class Componentable
	{
	public:
		Componentable() = default;

		template<class T>
		class ComponentRef
		{
		public:
			ComponentRef(std::nullptr_t)
				: _ptr(nullptr)
			{}

			ComponentRef(T& ref)
				: _ptr(&ref)
			{}

			ComponentRef(T* ptr)
				: _ptr(ptr)
			{}

			T& operator*()
			{
				return *_ptr;
			}

			operator bool() const
			{
				return _ptr != nullptr;
			}

			const T& operator*() const
			{
				return *_ptr;
			}

			T* operator->()
			{
				return _ptr;
			}

			const T* operator->() const
			{
				return _ptr;
			}

		private:
			T* _ptr;
		};

		template<class T>
		void CreateComponent(std::shared_ptr<T> from)
		{
			static std::type_index type = typeid(T);
			std::lock_guard lg{ _componentsLock };
			_components[type] = from;
		}

		template<class T>
		void CreateComponent(const T& from)
		{
			CreateComponent<T>(std::make_shared<T>(from));
		}

		template<class T>
		void CreateComponent()
		{
			CreateComponent<T>(T{});
		}

		template<class T>
		ComponentRef<T> GetComponent()
		{
			static std::type_index type = typeid(T);
			std::lock_guard lg{ _componentsLock };

			auto it = _components.find(type);
			if (it == _components.end())
				return ComponentRef<T>{ nullptr };

			auto p = (T*)it->second.get();
			return ComponentRef<T>(p);
		}

		template<class T>
		ComponentRef<T> GetExistingComponent()
		{
			static std::type_index type = typeid(T);

			auto component = GetComponent<T>();
			if (component)
				return component;
			else
				throw std::runtime_error(std::string("Componentable: Component ") + type.name() + " not found");
		}

		std::vector<std::type_index> GetCurrentComponentTypes()
		{
			std::lock_guard lg{ _componentsLock };

			std::vector<std::type_index> result;
			for (auto& [type, data] : _components)
				result.push_back(type);

			return result;
		}

		template<class T>
		bool HasComponent()
		{
			static std::type_index type = typeid(T);
			std::lock_guard lg{ _componentsLock };
			return _components.find(type) != _components.end() && _components[type] != nullptr;
		}

		template<class T>
		bool DestroyComponent()
		{
			static std::type_index type = typeid(T);
			std::lock_guard lg{ _componentsLock };

			return _components.erase(type);
		}

	private:
		std::unordered_map<std::type_index, std::shared_ptr<void>> _components;
		std::mutex _componentsLock;
	};
}
