/**
 * @file canvas.h
 * OpenGL drawing surface. @ingroup gl
 *
 * @authors Copyright (c) 2012 Jaakko Keränen <jaakko.keranen@iki.fi>
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
 * General Public License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA</small>
 */

#ifndef LIBDENG_CANVAS_H
#define LIBDENG_CANVAS_H

#include <QGLWidget>

/**
 * Drawing canvas with an OpenGL context and window surface. Each CanvasWindow
 * creates one Canvas instance on which to draw.
 * @ingroup gl
 */
class Canvas : public QGLWidget
{
    Q_OBJECT

public:
    explicit Canvas(QWidget *parent = 0);
    ~Canvas();
    
    /**
     * Sets a callback function that will be called when the canvas is ready
     * for GL initialization. The OpenGL context and drawing surface are not
     * ready to be used before that.
     *
     * @param canvasInitializeFunc  Callback.
     */
    void setInitCallback(void (*canvasInitializeFunc)(Canvas&));

    void setDrawCallback(void (*canvasDrawFunc)(Canvas&));

    void forcePaint();

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void showEvent(QShowEvent*);

protected slots:
    void notifyInit();

private:
    struct Instance;
    Instance* d;
};

#endif // LIBDENG_CANVAS_H
