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

""" Distribution class is not a separate running Greenlet, but rather a helper,
    which is called upon.
"""

from __future__ import print_function

import asyncio
import logging

from Pyrlang.Dist import helpers
from Pyrlang.Dist.epmd import EPMDClient, EPMDConnectionError
from Pyrlang.Dist.out_connection import OutConnection

logger = logging.getLogger(__name__)


class ErlangDistribution:
    """ Implements network part of the EPMD registration and Erlang distribution
        protocol. Extends functionality of Node, so all functions take
        Node as a parameter but don't store it to avoid creating a ref cycle.
    """

    def __init__(self, node, name: str) -> None:
        self.name_ = name
        """ Node name, a string. """

        self.creation_ = 0
        """ Creation id used in pid generation. EPMD gives creation id to 
            newly connected nodes. 
        """

        self.in_srv_ = None
        asyncio.get_event_loop().create_task(self._start_listening(node))

        self.epmd_ = EPMDClient()

    async def _start_listening(self, node):
        # Listener for Incoming connections from other nodes
        # Create handler using make_handler_in helper
        proto_kwargs = {"node": node}

        from Pyrlang.Dist.in_connection import InConnection
        handler_fn = helpers.make_handler_in(receiver_class=InConnection,
                                             args=[],
                                             kwargs=proto_kwargs)

        self.in_srv_ = await asyncio.start_server(handler_fn, '0.0.0.0', 0)
        # Dark magic to determine randomly assigned port
        self.in_port_ = self.in_srv_.sockets[0].getsockname()[1]

        logger.info("Dist: Listening for dist connections on port %d"
                    % self.in_port_)

    async def connect_epmd(self, _node) -> bool:
        """ Looks up EPMD daemon and connects to it trying to discover other 
            Erlang nodes.
        """
        while True:
            if self.epmd_.connect():
                return self.epmd_.alive2(self)

            asyncio.sleep(5.0)

    def disconnect(self) -> None:
        """ Finish EPMD connection, this will remove the node from the list of
            available nodes on EPMD
        """
        self.epmd_.close()

    def connect_to_node(self, this_node, remote_node: str):
        """ Query EPMD where is the node, and initiate dist connection. Blocks
            the Greenlet until the connection is made or have failed.

            :type this_node: Pyrlang.Node
            :param this_node: Reference to Erlang Node object
            :param remote_node: String with node 'name@ip'
            :return: Handler or None
        """
        try:
            host_port = EPMDClient.query_node(remote_node)
            (handler, _sock) = helpers.connect_with(
                protocol_class=OutConnection,
                host_port=host_port,
                args=[],
                kwargs={"node": this_node}
            )
            return handler

        except Exception as e:
            logger.error("Dist:", e)
            return None


__all__ = ['ErlangDistribution']
