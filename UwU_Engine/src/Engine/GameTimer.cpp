#include "GameTimer.h"
#include"uwupch.h"

namespace UwU_Engine
{

    void GameTimer::Reset()
    {
        m_baseTime = Clock::now();
        m_prevTime = m_baseTime;
        m_delta = 0.0;
        m_total = 0.0;
        m_pausedTotal = 0.0;
        m_accumulator = 0.0;
        m_paused = false;
    }

    void GameTimer::Pause()
    {
        if (!m_paused)
        {
            m_pauseStart = Clock::now();
            m_paused = true;
        }
    }

    void GameTimer::Resume()
    {
        if (m_paused)
        {
            // Discount the time spent paused so TotalTime() stays accurate.
            auto pausedFor = Duration(Clock::now() - m_pauseStart).count();
            m_pausedTotal += pausedFor;
            m_prevTime = Clock::now();  // don't count pause as a huge dt spike
            m_paused = false;
        }
    }

    void GameTimer::Tick()
    {
        if (m_paused)
        {
            m_delta = 0.0;
            return;
        }

        TimePoint now = Clock::now();

        // Raw delta in seconds
        double raw = Duration(now - m_prevTime).count();

        // Clamp to prevent spiral-of-death when a breakpoint is hit or the
        // window is dragged for several seconds.
        m_delta = (std::min)(raw, m_maxDelta);

        m_prevTime = now;
        m_total += m_delta;
        m_accumulator += m_delta;
    }

    bool GameTimer::ConsumeFixedStep()
    {
        if (m_accumulator >= m_fixedStep)
        {
            m_accumulator -= m_fixedStep;
            return true;
        }
        return false;
    }

}
