#
# Start a simple node and connect to an Erlang/Elixir node.
# This Python node is visible as `py@127.0.0.1`.
#
# Requires:     Erlang running on the same host as:
#               `erl -name erl@127.0.0.1 -setcookie COOKIE`
# Run:          from project root run `make example1`
#
import asyncio

# for Pyrlang tests: help Python find Pyrlang in this dir
import sys
sys.path.insert(0, ".")

from Pyrlang import Node, Atom


def main():
    ev_loop = asyncio.get_event_loop()

    node = Node("py@127.0.0.1", "COOKIE")

    # Attempt to send something will initiate a connection before sending
    pid = node.register_new_process(None)
    # To be able to send to Erlang shell by name first give it a registered
    # name: `erlang:register(shell, self()).`
    node.send(pid, (Atom('erl@127.0.0.1'), Atom('shell')), Atom('hello'))

    ev_loop.run_forever()


if __name__ == "__main__":
    main()
