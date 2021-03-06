/** @file windowtransform.cpp Base class for window content transformation.
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

#include "ui/windowtransform.h"
#include "ui/clientwindow.h"

using namespace de;

DENG2_PIMPL_NOREF(WindowTransform)
{
    ClientWindow *win;
};

WindowTransform::WindowTransform(ClientWindow &window)
    : d(new Instance)
{
    d->win = &window;
}

ClientWindow &WindowTransform::window() const
{
    return *d->win;
}

void WindowTransform::glInit()
{
    // nothing to do
}

void WindowTransform::glDeinit()
{
    // nothing to do
}

Vector2ui WindowTransform::logicalRootSize(Vector2ui const &physicalCanvasSize) const
{
    return physicalCanvasSize;
}

Vector2f WindowTransform::windowToLogicalCoords(de::Vector2i const &pos) const
{
    return pos;
}

void WindowTransform::drawTransformed()
{
    return d->win->root().draw();
}
