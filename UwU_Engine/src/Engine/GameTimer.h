#pragma once
// High-precision game timer built on std::chrono::steady_clock.
//
// Two usage modes:
//
//   1. Variable timestep (default):
//        timer.Tick();
//        float dt = timer.DeltaTime();   // seconds since last Tick()
//
//   2. Fixed timestep accumulator:
//        timer.Tick();
//        while (timer.ConsumeFixedStep())   // true each time a full step is ready
//            UpdatePhysics(timer.FixedStep());
//        RenderGame(timer.Alpha());         // interpolation factor [0,1]
//
// The timer starts paused; call Reset() then Resume() (or just Reset())
// before the main loop.

#include "Core.h"

namespace UwU_Engine
{

    class UWU_API GameTimer
    {
    public:
        using Clock = std::chrono::steady_clock;
        using TimePoint = Clock::time_point;
        using Duration = std::chrono::duration<double>;  // seconds as double

        // Fixed physics step in seconds (default: 1/60 s).
        void SetFixedStep(double seconds) { m_fixedStep = seconds; }
        double FixedStep() const { return m_fixedStep; }

        // Cap the maximum delta processed per frame to avoid the "spiral of death"
        // when the app freezes or is debugged. Default: 0.25 s.
        void SetMaxDelta(double seconds) { m_maxDelta = seconds; }

        // Reset all state and start running.
        void Reset();

        // Pause / resume without resetting totals.
        void Pause();
        void Resume();
        bool IsPaused() const { return m_paused; }

        // Call exactly once per frame (before Update / Render).
        void Tick();

        // Seconds elapsed since the previous Tick(). Zero while paused.
        float  DeltaTime()  const { return static_cast<float>(m_delta); }
        double DeltaTimeD() const { return m_delta; }

        // Total elapsed seconds while not paused.
        double TotalTime()  const { return m_total; }

        // Wall-clock frames per second (smoothed over last frame).
        float  FPS()        const { return m_delta > 0.0 ? static_cast<float>(1.0 / m_delta) : 0.0f; }

        // Call in a while loop. Returns true each time a full fixed step is
        // ready to be consumed. Decrement the accumulator each call.
        bool ConsumeFixedStep();

        // Leftover fraction [0, 1] for render interpolation.
        float Alpha() const
        {
            return (m_fixedStep > 0.0)
                ? static_cast<float>(m_accumulator / m_fixedStep)
                : 0.0f;
        }

    private:
        TimePoint m_baseTime{};         // time of Reset()
        TimePoint m_prevTime{};         // time of the previous Tick()
        TimePoint m_pauseStart{};

        double m_delta = 0.0;     // variable dt for this frame
        double m_total = 0.0;     // accumulated unpaused time
        double m_pausedTotal = 0.0;     // total time spent paused
        double m_accumulator = 0.0;     // for fixed timestep
        double m_fixedStep = 1.0 / 60.0;
        double m_maxDelta = 0.25;

        bool   m_paused = false;
    };

} 
