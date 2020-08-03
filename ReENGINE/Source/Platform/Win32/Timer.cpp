/*
 * Timer.cpp
 *
 * This source file defines all of the Win32Timer methods and constructors
 * declared in the Win32Timer.hpp header file.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Timer.hpp"

namespace Re 
{
	namespace Platform
	{
		Win32Timer::Win32Timer()
			: _baseTime(0), _curTime(0), _deltaTime(-1.0), _pausedTime(0), _prevTime(0), _stopTime(0), _stopped(false)
		{
			// Variable to Hold Frequency
			i64 countsPerSec;

			// Calculate Frequency and Period
			QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
			_secsPerCount = 1.0 / (f64)countsPerSec;
		}

		void Win32Timer::Reset() {
			// Get Current Time
			QueryPerformanceCounter((LARGE_INTEGER*)&_curTime);

			// Set Times
			_baseTime = _curTime;
			_prevTime = _curTime;
			_stopTime = 0;
			_stopped = false;
		}

		void Win32Timer::Start() {
			i64 startTime;

			// Get Current Time
			QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

			if (_stopped) {
				// Set Paused Time
				_pausedTime += startTime - _stopTime;

				// Set Previous Time
				_prevTime = startTime;

				// Reset Stop Time and Stopped
				_stopTime = 0;
				_stopped = false;
			}
		}

		void Win32Timer::Stop() {
			if (!_stopped) {
				// Get Current Time
				QueryPerformanceCounter((LARGE_INTEGER*)&_curTime);

				// Set Stop Time and Stopped
				_stopTime = _curTime;
				_stopped = true;
			}
		}

		void Win32Timer::Tick() {
			if (_stopped) {
				_deltaTime = 0.0;
				return;
			}

			// Get Current Time
			QueryPerformanceCounter((LARGE_INTEGER*)&_curTime);

			// Compute Delta Time
			_deltaTime = (_curTime - _prevTime) * _secsPerCount;

			// Set Previous Time
			_prevTime = _curTime;

			// Ensure non-negative _deltaTime
			if (_deltaTime < 0.0)
				_deltaTime = 0.0;
		}

		f32 Win32Timer::ElapsedTime() const {
			if (_stopped)
				return (f32)(((_stopTime - _pausedTime) - _baseTime) * _secsPerCount);
			else
				return (f32)(((_curTime - _pausedTime) - _baseTime) * _secsPerCount);
		}
	}
}
