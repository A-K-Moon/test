/* This file is part of the ScriptDev2 Project. See AUTHORS file for Copyright information
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "TimerAI.h"
#include "Chat/Chat.h"
#include "Log.h"
#include <string>

Timer::Timer(uint32 id, std::function<void()> functor, uint32 timerMin, uint32 timerMax, bool disabled)
    : id(id), timer(urand(timerMin, timerMax)), disabled(disabled), functor(functor), initialMin(timerMin), initialMax(timerMax), initialDisabled(disabled)
    {}

bool Timer::UpdateTimer(const uint32 diff, bool /*combat*/)
{
    if (disabled)
        return false;

    if (timer <= diff)
    {
        timer = 0;
        disabled = true;
        return true;
    }
    else timer -= diff;

    return false;
}

void Timer::ResetTimer()
{
    timer = urand(initialMin, initialMax);
    disabled = initialDisabled;
}

bool CombatTimer::UpdateTimer(const uint32 diff, bool combat)
{
    if (disabled)
        return false;

    if (this->combat != combat)
        return false;

    if (timer <= diff)
    {
        timer = 0;
        disabled = true;
        return true;
    }
    else timer -= diff;

    return false;
}

void TimerManager::AddTimer(uint32 id, Timer&& timer)
{
    m_timers.emplace(id, timer);
}

void TimerManager::AddCustomAction(uint32 id, bool disabled, std::function<void()> functor, TimerCombat timerCombat)
{
    switch (timerCombat)
    {
        case TIMER_ALWAYS:
            m_timers.emplace(id, Timer(id, functor, 0, 0, disabled));
            break;
        case TIMER_COMBAT_OOC:
            m_timers.emplace(id, CombatTimer(id, functor, false, 0, 0, disabled));
            break;
        case TIMER_COMBAT_COMBAT:
            m_timers.emplace(id, CombatTimer(id, functor, true, 0, 0, disabled));
            break;
    }    
}

void TimerManager::AddCustomAction(uint32 id, uint32 timer, std::function<void()> functor, TimerCombat timerCombat)
{
    switch (timerCombat)
    {
        case TIMER_ALWAYS:
            m_timers.emplace(id, Timer(id, functor, timer, timer, false));
            break;
        case TIMER_COMBAT_OOC:
            m_timers.emplace(id, CombatTimer(id, functor, false, timer, timer, false));
            break;
        case TIMER_COMBAT_COMBAT:
            m_timers.emplace(id, CombatTimer(id, functor, true, timer, timer, false));
            break;
    }
}

void TimerManager::AddCustomAction(uint32 id, uint32 timerMin, uint32 timerMax, std::function<void()> functor, TimerCombat timerCombat)
{
    switch (timerCombat)
    {
        case TIMER_ALWAYS:
            m_timers.emplace(id, Timer(id, functor, timerMin, timerMax, false));
            break;
        case TIMER_COMBAT_OOC:
            m_timers.emplace(id, CombatTimer(id, functor, false, timerMin, timerMax, false));
            break;
        case TIMER_COMBAT_COMBAT:
            m_timers.emplace(id, CombatTimer(id, functor, true, timerMin, timerMax, false));
            break;
    }
}

void TimerManager::ResetTimer(uint32 index, uint32 timer)
{
    auto data = m_timers.find(index);
    if (data == m_timers.end())
    {
        sLog.outError("Timer index %u does not exist.", index);
        return;
    }
    (*data).second.timer = timer; (*data).second.disabled = false;
}

void TimerManager::DisableTimer(uint32 index)
{
    auto data = m_timers.find(index);
    if (data == m_timers.end())
    {
        sLog.outError("Timer index %u does not exist.", index);
        return;
    }
    (*data).second.timer = 0; (*data).second.disabled = true;
}

void TimerManager::ReduceTimer(uint32 index, uint32 timer)
{
    auto data = m_timers.find(index);
    if (data == m_timers.end())
    {
        sLog.outError("Timer index %u does not exist.", index);
        return;
    }
    (*data).second.timer = std::min((*data).second.timer, timer);
}

void TimerManager::DelayTimer(uint32 index, uint32 timer)
{
    auto data = m_timers.find(index);
    if (data == m_timers.end())
    {
        sLog.outError("Timer index %u does not exist.", index);
        return;
    }
    if (!(*data).second.disabled)
        (*data).second.timer = (*data).second.timer > timer ? (*data).second.timer : timer;
}

void TimerManager::ResetIfNotStarted(uint32 index, uint32 timer)
{
    auto data = m_timers.find(index);
    if (data == m_timers.end())
    {
        sLog.outError("Timer index %u does not exist.", index);
        return;
    }
    if ((*data).second.disabled)
    {
        (*data).second.timer = timer;
        (*data).second.disabled = false;
    }
}

void TimerManager::UpdateTimers(const uint32 diff)
{
    UpdateTimers(diff, false);
}

void TimerManager::UpdateTimers(const uint32 diff, bool combat)
{
    for (auto& data : m_timers)
    {
        Timer& timer = data.second;
        if (timer.UpdateTimer(diff, combat))
            timer.functor();
    }
}

void TimerManager::ResetAllTimers()
{
    for (auto& data : m_timers)
        data.second.ResetTimer();
}

void TimerManager::GetAIInformation(ChatHandler& reader)
{
    reader.PSendSysMessage("TimerAI: Timers:");
    std::string output = "";
    for (auto itr = m_timers.begin(); itr != m_timers.end(); ++itr)
    {
        Timer& timer = (*itr).second;
        output += "Timer ID: " + std::to_string(timer.id) + " Timer: " + std::to_string(timer.timer), +" Disabled: " + std::to_string(timer.disabled) + "\n";
    }
    reader.PSendSysMessage("%s", output.data());
}

void CombatActions::UpdateTimers(const uint32 diff, bool combat)
{
    TimerManager::UpdateTimers(diff, combat);
    for (auto& data : m_CombatActions)
    {
        CombatTimer& timer = data.second;
        if (timer.UpdateTimer(diff, combat))
            timer.functor();
    }
}

void CombatActions::ResetAllTimers()
{
    for (uint32 i = 0; i < m_actionReadyStatus.size(); ++i)
    {
        auto itr = m_timerlessActionSettings.find(i);
        if (itr == m_timerlessActionSettings.end())
            m_actionReadyStatus[i] = false;
        else
            m_actionReadyStatus[i] = (*itr).second;
    }
    for (auto& data : m_CombatActions)
        data.second.ResetTimer();
    TimerManager::ResetAllTimers();
}

void CombatActions::AddCombatAction(uint32 id, bool disabled)
{
    m_CombatActions.emplace(id, CombatTimer(id, [&, id] { m_actionReadyStatus[id] = true; }, true, 0, 0, disabled));
    m_actionReadyStatus[id] = !disabled;
}

void CombatActions::AddCombatAction(uint32 id, uint32 timer)
{
    m_CombatActions.emplace(id, CombatTimer(id, [&, id] { m_actionReadyStatus[id] = true; }, true, timer, timer, false));
    m_actionReadyStatus[id] = false;
}

void CombatActions::AddCombatAction(uint32 id, uint32 timerMin, uint32 timerMax)
{
    m_CombatActions.emplace(id, CombatTimer(id, [&, id] { m_actionReadyStatus[id] = true; }, true, timerMin, timerMax, false));
    m_actionReadyStatus[id] = false;
}

void CombatActions::AddTimerlessCombatAction(uint32 id, bool byDefault)
{
    m_timerlessActionSettings[id] = byDefault;
    m_actionReadyStatus[id] = byDefault;
}

void CombatActions::ResetTimer(uint32 index, uint32 timer)
{
    auto data = m_CombatActions.find(index);
    if (data == m_CombatActions.end())
        TimerManager::ResetTimer(index, timer);
    else
    {
        (*data).second.timer = timer;
        (*data).second.disabled = false;
    }
}

void CombatActions::DisableTimer(uint32 index)
{
    auto data = m_CombatActions.find(index);
    if (data == m_CombatActions.end())
        TimerManager::DisableTimer(index);
    else
    {
        (*data).second.timer = 0;
        (*data).second.disabled = true;
    }
}

void CombatActions::ReduceTimer(uint32 index, uint32 timer)
{
    auto data = m_CombatActions.find(index);
    if (data == m_CombatActions.end())
        TimerManager::ReduceTimer(index, timer);
    else
        (*data).second.timer = std::min((*data).second.timer, timer);
}

void CombatActions::DelayTimer(uint32 index, uint32 timer)
{
    auto data = m_CombatActions.find(index);
    if (data == m_CombatActions.end())
        TimerManager::DelayTimer(index, timer);
    else if (!(*data).second.disabled)
        (*data).second.timer = (*data).second.timer > timer ? (*data).second.timer : timer;
}

void CombatActions::ResetIfNotStarted(uint32 index, uint32 timer)
{
    auto data = m_CombatActions.find(index);
    if (data == m_CombatActions.end())
        TimerManager::ResetIfNotStarted(index, timer);
    else if ((*data).second.disabled)
    {
        (*data).second.timer = timer;
        (*data).second.disabled = false;
    }
}

void CombatActions::DisableCombatAction(uint32 index)
{
    if (m_timerlessActionSettings.find(index) == m_timerlessActionSettings.end())
        DisableTimer(index);
    SetActionReadyStatus(index, false);
}

void CombatActions::GetAIInformation(ChatHandler& reader)
{
    reader.PSendSysMessage("Combat Timers:");
    std::string output = "";
    for (auto itr = m_CombatActions.begin(); itr != m_CombatActions.end(); ++itr)
    {
        Timer& timer = (*itr).second;
        output += "Timer ID: " + std::to_string(timer.id) + " Timer: " + std::to_string(timer.timer), +" Disabled: " + std::to_string(timer.disabled) + "\n";
    }
    reader.PSendSysMessage("%s", output.data());
}