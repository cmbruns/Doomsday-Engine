/*
 * The Doomsday Engine Project
 *
 * Copyright (c) 2011-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 *
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBDENG2_RECTANGLERULE_H
#define LIBDENG2_RECTANGLERULE_H

#include "../AnimationVector"
#include "../Rectangle"
#include "rules.h"

namespace de {

/**
 * A set of rules defining a rectangle.
 *
 * Instead of being derived from Rule, RuleRectangle acts as a complex mapping
 * between a set of input and output Rule instances. Note that RuleRectangle is
 * not reference-counted like Rule instances.
 *
 * RuleRectangle::rect() returns the rectangle's currently valid bounds. The
 * output rules for the sides can be used normally in other rules. Horizontal
 * and vertical axes are handled independently.
 *
 * Note that RuleRectangle uses a "fluent API" for the input rule set/clear
 * methods.
 *
 * @ingroup widgets
 */
class DENG2_PUBLIC RuleRectangle
{
public:
    RuleRectangle();

    // Output rules.
    Rule const &left() const;
    Rule const &top() const;
    Rule const &right() const;
    Rule const &bottom() const;
    Rule const &width() const;
    Rule const &height() const;

    /**
     * Sets one of the input rules of the rectangle.
     *
     * @param inputRule  Semantic of the input rule.
     * @param rule       Rule to use as input. A reference is held.
     */
    RuleRectangle &setInput(Rule::Semantic inputRule, Rule const &rule);

    RuleRectangle &setLeftTop(Rule const &left, Rule const &top);

    RuleRectangle &setRightBottom(Rule const &right, Rule const &bottom);

    RuleRectangle &setSize(Rule const &width, Rule const &height);

    /**
     * Sets the outputs of another rule rectangle as the inputs of this one.
     *
     * @param rect  Rectangle whose outputs to use as inputs.
     */
    RuleRectangle &setRect(RuleRectangle const &rect);

    /**
     * Sets the inputs of another rule rectangle as the inputs of this one.
     * (Note the difference to setRect().)
     *
     * @param rect  Rectangle whose inputs to use as inputs.
     */
    RuleRectangle &setInputsFromRect(RuleRectangle const &rect);

    RuleRectangle &clearInput(Rule::Semantic inputRule);

    /**
     * Returns an input rule.
     */
    Rule const &inputRule(Rule::Semantic inputRule);

    template <class RuleType>
    RuleType const &inputRuleAs(Rule::Semantic input) {
        RuleType const *r = dynamic_cast<RuleType const *>(&inputRule(input));
        DENG2_ASSERT(r != 0);
        return *r;
    }

    /**
     * Sets the anchor reference point within the rectangle for the anchor X
     * and anchor Y rules.
     *
     * @param normalizedPoint  (0, 0) refers to the top left corner,
     *                         (1, 1) to the bottom right.
     * @param transition       Transition time for the change.
     */
    void setAnchorPoint(Vector2f const &normalizedPoint, TimeDelta const &transition = 0);

    /**
     * Returns the current rectangle as defined by the input rules.
     */
    Rectanglef rect() const;

    /**
     * Returns the current size of the rectangle as defined by the input rules.
     */
    Vector2f size() const;

    /**
     * Returns the current size of the rectangle as defined by the input rules.
     */
    Vector2i sizei() const;

    /**
     * Returns the current rectangle as defined by the input rules.
     * Values are floored to integers.
     */
    Rectanglei recti() const;

    void setDebugName(String const &name);

    String description() const;

private:
    DENG2_PRIVATE(d)
};

} // namespace de

#endif // LIBDENG2_RECTANGLERULE_H
