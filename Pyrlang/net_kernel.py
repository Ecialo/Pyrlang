# Copyright 2018, Erlang Solutions Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import absolute_import
from __future__ import print_function

import logging

from Pyrlang import Term, gen
from Pyrlang.process import Process
from Pyrlang.node import Node

logger = logging.getLogger(__name__)


class NetKernel(Process):
    """ A special process which registers itself as ``net_kernel`` and handles
        one specific ``is_auth`` message, which is used by ``net_adm:ping``.
    """

    def __init__(self, node: Node) -> None:
        Process.__init__(self, node)
        node.register_name(self, Term.Atom('net_kernel'))

    def handle_one_inbox_message(self, msg):
        gencall = gen.parse_gen_message(msg)
        if not isinstance(gencall, gen.GenIncomingMessage):
            logging.debug("NetKernel:", gencall)
            return

        # Incoming gen_call packet to net_kernel, might be that net_adm:ping
        msg = gencall.message_

        if isinstance(msg[0], Term.Atom) and msg[0].text_ == 'is_auth':
            gencall.reply(local_pid=self.pid_,
                          result=Term.Atom('yes'))
        else:
            logger.warning("NetKernel: unknown message", msg)


__all__ = ['NetKernel']
