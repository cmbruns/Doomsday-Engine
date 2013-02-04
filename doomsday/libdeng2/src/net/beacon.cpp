/** @file beacon.cpp  Presence service based on UDP broadcasts.
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

#include "de/Beacon"
#include <QUdpSocket>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QTimer>
#include <QMap>

namespace de {

static char const *discoveryMessage = "Doomsday Beacon 1.0";

struct Beacon::Instance
{
    duint16 port;
    QUdpSocket *socket;
    Block message;
    QTimer *timer;
    Time discoveryEndsAt;
    QMap<Address, Block> found;

    Instance() : socket(0), timer(0)
    {}

    ~Instance()
    {
        delete socket;
        delete timer;
    }
};

Beacon::Beacon(duint16 port) : d(new Instance)
{
    d->port = port;
}

Beacon::~Beacon()
{
    delete d;
}

void Beacon::start()
{
    DENG2_ASSERT(!d->socket);

    d->socket = new QUdpSocket;
    connect(d->socket, SIGNAL(readyRead()), this, SLOT(readIncoming()));

    if(!d->socket->bind(d->port, QUdpSocket::ShareAddress))
    {
        /// @throws PortError Could not open the UDP port.
        throw PortError("Beacon::start", "Could not bind to UDP port " + String::number(d->port));
    }
}

void Beacon::setMessage(IByteArray const &advertisedMessage)
{
    d->message = advertisedMessage;
}

void Beacon::stop()
{
    delete d->socket;
    d->socket = 0;
}

void Beacon::discover(TimeDelta const &timeOut)
{
    if(d->timer) return; // Already discovering.

    DENG2_ASSERT(!d->socket);

    d->socket = new QUdpSocket;
    connect(d->socket, SIGNAL(readyRead()), this, SLOT(readDiscoveryReply()));

    // Choose a semi-random port for listening to replies from servers' beacons.
    if(!d->socket->bind(d->port + 1 + qrand() % 1024, QUdpSocket::ShareAddress))
    {
        /// @throws PortError Could not open the UDP port.
        throw PortError("Beacon::start", "Could not bind to UDP port " + String::number(d->port));
    }

    d->found.clear();

    // Time-out timer.
    d->discoveryEndsAt = Time() + timeOut;
    d->timer = new QTimer;
    connect(d->timer, SIGNAL(timeout()), this, SLOT(continueDiscovery()));
    d->timer->start(500);
}

QList<Address> Beacon::foundHosts() const
{
    return d->found.keys();
}

Block Beacon::messageFromHost(Address const &host) const
{
    if(!d->found.contains(host)) return Block();
    return d->found[host];
}

void Beacon::readIncoming()
{
    LOG_AS("Beacon");

    if(!d->socket) return;

    while(d->socket->hasPendingDatagrams())
    {
        QHostAddress from;
        quint16 port = 0;
        Block block(d->socket->pendingDatagramSize());
        d->socket->readDatagram(reinterpret_cast<char *>(block.data()),
                                block.size(), &from, &port);

        LOG_DEBUG("Received %i bytes from %s port %i") << block.size() << from.toString() << port;

        if(block == discoveryMessage)
        {
            // Send a reply.
            d->socket->writeDatagram(d->message, from, port);
        }
    }
}

void Beacon::readDiscoveryReply()
{
    LOG_AS("Beacon");

    if(!d->socket) return;

    while(d->socket->hasPendingDatagrams())
    {
        QHostAddress from;
        quint16 port = 0;
        Block block(d->socket->pendingDatagramSize());
        d->socket->readDatagram(reinterpret_cast<char *>(block.data()),
                                block.size(), &from, &port);

        if(block == discoveryMessage)
            continue;

        LOG_DEBUG("Received %i bytes from %s") << block.size() << from.toString();

        d->found.insert(Address(from), block);

        emit found(Address(from), block);
    }
}

void Beacon::continueDiscovery()
{
    DENG2_ASSERT(d->socket);
    DENG2_ASSERT(d->timer);

    // Time to stop discovering?
    if(Time() > d->discoveryEndsAt)
    {
        d->timer->stop();

        emit finished();

        d->socket->deleteLater();
        d->socket = 0;

        d->timer->deleteLater();
        d->timer = 0;
        return;
    }

    Block block(discoveryMessage);

    LOG_DEBUG("Broadcasting %i bytes") << block.size();

    // Send a new broadcast.
    d->socket->writeDatagram(block,
                             QHostAddress::Broadcast,
                             d->port);
}

} // namespace de
