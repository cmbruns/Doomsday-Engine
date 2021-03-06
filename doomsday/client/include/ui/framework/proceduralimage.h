/** @file proceduralimage.h  Procedural image.
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

#ifndef DENG_CLIENT_PROCEDURALIMAGE_H
#define DENG_CLIENT_PROCEDURALIMAGE_H

#include <de/Vector>
#include <de/GLBuffer>

/**
 * Base class for procedural images.
 *
 * A procedural image can be used instead of a static one to generate geometry
 * on the fly (see LabelWidget).
 */
class ProceduralImage
{
public:
    typedef de::Vector2f Size;
    typedef de::Vector4f Color;
    typedef de::GLBufferT<de::Vertex2TexRgba> DefaultVertexBuf;

public:
    ProceduralImage(Size const &size = Size());
    virtual ~ProceduralImage();

    Size size() const;
    Color color() const;

    void setSize(Size const &size);
    void setColor(Color const &color);

    virtual void update();
    virtual void glDeinit();
    virtual void glMakeGeometry(DefaultVertexBuf::Builder &verts, de::Rectanglef const &rect) = 0;

private:
    Size _size;
    Color _color;
};

#endif // DENG_CLIENT_PROCEDURALIMAGE_H
