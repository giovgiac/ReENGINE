/*
 * Timer.hpp
 *
 * This header file declares the class that's responsible for
 * keeping time in Win32-based operating systems.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Platform/Timer.hpp"

namespace Re 
{
	namespace Platform 
	{
		/*
		 * Win32Timer Class
		 *
		 * This class is responsible for keeping time in Win32-based
		 * operating systems.
		 *
		 */
		class Win32Timer : public ITimer {
		private:
			f64		_secsPerCount;
			f64		_deltaTime;
			i64		_baseTime;
			i64		_curTime;
			i64		_pausedTime;
			i64		_prevTime;
			i64		_stopTime;
			bool	_stopped;

		public:
			/*
			 * Win32Timer Constructor
			 *
			 * This constructor initializes the times to their default values
			 * and calculates the period and frequency of the timer.
			 *
			 */
			Win32Timer();

			/*
			 * Win32Timer Reset Method
			 *
			 * This method resets the timer, resetting it's baseTime to curTime.
			 *
			 * return: nothing.
			 *
			 */
			void Reset();

			/*
			 * Win32Timer Start Method
			 *
			 * This method starts the timer, if it was previously stopped, counting
			 * time normally, instead of to pausedTime.
			 *
			 * return: nothing.
			 *
			 */
			void Start();

			/*
			 * Win32Timer Stop Method
			 *
			 * This method stops the timer, adding future time differences to the
			 * pausedTime.
			 *
			 * return: nothing.
			 *
			 */
			void Stop();

			/*
			 * Win32Timer Tick Method
			 *
			 * This method is supposed to be called every frame and is responsible
			 * for calculating the deltaTime between frames.
			 *
			 * return: nothing.
			 *
			 */
			void Tick();

			/*
			 * Win32Timer ElapsedTime Method
			 *
			 * This method calculates the total elapsed time, since the start of the
			 * application or the last timer reset.
			 *
			 * return: a float with the total time elapsed in seconds.
			 *
			 */
			f32 ElapsedTime() const;

			/*
			 * Win32Timer DeltaTime Method
			 *
			 * This method is a getter for the deltaTime between the current and
			 * previous frames.
			 *
			 * return: a float with the deltaTime in seconds.
			 *
			 */
			INLINE f32 DeltaTime() const { return (f32)_deltaTime; }
		};
	}
}
