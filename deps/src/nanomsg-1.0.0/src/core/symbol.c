/*
    Copyright (c) 2013 Evan Wies <evan@neomantra.net>
    Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.
    Copyright (c) 2016 Bent Cardan. All rights reserved.
    Copyright 2016 Garrett D'Amore <garrett@damore.org>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom
    the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#include "../nn.h"

#include "../inproc.h"
#include "../ipc.h"
#include "../tcp.h"

#include "../pair.h"
#include "../pubsub.h"
#include "../reqrep.h"
#include "../pipeline.h"
#include "../survey.h"
#include "../bus.h"
#include "../ws.h"

#include <string.h>

#define	NN_SYM(sym, namespace, typ, unit) \
    { sym, #sym, NN_NS_ ## namespace, NN_TYPE_ ## typ, NN_UNIT_ ## unit }

static const struct nn_symbol_properties sym_value_names [] = {
    NN_SYM(NN_NS_NAMESPACE, NAMESPACE, NONE, NONE),
    NN_SYM(NN_NS_VERSION, NAMESPACE, NONE, NONE),
    NN_SYM(NN_NS_DOMAIN, NAMESPACE, NONE, NONE),
    NN_SYM(NN_NS_TRANSPORT, NAMESPACE, NONE, NONE),
    NN_SYM(NN_NS_PROTOCOL, NAMESPACE, NONE, NONE),
    NN_SYM(NN_NS_OPTION_LEVEL, NAMESPACE, NONE, NONE),
    NN_SYM(NN_NS_SOCKET_OPTION, NAMESPACE, NONE, NONE),
    NN_SYM(NN_NS_TRANSPORT_OPTION, NAMESPACE, NONE, NONE),
    NN_SYM(NN_NS_OPTION_TYPE, NAMESPACE, NONE, NONE),
    NN_SYM(NN_NS_OPTION_UNIT, NAMESPACE, NONE, NONE),
    NN_SYM(NN_NS_FLAG, NAMESPACE, NONE, NONE),
    NN_SYM(NN_NS_ERROR, NAMESPACE, NONE, NONE),
    NN_SYM(NN_NS_LIMIT, NAMESPACE, NONE, NONE),
    NN_SYM(NN_NS_EVENT, NAMESPACE, NONE, NONE),
    NN_SYM(NN_NS_STATISTIC, NAMESPACE, NONE, NONE),

    NN_SYM(NN_TYPE_NONE, OPTION_TYPE, NONE, NONE),
    NN_SYM(NN_TYPE_INT, OPTION_TYPE, NONE, NONE),
    NN_SYM(NN_TYPE_STR, OPTION_TYPE, NONE, NONE),

    NN_SYM(NN_UNIT_NONE, OPTION_UNIT, NONE, NONE),
    NN_SYM(NN_UNIT_BYTES, OPTION_UNIT, NONE, NONE),
    NN_SYM(NN_UNIT_MILLISECONDS, OPTION_UNIT, NONE, NONE),
    NN_SYM(NN_UNIT_PRIORITY, OPTION_UNIT, NONE, NONE),
    NN_SYM(NN_UNIT_BOOLEAN, OPTION_UNIT, NONE, NONE),
    NN_SYM(NN_UNIT_COUNTER, OPTION_UNIT, NONE, NONE),
    NN_SYM(NN_UNIT_MESSAGES, OPTION_UNIT, NONE, NONE),

    NN_SYM(NN_VERSION_CURRENT, VERSION, NONE, NONE),
    NN_SYM(NN_VERSION_REVISION, VERSION, NONE, NONE),
    NN_SYM(NN_VERSION_AGE, VERSION, NONE, NONE),

    NN_SYM(AF_SP, DOMAIN, NONE, NONE),
    NN_SYM(AF_SP_RAW, DOMAIN, NONE, NONE),

    NN_SYM(NN_INPROC, TRANSPORT, NONE, NONE),
    NN_SYM(NN_IPC, TRANSPORT, NONE, NONE),
    NN_SYM(NN_TCP, TRANSPORT, NONE, NONE),
    NN_SYM(NN_WS, TRANSPORT, NONE, NONE),

    NN_SYM(NN_PAIR, PROTOCOL, NONE, NONE),
    NN_SYM(NN_PUB, PROTOCOL, NONE, NONE),
    NN_SYM(NN_SUB, PROTOCOL, NONE, NONE),
    NN_SYM(NN_REP, PROTOCOL, NONE, NONE),
    NN_SYM(NN_REQ, PROTOCOL, NONE, NONE),
    NN_SYM(NN_PUSH, PROTOCOL, NONE, NONE),
    NN_SYM(NN_PULL, PROTOCOL, NONE, NONE),
    NN_SYM(NN_SURVEYOR, PROTOCOL, NONE, NONE),
    NN_SYM(NN_RESPONDENT, PROTOCOL, NONE, NONE),
    NN_SYM(NN_BUS, PROTOCOL, NONE, NONE),

    NN_SYM(NN_SOCKADDR_MAX, LIMIT, NONE, NONE),

    NN_SYM(NN_SOL_SOCKET, OPTION_LEVEL, NONE, NONE),

    NN_SYM(NN_LINGER, SOCKET_OPTION, INT, MILLISECONDS),
    NN_SYM(NN_SNDBUF, SOCKET_OPTION, INT, BYTES),
    NN_SYM(NN_RCVBUF, SOCKET_OPTION, INT, BYTES),
    NN_SYM(NN_RCVMAXSIZE, SOCKET_OPTION, INT, BYTES),
    NN_SYM(NN_SNDTIMEO, SOCKET_OPTION, INT, MILLISECONDS),
    NN_SYM(NN_RCVTIMEO, SOCKET_OPTION, INT, MILLISECONDS),
    NN_SYM(NN_RECONNECT_IVL, SOCKET_OPTION, INT, MILLISECONDS),
    NN_SYM(NN_RECONNECT_IVL_MAX, SOCKET_OPTION, INT, MILLISECONDS),
    NN_SYM(NN_SNDPRIO, SOCKET_OPTION, INT, PRIORITY),
    NN_SYM(NN_RCVPRIO, SOCKET_OPTION, INT, PRIORITY),
    NN_SYM(NN_SNDFD, SOCKET_OPTION, INT, NONE),
    NN_SYM(NN_RCVFD, SOCKET_OPTION, INT, NONE),
    NN_SYM(NN_DOMAIN, SOCKET_OPTION, INT, NONE),
    NN_SYM(NN_PROTOCOL, SOCKET_OPTION, INT, NONE),
    NN_SYM(NN_IPV4ONLY, SOCKET_OPTION, INT, BOOLEAN),
    NN_SYM(NN_SOCKET_NAME, SOCKET_OPTION, STR, NONE),

    NN_SYM(NN_SUB_SUBSCRIBE, TRANSPORT_OPTION, STR, NONE),
    NN_SYM(NN_SUB_UNSUBSCRIBE, TRANSPORT_OPTION, STR, NONE),
    NN_SYM(NN_REQ_RESEND_IVL, TRANSPORT_OPTION, INT, MILLISECONDS),
    NN_SYM(NN_SURVEYOR_DEADLINE, TRANSPORT_OPTION, INT, MILLISECONDS),
    NN_SYM(NN_TCP_NODELAY, TRANSPORT_OPTION, INT, BOOLEAN),
    NN_SYM(NN_WS_MSG_TYPE, TRANSPORT_OPTION, INT, NONE),

    NN_SYM(NN_DONTWAIT, FLAG, NONE, NONE),
    NN_SYM(NN_WS_MSG_TYPE_TEXT, FLAG, NONE, NONE),
    NN_SYM(NN_WS_MSG_TYPE_BINARY, FLAG, NONE, NONE),

    NN_SYM(NN_POLLIN, EVENT, NONE, NONE),
    NN_SYM(NN_POLLOUT, EVENT, NONE, NONE),

    NN_SYM(EADDRINUSE, ERROR, NONE, NONE),
    NN_SYM(EADDRNOTAVAIL, ERROR, NONE, NONE),
    NN_SYM(EAFNOSUPPORT, ERROR, NONE, NONE),
    NN_SYM(EAGAIN, ERROR, NONE, NONE),
    NN_SYM(EBADF, ERROR, NONE, NONE),
    NN_SYM(ECONNREFUSED, ERROR, NONE, NONE),
    NN_SYM(EFAULT, ERROR, NONE, NONE),
    NN_SYM(EFSM, ERROR, NONE, NONE),
    NN_SYM(EINPROGRESS, ERROR, NONE, NONE),
    NN_SYM(EINTR, ERROR, NONE, NONE),
    NN_SYM(EINVAL, ERROR, NONE, NONE),
    NN_SYM(EMFILE, ERROR, NONE, NONE),
    NN_SYM(ENAMETOOLONG, ERROR, NONE, NONE),
    NN_SYM(ENETDOWN, ERROR, NONE, NONE),
    NN_SYM(ENOBUFS, ERROR, NONE, NONE),
    NN_SYM(ENODEV, ERROR, NONE, NONE),
    NN_SYM(ENOMEM, ERROR, NONE, NONE),
    NN_SYM(ENOPROTOOPT, ERROR, NONE, NONE),
    NN_SYM(ENOTSOCK, ERROR, NONE, NONE),
    NN_SYM(ENOTSUP, ERROR, NONE, NONE),
    NN_SYM(EPROTO, ERROR, NONE, NONE),
    NN_SYM(EPROTONOSUPPORT, ERROR, NONE, NONE),
    NN_SYM(ETERM, ERROR, NONE, NONE),
    NN_SYM(ETIMEDOUT, ERROR, NONE, NONE),
    NN_SYM(EACCES, ERROR, NONE, NONE),
    NN_SYM(ECONNABORTED, ERROR, NONE, NONE),
    NN_SYM(ECONNRESET, ERROR, NONE, NONE),
    NN_SYM(EHOSTUNREACH, ERROR, NONE, NONE),
    NN_SYM(EMSGSIZE, ERROR, NONE, NONE),
    NN_SYM(ENETRESET, ERROR, NONE, NONE),
    NN_SYM(ENETUNREACH, ERROR, NONE, NONE),
    NN_SYM(ENOTCONN, ERROR, NONE, NONE),

    NN_SYM(NN_STAT_ESTABLISHED_CONNECTIONS, STATISTIC, INT, COUNTER),
    NN_SYM(NN_STAT_ACCEPTED_CONNECTIONS, STATISTIC, INT, COUNTER),
    NN_SYM(NN_STAT_DROPPED_CONNECTIONS, STATISTIC, INT, COUNTER),
    NN_SYM(NN_STAT_BROKEN_CONNECTIONS, STATISTIC, INT, COUNTER),
    NN_SYM(NN_STAT_CONNECT_ERRORS, STATISTIC, INT, COUNTER),
    NN_SYM(NN_STAT_BIND_ERRORS, STATISTIC, INT, COUNTER),
    NN_SYM(NN_STAT_ACCEPT_ERRORS, STATISTIC, INT, COUNTER),
    NN_SYM(NN_STAT_MESSAGES_SENT, STATISTIC, INT, MESSAGES),
    NN_SYM(NN_STAT_MESSAGES_RECEIVED, STATISTIC, INT, MESSAGES),
    NN_SYM(NN_STAT_BYTES_SENT, STATISTIC, INT, BYTES),
    NN_SYM(NN_STAT_BYTES_RECEIVED, STATISTIC, INT, BYTES),
    NN_SYM(NN_STAT_CURRENT_CONNECTIONS, STATISTIC, INT, NONE),
    NN_SYM(NN_STAT_INPROGRESS_CONNECTIONS, STATISTIC, INT, NONE),
    NN_SYM(NN_STAT_CURRENT_SND_PRIORITY, STATISTIC, INT, PRIORITY),
    NN_SYM(NN_STAT_CURRENT_EP_ERRORS, STATISTIC, INT, NONE)
};

const int SYM_VALUE_NAMES_LEN = (sizeof (sym_value_names) /
    sizeof (sym_value_names [0]));


const char *nn_symbol (int i, int *value)
{
    const struct nn_symbol_properties *svn;
    if (i < 0 || i >= SYM_VALUE_NAMES_LEN) {
        errno = EINVAL;
        return NULL;
    }

    svn = &sym_value_names [i];
    if (value)
        *value = svn->value;
    return svn->name;
}

int nn_symbol_info (int i, struct nn_symbol_properties *buf, int buflen)
{
    if (i < 0 || i >= SYM_VALUE_NAMES_LEN) {
        return 0;
    }
    if (buflen > (int)sizeof (struct nn_symbol_properties)) {
        buflen = (int)sizeof (struct nn_symbol_properties);
    }
    memcpy(buf, &sym_value_names [i], buflen);
    return buflen;
}