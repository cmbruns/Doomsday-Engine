/** @file shelluser.cpp  Remote user of a shell connection.
 * @ingroup server
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

#include "shelluser.h"
#include <de/shell/Protocol>
#include <de/shell/Lexicon>
#include <de/LogSink>
#include <de/Log>
#include <de/LogBuffer>

#include "con_main.h"

using namespace de;

struct ShellUser::Instance : public LogSink
{
    ShellUser &self;

    /// Log entries to be sent are collected here.
    shell::LogEntryPacket logEntryPacket;

    Instance(ShellUser &inst) : self(inst)
    {
        // We will send all log entries to a shell user.
        LogBuffer::appBuffer().addSink(*this);
    }

    ~Instance()
    {
        LogBuffer::appBuffer().removeSink(*this);
    }

    LogSink &operator << (LogEntry const &entry)
    {
        logEntryPacket.add(entry);
        return *this;
    }

    LogSink &operator << (String const &)
    {
        return *this;
    }

    /**
     * Sends the accumulated log entries over the link.
     */
    void flush()
    {
        if(!logEntryPacket.isEmpty() && self.status() == shell::Link::Connected)
        {
            self << logEntryPacket;
            logEntryPacket.clear();
        }
    }
};

ShellUser::ShellUser(Socket *socket) : shell::Link(socket), d(new Instance(*this))
{
    connect(this, SIGNAL(packetsReady()), this, SLOT(handleIncomingPackets()));
}

static int addToTerms(knownword_t const *word, void *parameters)
{
    shell::Lexicon *lexi = reinterpret_cast<shell::Lexicon *>(parameters);
    lexi->addTerm(Str_Text(Con_KnownWordToString(word)));
    return 0;
}

void ShellUser::sendInitialUpdate()
{
    // Console lexicon.
    shell::Lexicon lexi;
    Con_IterateKnownWords(0, WT_ANY, addToTerms, &lexi);
    lexi.setAdditionalWordChars("-_.");
    QScopedPointer<RecordPacket> packet(protocol().newConsoleLexicon(lexi));
    *this << *packet;
}

ShellUser::~ShellUser()
{
    delete d;
}

void ShellUser::handleIncomingPackets()
{
    forever
    {
        QScopedPointer<Packet> packet(nextPacket());
        if(packet.isNull()) break;

        try
        {
            switch(protocol().recognize(packet.data()))
            {
            case shell::Protocol::Command:
                Con_Execute(CMDS_CONSOLE, protocol().command(*packet).toUtf8().constData(), false, true);
                break;

            default:
                break;
            }
        }
        catch(Error const &er)
        {
            LOG_WARNING("Error while processing packet from %s:\n%s") << packet->from() << er.asText();
        }
    }
}