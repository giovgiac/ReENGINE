/*
 * World.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Entity.hpp"
#include "Core/Result.hpp"
#include "Graphics/Renderer.hpp"
#include "Entities/Camera.hpp"
#include "Entities/DirectionalLight.hpp"
#include "Entities/PointLight.hpp"
#include "Entities/SpotLight.hpp"

#include "Platform/Win32/Window.hpp"
#include "Platform/Win32/Timer.hpp"

#include <boost/container/map.hpp>
#include <boost/container/vector.hpp>
#include <boost/function.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/type_traits/is_base_of.hpp>

namespace Re
{
	namespace Core
	{
		enum class WorldResult
		{
			Success = 0,
			Failure = 1
		};

		class World
		{
		private:
			void AddEntity(const boost::shared_ptr<Core::Entity>& entity);
			void AddEntity(const boost::shared_ptr<Entities::Camera>& camera);
			void AddEntity(const boost::shared_ptr<Entities::DirectionalLight>& light);
			void AddEntity(const boost::shared_ptr<Entities::PointLight>& light);
			void AddEntity(const boost::shared_ptr<Entities::SpotLight>& light);

			void DispatchToRenderer();
			void JoinDispatchThreads();

		public:
			World();

			WorldResult Startup();
			void Shutdown();
			void Loop();
			void Update();

			INLINE const Graphics::Renderer& GetRenderer() const { return _renderer; }
			INLINE Platform::Win32Window& GetWindow() { return _window; }

			template <typename EntityType, typename... EntityArgs>
			boost::shared_ptr<EntityType> SpawnEntity(EntityArgs&&... args)
			{
				static_assert(boost::is_base_of<Entity, EntityType>::value, "EntityType passed for SpawnEntity does not inherit from Entity.");

				auto newEntity = boost::shared_ptr<EntityType>(new EntityType(std::forward<EntityArgs>(args)...));
				_entities.emplace(&typeid(EntityType), newEntity);
				newEntity->_owner = this;
				newEntity->Initialize();
				AddEntity(newEntity);
				return newEntity;
			}

			template <typename EntityType>
			boost::container::vector<boost::weak_ptr<EntityType>> GetEntities()
			{
				typedef boost::container::multimap<const std::type_info*, boost::shared_ptr<Entity>>::value_type value_type;
				boost::function<boost::weak_ptr<EntityType>(value_type&)> f = boost::bind(&value_type::second, _1);
				return boost::container::vector<boost::weak_ptr<EntityType>>(
					boost::make_transform_iterator(_entities.lower_bound(&typeid(EntityType)), f),
					boost::make_transform_iterator(_entities.upper_bound(&typeid(EntityType)), f)
				);
			}

		private:
			boost::lockfree::queue<Entity*> _dispatchQueue;
			boost::thread _dispatchThread;
			boost::mutex _dispatchThreadMutex;
			boost::condition_variable _shouldDispatch;
			bool _dispatchThreadShouldClose;

			boost::container::multimap<const std::type_info*, boost::shared_ptr<Entity>> _entities;
			
			Graphics::Renderer _renderer;
			Platform::Win32Window _window;
			Platform::Win32Timer _timer;

		};
	}
}
