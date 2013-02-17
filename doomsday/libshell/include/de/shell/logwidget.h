/** @file logwidget.h  Widget for output message log.
 *
 * @authors Copyright © 2013 Jaakko Keränen <jaakko.keranen@iki.fi>
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

#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <de/shell/TextWidget>
#include <de/LogSink>

namespace de {
namespace shell {

class LIBSHELL_PUBLIC LogWidget : public TextWidget
{
    Q_OBJECT

public:
    LogWidget(String const &name = "");
    virtual ~LogWidget();

    /**
     * Returns the log sink that can be connected to a log buffer for receiving
     * log entries into the widget's buffer.
     */
    LogSink &logSink();

    void draw();
    bool handleEvent(Event const &event);

public slots:
    /**
     * Moves the scroll offset of the widget to the bottom of the history.
     */
    void scrollToBottom();

private:
    struct Instance;
    Instance *d;
};

} // namespace shell
} // namespace de

#endif // LOGWIDGET_H