/******************************************************************************

    GPLv3 License
    Copyright (c) 2023 Aria Janke

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*****************************************************************************/

#include "Tasks.hpp"

/* static */ const BackgroundTaskCompletion
    BackgroundTaskCompletion::k_finished =
    BackgroundTaskCompletion{Status::finished, nullptr};

/* static */ const BackgroundTaskCompletion
    BackgroundTaskCompletion::k_in_progress =
    BackgroundTaskCompletion{Status::in_progress, nullptr};

bool BackgroundTaskCompletion::operator ==
    (const BackgroundTaskCompletion & rhs) const
{
    if (m_status == Status::delay_until)
        { return m_delay_task == rhs.m_delay_task; }
    return m_status == rhs.m_status;
}

SharedPtr<BackgroundDelayTask>
    BackgroundTaskCompletion::move_out_delay_task()
{
    if (m_delay_task) {
        m_status = Status::invalid;
        return std::move(m_delay_task);
    }
    return nullptr;
}

BackgroundTaskCompletion::BackgroundTaskCompletion
    (Status status_,
     SharedPtr<BackgroundDelayTask> && delay_task_):
    m_status(status_),
    m_delay_task(std::move(delay_task_)) {}
