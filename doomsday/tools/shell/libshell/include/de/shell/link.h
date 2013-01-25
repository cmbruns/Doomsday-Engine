/** @file link.h  Network connection to a server.
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

#ifndef LIBSHELL_LINK_H
#define LIBSHELL_LINK_H

#include <de/Address>
#include <de/Time>
#include <de/Transmitter>
#include <de/shell/Protocol>
#include <QObject>

namespace de {
namespace shell {

/**
 * Network connection to a server using the shell protocol.
 */
class DENG2_PUBLIC Link : public QObject, public Transmitter
{
    Q_OBJECT

public:
    enum Status
    {
        Disconnected,
        Connecting,
        Connected
    };

public:
    /**
     * Opens a connection to a server over the network.
     *
     * @param address  Address of the server.
     */
    Link(Address const &address);

    virtual ~Link();

    /**
     * Peer address of the link.
     */
    Address address() const;

    /**
     * Current status of the connection.
     */
    Status status() const;

    /**
     * Returns the time when the link was successfully connected.
     */
    Time connectedAt() const;

    /**
     * Returns the next received packet.
     *
     * @return Received packet. Ownership given to caller. Returns @c NULL if
     * there are no more packets ready.
     */
    Packet *nextPacket();

    // Transmitter.
    void send(IByteArray const &data);

protected slots:
    void socketConnected();
    void socketDisconnected();

signals:
    void connected();
    void packetsReady();
    void disconnected();

private:
    struct Instance;
    Instance *d;
};

} // namespace shell
} // namespace de

#endif // LIBSHELL_LINK_H