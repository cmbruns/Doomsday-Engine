/** @file task.cpp  Concurrent task.
 *
 * @authors Copyright (c) 2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 *
 * @par License
 * GPL: http://www.gnu.org/licenses/gpl.html
 *
 * <small>This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version. This program is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details. You should have received a copy of the GNU
 * General Public License along with this program; if not, see:
 * http://www.gnu.org/licenses</small>
 */

#include "de/Task"
#include "de/TaskPool"
#include "de/Log"

namespace de {

Task::Task() : _pool(0)
{}

TaskPool &Task::pool() const
{
    DENG2_ASSERT(_pool != 0);
    return *_pool;
}

void Task::run()
{
    try
    {
        runTask();
    }
    catch(Error const &er)
    {
        LOG_AS("Task");
        LOG_WARNING("Aborted due to exception: ") << er.asText();
    }

    // Cleanup.
    if(_pool) _pool->taskFinished(*this);
    Log::disposeThreadLog();
}

} // namespace de
