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

#include "Platform/Win32/Window.hpp"
#include "Platform/Win32/Timer.hpp"

#include <boost/container/map.hpp>
#include <boost/container/vector.hpp>
#include <boost/function.hpp>
#include <boost/iterator/transform_iterator.hpp>
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
			struct DispatchThreadData
			{
				usize _index;
				bool _shouldClose;
				bool _shouldRender;
				boost::thread _handle;

				DispatchThreadData()
					: _index(-1), _shouldClose(false), _shouldRender(false) {}
			};

		public:
			World();

			WorldResult Startup();
			void Shutdown();
			void Render();
			void Loop();

			template <typename EntityType, typename... EntityArgs>
			EntityType& AddEntity(EntityArgs&&... args)
			{
				static_assert(boost::is_base_of<Entity, EntityType>::value, "EntityType passed for AddEntity does not inherit from Entity.");

				auto newEntity = boost::make_shared<EntityType>(std::forward<EntityArgs>(args)...);
				_entities.emplace(&typeid(EntityType), newEntity);
				newEntity->_owner = this;
				newEntity->Initialize();
				return *newEntity;
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
			void DispatchToRenderer(usize threadIndex);
			void JoinDispatchThreads();

		private:
			boost::container::multimap<const std::type_info*, boost::shared_ptr<Entity>> _entities;
			boost::container::vector<DispatchThreadData> _dispatchThreadsData;
			boost::container::vector<boost::mutex> _dispatchThreadsMutex;
			boost::condition_variable _shouldDispatch;
			
			Graphics::Renderer _renderer;
			Platform::Win32Window _window;
			Platform::Win32Timer _timer;
		};
	}
}
