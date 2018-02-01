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
import traceback

from Pyrlang import Term, gen
from Pyrlang.process import Process
from Pyrlang.node import Node

logger = logging.getLogger(__name__)


class Rex(Process):
    """ Remote executor for RPC calls. Registers itself under the name ``rex``
        and accepts RPC call messages.
        Erlang ``rpc:call`` sends a ``$gen_call`` styled message to the
        registered name ``rex`` on the remote node which we parse and attempt
        to execute.
    """

    def __init__(self, node: Node) -> None:
        Process.__init__(self, node)
        node.register_name(self, Term.Atom('rex'))

        self.traceback_depth_ = 5
        """ This being non-zero enables formatting exception tracebacks with the
            given depth. Traceback is attached as a 'traceback' field in the
            exception, that is sent to the caller. Default: 5
        """

    def handle_one_inbox_message(self, msg) -> None:
        """ Function overrides
            :py:meth:`~Pyrlang.process.Process.handle_one_inbox_message`
            and expects a ``$gen_call`` styled message.
            The result or exception are delivered back to the sender via
            message passing.

            :param msg: A tuple with Atom ``$gen_call`` as the first element
            :return: None
        """
        gencall = gen.parse_gen_call(msg)
        if isinstance(gencall, str):
            logger.debug("REX:", gencall)
            return

        # Find and run the thing
        try:
            pmod = __import__(gencall.get_mod_str(), fromlist=[''])
            pfun = getattr(pmod, gencall.get_fun_str())
            args = gencall.get_args()

            # Call the thing
            val = pfun(*args)

            # Send a reply
            gencall.reply(local_pid=self.pid_,
                          result=val)

        except Exception as excpt:
            # Send an error
            if self.traceback_depth_ > 0:
                excpt.traceback = traceback.format_exc(self.traceback_depth_)

            gencall.reply_exit(local_pid=self.pid_,
                               reason=excpt)
