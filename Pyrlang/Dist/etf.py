# Copyright 2017, Erlang Solutions Ltd.
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

""" Module implements encoder and decoder from ETF (Erlang External Term Format)
    used by the network distribution layer.

    Encoding terms takes optional 'options' argument. Default is ``None`` but
    it can be a dictionary with the following optional keys:

    *   "simple_lists", default True. Returns Erlang lists as Python lists,
        set this to False and term.List object will be returned which is able
        to also store list tail and convert contents to Unicode string.
    *   "simple_binaries", default False. Ignores bit tail of bit strings and
        returns all Erlang binaries as Python bytes.
    *   "atoms_as_strings", default False. Always converts atoms to Python
        strings. This is potentially faster than using the Atom wrapper class.
"""
from __future__ import print_function

import sys
from zlib import decompressobj

from Pyrlang import logger
from Pyrlang.Dist import util, etf_python

LOG = logger.nothing
ERROR = logger.tty

ETF_VERSION_TAG = 131  # also defined in etf_python.py
TAG_COMPRESSED = 80


def use_native_implementation():
    """ If C++ compiled extension is present, this will import it and replace
        coding functions with native implementations and return True.

        :return: True if native library imported, else False
    """
    try:
        # if successful, monkey patch worker functions of the current module
        import Pyrlang.term
        import nativeETF

        def wrap_bin_to_term(data, opts):
            """ The C++ version takes data and an index instead of a
                data slice. """
            (value, index) = nativeETF.binary_to_term_native(data, 0, opts)
            # Return slice of tail
            return value, data[index:]

        current_module = sys.modules[__name__]
        current_module.binary_to_term_2 = wrap_bin_to_term
        current_module.term_to_binary_2 = nativeETF.term_to_binary_native
        return True

    except ImportError:
        return False


def binary_to_term(data: bytes, options: dict = None):
    """ Strip 131 header and unpack if the data was compressed.

        :param data: The incoming encoded data with the 131 byte
        :param options: See description on top of the module
        :raises RuntimeError: when the tag is not 131, when compressed
            data is incomplete or corrupted
    """
    if data[0] != ETF_VERSION_TAG:
        raise RuntimeError("Unsupported external term version")

    if data[1] == TAG_COMPRESSED:
        do = decompressobj()
        decomp_size = util.u32(data, 2)
        decomp = do.decompress(data[6:]) + do.flush()
        if len(decomp) != decomp_size:
            # Data corruption?
            raise RuntimeError("Compressed size mismatch with actual")

        return binary_to_term_2(decomp, options)

    return binary_to_term_2(data[1:], options)


binary_to_term_2 = etf_python.binary_to_term_2


def term_to_binary(val):
    """ Prepend the 131 header byte to encoded data.
        :raises RuntimeError: when something is wrong
    """
    return bytes([ETF_VERSION_TAG]) + term_to_binary_2(val)


term_to_binary_2 = etf_python.term_to_binary_2


def use_python_implementation():
    """ Stop using native C++ implementation and roll back the Python
        variant. """
    current_module = sys.modules[__name__]
    current_module.binary_to_term_2 = etf_python.binary_to_term_2
    current_module.term_to_binary_2 = etf_python.term_to_binary_2


__all__ = ['binary_to_term', 'binary_to_term_2',
           'term_to_binary', 'term_to_binary_2',
           'use_native_implementation', 'use_python_implementation']
