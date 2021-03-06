/** @file panelwidget.cpp
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

#include "ui/widgets/panelwidget.h"
#include "GuiRootWidget"
#include "ui/style.h"
#include "ui/clientwindow.h"

#include <de/Drawable>
#include <de/MouseEvent>
#include <de/ScalarRule>
#include <de/math.h>
#include <QTimer>

using namespace de;

static TimeDelta const OPENING_ANIM_SPAN = 0.4;
static TimeDelta const CLOSING_ANIM_SPAN = 0.3;

DENG_GUI_PIMPL(PanelWidget)
{
    typedef DefaultVertexBuf VertexBuf;

    bool opened;
    ui::Direction dir;
    ui::SizePolicy secondaryPolicy;
    GuiWidget *content;
    ScalarRule *openingRule;
    QTimer dismissTimer;

    // GL objects.
    Drawable drawable;
    GLUniform uMvpMatrix;
    //GLUniform uColor;

    Instance(Public *i)
        : Base(i),
          opened(false),
          dir(ui::Down),
          secondaryPolicy(ui::Expand),
          content(0),
          uMvpMatrix("uMvpMatrix", GLUniform::Mat4)
    {
        openingRule = new ScalarRule(0);

        dismissTimer.setSingleShot(true);
        QObject::connect(&dismissTimer, SIGNAL(timeout()), thisPublic, SLOT(dismiss()));
    }

    ~Instance()
    {
        releaseRef(openingRule);
    }

    void glInit()
    {
        drawable.addBuffer(new VertexBuf);
        shaders().build(drawable.program(), "generic.textured.color")
                << uMvpMatrix << uAtlas();
    }

    void glDeinit()
    {
        drawable.clear();
    }

    bool isVerticalAnimation() const
    {
        return isVertical(dir) || dir == ui::NoDirection;
    }

    void updateLayout()
    {
        DENG2_ASSERT(content != 0);

        // Widget's size depends on the opening animation.
        if(isVerticalAnimation())
        {
            self.rule().setInput(Rule::Height, *openingRule);
            if(secondaryPolicy == ui::Expand)
            {
                self.rule().setInput(Rule::Width, content->rule().width());
            }
        }
        else
        {
            self.rule().setInput(Rule::Width, *openingRule);
            if(secondaryPolicy == ui::Expand)
            {
                self.rule().setInput(Rule::Height, content->rule().height());
            }
        }
    }

    void updateGeometry()
    {
        Rectanglei pos;
        if(self.hasChangedPlace(pos) || self.geometryRequested())
        {
            self.requestGeometry(false);

            VertexBuf::Builder verts;
            self.glMakeGeometry(verts);
            drawable.buffer<VertexBuf>().setVertices(gl::TriangleStrip, verts, gl::Static);
        }
    }

    void startOpeningAnimation(TimeDelta span)
    {
        if(isVerticalAnimation())
        {
            openingRule->set(content->rule().height(), span);
        }
        else
        {
            openingRule->set(content->rule().width(), span);
        }
        openingRule->setStyle(Animation::Bounce, 8);
    }

    void close(TimeDelta delay)
    {
        if(!opened) return;

        opened = false;

        self.setBehavior(DisableEventDispatchToChildren);

        // Begin the closing animation.
        openingRule->set(0, CLOSING_ANIM_SPAN + delay, delay);
        openingRule->setStyle(Animation::EaseIn);

        self.panelClosing();

        DENG2_FOR_PUBLIC_AUDIENCE(Close, i)
        {
            i->panelBeingClosed(self);
        }

        emit self.closed();

        dismissTimer.start();
        dismissTimer.setInterval((CLOSING_ANIM_SPAN + delay).asMilliSeconds());
    }
};

PanelWidget::PanelWidget(String const &name) : GuiWidget(name), d(new Instance(this))
{
    setBehavior(ChildHitClipping);
    hide();
}

void PanelWidget::setContent(GuiWidget *content)
{
    if(d->content)
    {        
        destroy(takeContent());
    }

    d->content = content;
    add(content); // ownership taken

    // Place the content inside the panel.
    content->rule()
            .setInput(Rule::Left, rule().left())
            .setInput(Rule::Top,  rule().top());
}

GuiWidget &PanelWidget::content() const
{
    DENG2_ASSERT(d->content != 0);
    return *d->content;
}

GuiWidget *PanelWidget::takeContent()
{
    GuiWidget *w = d->content;
    d->content = 0;

    w->rule().clearInput(Rule::Left);
    w->rule().clearInput(Rule::Top);

    if(d->secondaryPolicy == ui::Expand)
    {
        rule().clearInput(Rule::Width);
        rule().clearInput(Rule::Height);
    }

    remove(*w);
    return w;
}

void PanelWidget::setOpeningDirection(ui::Direction dir)
{
    d->dir = dir;
}

void PanelWidget::setSizePolicy(ui::SizePolicy policy)
{
    d->secondaryPolicy = policy;
}

ui::Direction PanelWidget::openingDirection() const
{
    return d->dir;
}

bool PanelWidget::isOpen() const
{
    return d->opened;
}

void PanelWidget::close(TimeDelta delayBeforeClosing)
{
    d->close(delayBeforeClosing);
}

void PanelWidget::viewResized()
{
    GuiWidget::viewResized();

    d->uMvpMatrix = root().projMatrix2D();

    requestGeometry();
}

void PanelWidget::update()
{
    GuiWidget::update();
}

void PanelWidget::preDrawChildren()
{
    GLState::push().setNormalizedScissor(normalizedRect());
}

void PanelWidget::postDrawChildren()
{
    GLState::pop();
}

bool PanelWidget::handleEvent(Event const &event)
{
    if(event.type() == Event::MouseButton)
    {
        MouseEvent const &mouse = event.as<MouseEvent>();

        // Eat buttons that land on the panel.
        if(hitTest(mouse.pos()))
        {
            root().setFocus(0);
            return true;
        }
    }

    return GuiWidget::handleEvent(event);
}

void PanelWidget::open()
{
    if(d->opened) return;

    d->dismissTimer.stop();

    unsetBehavior(DisableEventDispatchToChildren);
    show();

    preparePanelForOpening();

    // Start the opening animation.
    d->startOpeningAnimation(OPENING_ANIM_SPAN);

    d->opened = true;

    emit opened();
}

void PanelWidget::close()
{
    d->close(0.2);
}

void PanelWidget::dismiss()
{
    if(isHidden()) return;

    hide();
    d->opened = false;
    d->dismissTimer.stop();

    panelDismissed();

    emit dismissed();
}

void PanelWidget::drawContent()
{
    d->updateGeometry();
    d->drawable.draw();
}

void PanelWidget::glMakeGeometry(DefaultVertexBuf::Builder &verts)
{
    GuiWidget::glMakeGeometry(verts);
}

void PanelWidget::glInit()
{
    d->glInit();
}

void PanelWidget::glDeinit()
{
    d->glDeinit();
}

void PanelWidget::preparePanelForOpening()
{
    d->updateLayout();
}

void PanelWidget::panelClosing()
{
    // to be overridden
}

void PanelWidget::panelDismissed()
{
    // nothing to do
}
