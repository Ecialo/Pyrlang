#
# This example shows:
# 1. Creating a Python object based on `Process` class, and registering it with
#   a name.
# 2. A remote node can send to this process by name or Pid.
# 3. On incoming message MyProcess'es `handle_inbox` will find that there is a
#   message and will call `handle_one_inbox_message` which is overridden here.
# 4. There is no way to know sender unless you add return address into the msg.
#
# Requires:     Erlang running on the same host as:
#               `erl -name erl@127.0.0.1 -setcookie COOKIE`
# Run:          from project root run `make example2`
# Erl command:  {my_process, 'py@127.0.0.1'} ! hello.
#
import asyncio
import sys
sys.path.insert(0, ".")

from pyrlang.term.atom import Atom
from pyrlang.process import Process


class MyProcess(Process):
    def __init__(self, node) -> None:
        Process.__init__(self, node)
        node.register_name(self, Atom('my_process'))  # optional
        print("registering process - 'my_process'")

    def handle_one_inbox_message(self, msg):
        print("Incoming", msg)


async def main():
    node = PyrlangGevent.Node("py@127.0.0.1", "COOKIE")
    node.start()
    mp = MyProcess(node)
    while True:
        await asyncio.sleep(0.1)


if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    loop.run_until_complete(main())
    loop.close()
