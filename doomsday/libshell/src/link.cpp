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

#include "de/shell/Link"
#include "de/shell/Protocol"
#include <de/Message>
#include <de/Socket>
#include <de/Time>
#include <de/Log>
#include <QTimer>

namespace de {
namespace shell {

struct Link::Instance
{
    Link &self;
    String tryingToConnectToHost;
    Time startedTryingAt;
    TimeDelta timeout;
    Address peerAddress;
    Socket *socket;
    Protocol protocol;
    Status status;
    Time connectedAt;

    Instance(Link &i)
        : self(i),
          socket(0),
          status(Disconnected),
          connectedAt(Time::invalidTime()) {}

    ~Instance()
    {
        delete socket;
    }
};

Link::Link(String const &domain, TimeDelta const &timeout) : d(new Instance(*this))
{
    d->socket = new Socket;

    connect(d->socket, SIGNAL(addressResolved()), this, SIGNAL(addressResolved()));
    connect(d->socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(d->socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
    connect(d->socket, SIGNAL(messagesReady()), this, SIGNAL(packetsReady()));

    // Fallback to default port.
    d->tryingToConnectToHost = domain;
    d->socket->setQuiet(true); // we'll be retrying a few times
    d->socket->connectToDomain(d->tryingToConnectToHost, 13209 /* default port */);

    d->status = Connecting;
    d->startedTryingAt = Time();
    d->timeout = timeout;
}

Link::Link(Address const &address) : d(new Instance(*this))
{
    d->peerAddress = address;
    d->socket = new Socket;

    connect(d->socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(d->socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
    connect(d->socket, SIGNAL(messagesReady()), this, SIGNAL(packetsReady()));

    // Fallback to default port.
    if(!d->peerAddress.port()) d->peerAddress.setPort(13209);

    d->socket->connect(d->peerAddress);

    d->status = Connecting;
    d->startedTryingAt = Time();
    d->timeout = 0;
}

Link::Link(Socket *openSocket) : d(new Instance(*this))
{
    d->peerAddress = openSocket->peerAddress();
    d->socket = openSocket;

    // Note: socketConnected() not used because the socket is already open.
    connect(d->socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
    connect(d->socket, SIGNAL(messagesReady()), this, SIGNAL(packetsReady()));

    d->status = Connected;
    d->connectedAt = Time();
}

Link::~Link()
{
    delete d;
}

Address Link::address() const
{
    if(d->socket->isOpen()) return d->socket->peerAddress();
    return d->peerAddress;
}

Link::Status Link::status() const
{
    return d->status;
}

Time Link::connectedAt() const
{
    return d->connectedAt;
}

Protocol &Link::protocol()
{
    return d->protocol;
}

Packet *Link::nextPacket()
{
    if(!d->socket->hasIncoming()) return 0;

    std::auto_ptr<Message> data(d->socket->receive());
    Packet *packet = d->protocol.interpret(*data.get());
    if(packet) packet->setFrom(data->address());
    return packet;
}

void Link::send(IByteArray const &data)
{
    d->socket->send(data);
}

void Link::socketConnected()
{
    LOG_AS("Link");
    LOG_VERBOSE("Successfully connected to server %s") << d->socket->peerAddress();

    // Tell the server to switch to shell mode (v1).
    *d->socket << String("Shell").toLatin1();

    d->status = Connected;
    d->connectedAt = Time();
    d->peerAddress = d->socket->peerAddress();

    emit connected();
}

void Link::socketDisconnected()
{
    LOG_AS("Link");

    if(d->status == Connecting)
    {
        if(d->startedTryingAt.since() < d->timeout)
        {
            // Let's try again a bit later.
            QTimer::singleShot(500, d->socket, SLOT(reconnect()));
            return;
        }
        d->socket->setQuiet(false);
    }

    if(!d->peerAddress.isNull())
    {
        LOG_INFO("Disconnected from %s") << d->peerAddress;
    }
    else
    {
        LOG_INFO("Disconnected");
    }

    d->status = Disconnected;

    emit disconnected();

    // Slots have now had an opportunity to observe the total
    // duration of the connection that has just ended.
    d->connectedAt = Time::invalidTime();
}

} // namespace shell
} // namespace de