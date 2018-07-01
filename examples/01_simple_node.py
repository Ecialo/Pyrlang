#
# Start a simple node and connect to an Erlang/Elixir node.
# This Python node is visible as `py@127.0.0.1`.
#
# Requires:     Erlang running on the same host as:
#               `erl -name erl@127.0.0.1 -setcookie COOKIE`
# Run:          from project root run `make example1`
#
import asyncio
import sys
sys.path.insert(0, ".")

from pyrlang.node import Node
from pyrlang.term.atom import Atom


async def main(loop: asyncio.AbstractEventLoop):
    node = Node("py@127.0.0.1", "COOKIE", loop=loop)
    node.start()

    # Attempt to send something will initiate a connection before sending
    pid = node.register_new_process(None)
    # To be able to send to Erlang shell by name first give it a registered
    # name: `erlang:register(shell, self()).`
    erl_node = Atom('erl@127.0.0.1')
    erl_registered_name = Atom('shell')
    await node.send(pid, (erl_node, erl_registered_name), Atom('hello'))

    while True:
        # Sleep gives other greenlets time to run
        await asyncio.sleep(0.1)


if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    loop.run_until_complete(main(loop))
    loop.close()
