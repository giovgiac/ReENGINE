/*
 * Timer.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"

namespace Re
{
	namespace Platform 
	{
		/**
		 * @brief This interface is responsible for standardizing the implementations of timers
		 * across multiple operating systems.
		 *
		 */
		struct ITimer
		{
            /**
             * @brief A virtual destructor for the interface, so that it can be inherited
             * safely and in a well-defined manner.
             *
             */
            virtual ~ITimer() = default;

			/**
			 * @brief This method resets the timer, returning it back to zero time.
			 *
			 */
			virtual void Reset() = 0;

			/**
			 * @brief This method starts the timer, if it was previously stopped, counting
			 * time normally.
			 *
			 */
			virtual void Start() = 0;

			/**
			 * @brief This method stops the timer and starts counting on paused time.
			 *
			 */
			virtual void Stop() = 0;

			/**
			 * @brief This method is supposed to be called every frame and is responsible
			 * for calculating the deltaTime between frames.
			 *
			 */
			virtual void Tick() = 0;

			/**
			 * @brief This method calculates the total elapsed time, since the start of the
			 * application or the last timer reset.
			 *
			 * @return a float with the total elapsed time in seconds.
			 *
			 */
			virtual f32 ElapsedTime() const = 0;

			/**
			 * @brief This method is a getter for the deltaTime between the current and
			 * previous frames.
			 *
			 * @return a float with the deltaTime in seconds.
			 *
			 */
			virtual f32 DeltaTime() const = 0;
		};
	}
}
