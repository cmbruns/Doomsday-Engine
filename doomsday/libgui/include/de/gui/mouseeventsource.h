/** @file mouseeventsource.h  Object that produces mouse events.
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

#ifndef LIBGUI_MOUSEEVENTSOURCE_H
#define LIBGUI_MOUSEEVENTSOURCE_H

#include "libgui.h"
#include "mouseevent.h"
#include <de/Vector>
#include <de/Observers>

namespace de {

/**
 * Object that produces mouse events.
 */
class LIBGUI_PUBLIC MouseEventSource
{
public:
    enum State
    {
        Untrapped,
        Trapped
    };

    DENG2_DEFINE_AUDIENCE(MouseStateChange, void mouseStateChanged(State))

    DENG2_DEFINE_AUDIENCE(MouseEvent,       void mouseEvent(MouseEvent const &))

public:
    virtual ~MouseEventSource() {}
};

} // namespace de

#endif // LIBGUI_MOUSEEVENTSOURCE_H
