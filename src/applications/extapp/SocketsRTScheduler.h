//
// Copyright (C) 2005 Andras Varga,
//                    Christian Dankbar, Irene Ruengeler, Michael Tuexen
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_SOCKETSRTSCHEDULER_H
#define __INET_SOCKETSRTSCHEDULER_H

#define WANT_WINSOCK2

#include <platdep/sockets.h>
#include <platdep/timeutil.h>
#include "INETDefs.h"

#define HAVE_U_INT8_T
#define HAVE_U_INT16_T
#define HAVE_U_INT32_T
#define HAVE_U_INT64_T


class ISocketRT
{
  public:
    virtual ~ISocketRT() {}
    virtual void recved(int fd, int length, const char* bytes) = 0;
};

class SocketsRTScheduler : public cScheduler
{
  public:
    class Socket
    {
      public:
        ISocketRT *module;
        int fd;
        bool isListener;
      public:
        Socket() : module(NULL), fd(INVALID_SOCKET), isListener(false) {}
        Socket(ISocketRT *module, int fd, bool isListener) : module(module), fd(fd), isListener(isListener) {}
    };

  protected:
    enum { TIMEOUT = 10000 /* 10 ms */ };

  protected:
    static std::vector<Socket> sockets;
    static timeval baseTime;

  protected:
    virtual bool receiveWithTimeout();
    virtual int receiveUntil(const timeval& targetTime);

  public:
    SocketsRTScheduler();
    virtual ~SocketsRTScheduler();

    /**
     * Called at the beginning of a simulation run.
     */
    virtual void startRun();

    /**
     * Called at the end of a simulation run.
     */
    virtual void endRun();

    /**
     * Recalculates "base time" from current wall clock time.
     */
    virtual void executionResumed();

#if OMNETPP_VERSION >= 0x0500
    /**
     * Returns the first event in the Future Event Set.
     */
    virtual cEvent *guessNextEvent();

    /**
     * Scheduler function -- it comes from the cScheduler interface.
     */
    virtual cEvent *takeNextEvent();

    /**
     * Scheduler function -- it comes from the cScheduler interface.
     */
    virtual void putBackEvent(cEvent *event);
#else
    /**
     * Scheduler function -- it comes from cScheduler interface.
     */
    virtual cMessage *getNextEvent();
#endif

    /**
     */
    void addSocket(ISocketRT *mod, int fd, bool isListener);
};

#endif
