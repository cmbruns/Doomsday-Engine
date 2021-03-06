/** @file choicewidget.cpp  Widget for choosing from a set of alternatives.
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

#include "ui/widgets/choicewidget.h"
#include "ui/widgets/popupmenuwidget.h"
#include "SignalAction"

using namespace de;
using namespace ui;

DENG_GUI_PIMPL(ChoiceWidget),
DENG2_OBSERVES(Data, Addition),
DENG2_OBSERVES(Data, Removal),
DENG2_OBSERVES(Data, OrderChange),
DENG2_OBSERVES(ChildWidgetOrganizer, WidgetCreation),
DENG2_OBSERVES(ChildWidgetOrganizer, WidgetUpdate)
{
    /**
     * Items in the choice's popup uses this as action to change the selected
     * item.
     */
    struct SelectAction : public de::Action
    {
        Instance *d;
        ui::Item const &selItem;

        SelectAction(Instance *inst, ui::Item const &item) : d(inst), selItem(item) {}

        void trigger()
        {
            Action::trigger();
            d->selected = d->items().find(selItem);
            d->updateButtonWithSelection();
            d->updateItemHighlight();
            d->choices->dismiss();

            emit d->self.selectionChangedByUser(d->selected);
        }

        Action *duplicate() const
        {
            DENG2_ASSERT(false); // not needed
            return 0;
        }
    };

    PopupMenuWidget *choices;
    Data::Pos selected; ///< One item is always selected.

    Instance(Public *i) : Base(i), selected(Data::InvalidPos)
    {
        self.setFont("choice.selected");

        choices = new PopupMenuWidget;
        choices->items().audienceForAddition += this;
        choices->items().audienceForRemoval += this;
        choices->items().audienceForOrderChange += this;
        choices->menu().organizer().audienceForWidgetCreation += this;
        choices->menu().organizer().audienceForWidgetUpdate += this;
        self.add(choices);

        self.setAction(new SignalAction(thisPublic, SLOT(openPopup())));

        updateButtonWithSelection();
        updateStyle();
    }

    void updateStyle()
    {
        // Popup background color.
        choices->set(choices->background().withSolidFill(style().colors().colorf("choice.popup")));
    }

    void widgetCreatedForItem(GuiWidget &widget, ui::Item const &item)
    {
        if(ButtonWidget *but = widget.maybeAs<ButtonWidget>())
        {
            // Make sure the created buttons have an action that updates the
            // selected item.
            but->setAction(new SelectAction(this, item));
        }
    }

    void widgetUpdatedForItem(GuiWidget &, ui::Item const &item)
    {
        if(isValidSelection() && &item == &self.selectedItem())
        {
            // Make sure the button is up to date, too.
            updateButtonWithItem(self.selectedItem());
        }
    }

    Data const &items() const
    {
        return choices->items();
    }

    bool isValidSelection() const
    {
        return selected < items().size();
    }

    void contextItemAdded(Data::Pos id, ui::Item const &)
    {
        if(selected >= items().size())
        {
            // If the previous selection was invalid, make a valid one now.
            selected = 0;

            updateButtonWithSelection();
            return;
        }

        if(id <= selected)
        {
            // New item added before/at the selection.
            selected++;
        }
    }

    void contextItemRemoved(Data::Pos id, ui::Item &)
    {
        if(id <= selected && selected > 0)
        {
            selected--;
        }

        updateButtonWithSelection();
    }

    void contextItemOrderChanged()
    {
        updateButtonWithSelection();
    }

    void updateItemHighlight()
    {
        // Highlight the currently selected item.
        for(Data::Pos i = 0; i < items().size(); ++i)
        {
            if(GuiWidget *w = choices->menu().organizer().itemWidget(i))
            {
                w->setFont(i == selected? "choice.selected" : "default");
            }
        }
    }

    void updateButtonWithItem(ui::Item const &item)
    {
        self.setText(item.label());

        ActionItem const *act = dynamic_cast<ActionItem const *>(&item);
        if(act)
        {
            self.setImage(act->image());
        }
    }

    void updateButtonWithSelection()
    {
        // Update the main button.
        if(isValidSelection())
        {
            updateButtonWithItem(items().at(selected));
        }
        else
        {
            // No valid selection.
            self.setText("");
            self.setImage(Image());
        }

        emit self.selectionChanged(selected);
    }
};

ChoiceWidget::ChoiceWidget(String const &name) : ButtonWidget(name), d(new Instance(this))
{
    setOpeningDirection(ui::Right);
}

void ChoiceWidget::setOpeningDirection(Direction dir)
{
    d->choices->setAnchorAndOpeningDirection(hitRule(), dir);
}

PopupMenuWidget &ChoiceWidget::popup()
{
    return *d->choices;
}

void ChoiceWidget::setSelected(Data::Pos pos)
{
    if(d->selected != pos)
    {
        d->selected = pos;
        d->updateButtonWithSelection();
        d->updateItemHighlight();
    }
}

Data::Pos ChoiceWidget::selected() const
{
    return d->selected;
}

Item const &ChoiceWidget::selectedItem() const
{
    DENG2_ASSERT(d->isValidSelection());
    return d->items().at(d->selected);
}

void ChoiceWidget::openPopup()
{
    d->updateItemHighlight();
    d->choices->open();
}

ui::Data &ChoiceWidget::items()
{
    return d->choices->items();
}
