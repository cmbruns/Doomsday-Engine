/** @file commandwidget.h  Abstract command line based widget.
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

#include "ui/widgets/commandwidget.h"
#include "ui/widgets/documentwidget.h"
#include "ui/widgets/popupwidget.h"
#include "ui/style.h"
#include "clientapp.h"

#include <de/shell/EditorHistory>
#include <de/KeyEvent>
#include <de/App>

using namespace de;

DENG_GUI_PIMPL(CommandWidget)
{
    shell::EditorHistory history;
    DocumentWidget *completions;
    PopupWidget *popup; ///< Popup for autocompletions.
    bool allowReshow;   ///< Contents must still be valid.

    Instance(Public *i) : Base(i), history(i), allowReshow(false)
    {
        // Popup for autocompletions.
        completions = new DocumentWidget;
        completions->setMaximumLineWidth(640);
        completions->setScrollBarColor("inverted.accent");

        popup = new PopupWidget;
        popup->useInfoStyle();
        popup->setContent(completions);

        // Height for the content: depends on the document height (plus margins), but at
        // most 400; never extend outside the view, though.
        completions->rule().setInput(Rule::Height,
                OperatorRule::minimum(
                    OperatorRule::minimum(style().rules().rule("editor.completion.height"),
                                          completions->contentRule().height() +
                                          completions->margins().height()),
                    self.rule().top() - style().rules().rule("gap")));

        self.add(popup);
    }
};

CommandWidget::CommandWidget(String const &name)
    : LineEditWidget(name), d(new Instance(this))
{}

PopupWidget &CommandWidget::autocompletionPopup()
{
    return *d->popup;
}

void CommandWidget::focusGained()
{
    LineEditWidget::focusGained();
    emit gotFocus();
}

void CommandWidget::focusLost()
{
    LineEditWidget::focusLost();

    // Get rid of the autocompletion popup.
    closeAutocompletionPopup();

    emit lostFocus();
}

bool CommandWidget::handleEvent(Event const &event)
{
    if(hasFocus() && event.isKeyDown())
    {
        KeyEvent const &key = event.as<KeyEvent>();

        if(d->allowReshow &&
           isSuggestingCompletion() &&
           key.qtKey() == Qt::Key_Tab && !d->popup->isOpen() &&
           suggestedCompletions().size() > 1)
        {
            // The completion popup has been manually dismissed, but the editor is
            // still in autocompletion mode. Let's just reopen the popup with its
            // old content.
            d->popup->open();
            return true;
        }

        // Override the handling of the Enter key.
        if(key.qtKey() == Qt::Key_Return || key.qtKey() == Qt::Key_Enter)
        {
            if(isAcceptedAsCommand(text()))
            {
                // We must make sure that the ongoing autocompletion ends.
                acceptCompletion();

                String const entered = d->history.enter();
                executeCommand(entered);
                emit commandEntered(entered);
            }
            return true;
        }
    }

    if(LineEditWidget::handleEvent(event))
    {
        // Editor handled the event normally.
        return true;
    }

    if(hasFocus())
    {
        // All Tab keys are eaten by a focused console command widget.
        if(event.isKey() && event.as<KeyEvent>().ddKey() == DDKEY_TAB)
        {
            return true;
        }

        if(event.isKeyDown())
        {
            // Fall back to history navigation.
            return d->history.handleControlKey(event.as<KeyEvent>().qtKey());
        }
    }
    return false;
}

void CommandWidget::dismissContentToHistory()
{
    d->history.goToLatest();

    if(!text().isEmpty())
    {
        d->history.enter();
    }
}

void CommandWidget::closeAutocompletionPopup()
{
    d->popup->close();
    d->allowReshow = false;
}

void CommandWidget::showAutocompletionPopup(String const &completionsText)
{
    d->completions->setText(completionsText);
    d->completions->scrollToTop(0);

    d->popup->setAnchorX(cursorRect().middle().x);
    d->popup->setAnchorY(rule().top());
    d->popup->open();

    d->allowReshow = true;
}

void CommandWidget::autoCompletionEnded(bool accepted)
{
    LineEditWidget::autoCompletionEnded(accepted);
    closeAutocompletionPopup();
}
