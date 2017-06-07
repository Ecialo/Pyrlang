#include "library.h"
#include "byte_codec.h"

namespace pyrlang {


void B2TOptions::parse(const Py::Dict& pyopts) {
  if (pyopts.hasKey("simple_binaries")) {
    simple_binaries = pyopts.getItem("simple_binaries").as_bool();
  }
  if (pyopts.hasKey("atoms_as_strings")) {
    atoms_as_strings = pyopts.getItem("atoms_as_strings").as_bool();
  }
  if (pyopts.hasKey("simple_lists")) {
    simple_lists = pyopts.getItem("simple_lists").as_bool();
  }
}


Py::Object NativeETFModule::py_binary_to_term_native(const Py::Tuple& args) {
  // Instead of moving and slicing bytes, we manipulate constant bytes object
  // and an integer offset in it.
  // :type data: bytes
  // :type offset: int
  // :type options: dict, must be set
  args.verify_length(1, 3);

  B2TOptions options;
  if (args[2].isDict()) {
    options.parse(args[2]);
  }

  // XXX: Possibly copying to a string can be avoided
  std::string input_str = Py::Bytes(args[0]).as_std_string();
  size_t index = (unsigned long) Py::Long(args[1]);

  Py::Object result;
  std::tie(result, index) = binary_to_term_native(input_str, index, options);

  return Py::TupleN(result, Py::Long(index));
}


NativeETFModule::B2TResult
NativeETFModule::binary_to_term_native(const std::string& input_str,
                                       size_t index,
                                       const B2TOptions& options)
{
  if (index >= input_str.length()) {
    return etf_error("No data remaining, must at least have 1 byte more");
  }

  const char* ptr = input_str.data() + index;
  size_t input_length = input_str.length() - index;

  char tag = ptr[0];
  switch (tag) {
    case TAG_ATOM_EXT:
    case TAG_ATOM_UTF8_EXT: {
      return _decode_atom(index, options, ptr, input_length, tag);
    }

    case TAG_SMALL_ATOM_EXT:
    case TAG_SMALL_ATOM_UTF8_EXT: {
      return _decode_small_atom(index, options, ptr, input_length, tag);
    }

    case TAG_NIL_EXT: {
      return std::make_pair((Py::Object)Py::List(), index + 1);
    }

    case TAG_STRING_EXT: {
      auto len_expected = codec::read_big_u16(ptr + 1);
      return std::make_pair((Py::Object)Py::String(ptr + 3, len_expected),
                            index + 3 + len_expected);
    }

    case TAG_LIST_EXT: {
      return _decode_list(input_str, index, options, ptr, input_length);
    }

    case TAG_MAP_EXT: {
      return _decode_map(input_str, index, options, ptr, input_length);
    }

    case TAG_SMALL_TUPLE_EXT: {
      if (input_length < 2) {
        return incomplete_data("decoding length for a small tuple");
      }
      int tuple_size = ptr[1];
      return _decode_tuple(input_str, index + 2, options, tuple_size);
    }

    case TAG_LARGE_TUPLE_EXT: {
      if (input_length < 5) {
        return incomplete_data("decoding length for a large tuple");
      }
      int tuple_size = codec::read_big_u32(ptr + 1);
      return _decode_tuple(input_str, index + 6, options, tuple_size);
    }

    case TAG_SMALL_INT: {
      // 8-bit unsigned
      int val = (int)(uint8_t)ptr[1]; // chars are signed, we want a byte
      return std::make_pair((Py::Object)Py::Long(val), index + 2);
    }

    case TAG_INT: {
      // 32-bit signed
      int32_t val = (int32_t)codec::read_big_u32(ptr + 1);
      return std::make_pair((Py::Object)Py::Long(val), index + 5);
    }

    case TAG_PID_EXT: {
      return _decode_pid(input_str, index, options, ptr, input_length);
    }

    case TAG_NEW_REF_EXT: {
      return _decode_ref(input_str, index, options, ptr, input_length);
    }

    case TAG_NEW_FLOAT_EXT: {
      // Value is stored as big-endian IEEE 64-bit float
      double val = codec::read_big_float64(ptr + 1);
      return std::make_pair((Py::Object)Py::Float(val), index + 9);
    }

    case TAG_BINARY_EXT: {
      constexpr auto PREFIX_SIZE = 5;
      if (input_length < PREFIX_SIZE) {
        return incomplete_data("decoding length for a binary");
      }

      auto len_expected = codec::read_big_u32(ptr + 1);

      return _decode_binary(index, options, ptr, input_length, len_expected,
                            PREFIX_SIZE, 8);
    }

    case TAG_BIT_BINARY_EXT: {
      constexpr auto PREFIX_SIZE = 6;
      if (input_length < PREFIX_SIZE) {
        return incomplete_data("decoding length for a bit-binary");
      }

      auto len_expected = codec::read_big_u32(ptr + 1);
      auto last_byte_bits = ptr[5];

      return _decode_binary(index, options, ptr, input_length, len_expected,
                            PREFIX_SIZE, last_byte_bits);
    }

    default:
      break;
  }

  return etf_error("Unknown tag %d", tag);
}

NativeETFModule::B2TResult
NativeETFModule::_decode_binary(size_t index, const B2TOptions& options,
                                const char* ptr, size_t input_length,
                                uint32_t len_expected, const int offset,
                                const int last_byte_bits) {
  if (len_expected + offset > input_length) {
    return incomplete_data("reading data for a binary");
  }

  Py::Bytes binary_data(ptr + offset, len_expected);

  if (options.simple_binaries) {
    return std::make_pair((Py::Object) binary_data,
                          index + offset + len_expected);
  }

  // Create a term.Binary object
  // Construct a term.Reference
  Py::Callable bin_class(term_mod_.getAttr("Binary"));
  auto ctor_args = Py::TupleN(binary_data,
                              Py::Long(last_byte_bits));
  return std::make_pair(bin_class.apply(ctor_args),
                        index + offset + len_expected);
}


NativeETFModule::B2TResult
NativeETFModule::_decode_ref(const std::string& input_str, size_t index,
                             const B2TOptions& options, const char* ptr,
                             size_t input_length) {
  if (input_length < 2) {
    return incomplete_data("decoding length for a new-ref");
  }

  const auto term_len = codec::read_big_u16(ptr + 1);

  Py::Object node;
  std::tie(node, index) = binary_to_term_native(input_str,
                                                index + 3, // skip the tag
                                                options);

  // Sanity check
  int id_data_byte_size = term_len * 4;
  if (input_length < index + id_data_byte_size + 1) {
    return incomplete_data("reading a new-ref");
  }

  ptr = input_str.data() + index;
  const uint8_t creation = (uint8_t) ptr[0];

  Py::Bytes id_data(ptr + 1, id_data_byte_size);

  // Construct a term.Reference
  Py::Callable ref_class(term_mod_.getAttr("Reference"));
  auto ctor_args = Py::TupleN(node,
                              Py::Long(creation),
                              id_data);
  return std::make_pair(ref_class.apply(ctor_args),
                        index + 1 +
                        id_data_byte_size); // skip creation, id_data
}


NativeETFModule::B2TResult
NativeETFModule::_decode_map(const std::string& input_str, size_t index,
                             const B2TOptions& options, const char* ptr,
                             size_t input_length) {
  if (input_length < 5) {
    return incomplete_data("decoding length for a map");
  }

  uint32_t len_expected = codec::read_big_u32(ptr + 1);

  Py::Dict result;
  index += 5; // skip the type byte and 4 bytes length

  for (uint32_t i = 0; i < len_expected; ++i) {
    Py::Object key, val;
    std::tie(key, index) = binary_to_term_native(input_str, index, options);
    std::tie(val, index) = binary_to_term_native(input_str, index, options);
    result.setItem(key, val);
  }

  return std::make_pair((Py::Object) result, index);
}


NativeETFModule::B2TResult
NativeETFModule::_decode_pid(const std::string& input_str, size_t index,
                             const B2TOptions& options, const char* ptr,
                             size_t input_length) {
  if (input_length < 10) {
    return incomplete_data("decoding ext pid");
  }
  Py::Object node;
  std::tie(node, index) = binary_to_term_native(input_str,
                                                index + 1, // skip the tag
                                                options);
  ptr = input_str.data() + index;
  uint32_t id = codec::read_big_u32(ptr);
  uint32_t serial = codec::read_big_u32(ptr + 4);
  uint8_t creation = (uint8_t) ptr[8];

  // Construct a term.Pid
  Py::Callable pid_class(term_mod_.getAttr("Pid"));
  auto ctor_args = Py::TupleN(node,
                              Py::Long((unsigned long) id),
                              Py::Long((unsigned long) serial),
                              Py::Long(creation));
  return std::make_pair(pid_class.apply(ctor_args),
                        index + 9); // skip id, serial, creation
}


NativeETFModule::B2TResult
NativeETFModule::_decode_tuple(const std::string& input_str, size_t index,
                               const B2TOptions& options, int tuple_size) {
  auto result = Py::Tuple(tuple_size); // Tuple ctor uses int, so we do too
  for (int i = 0; i < tuple_size; ++i) {
    Py::Object item;
    std::tie(item, index) = binary_to_term_native(input_str, index, options);
    result.setItem(i, item);
  }
  return std::make_pair((Py::Object) result, index);
}


NativeETFModule::B2TResult
NativeETFModule::_decode_list(const std::string& input_str, size_t index,
                              const B2TOptions& options, const char* ptr,
                              size_t input_length) {
  if (input_length < 5) {
    return incomplete_data("decoding length for a list");
  }

  uint32_t len_expected = codec::read_big_u32(ptr + 1);

  Py::List result;
  index += 5; // skip the type byte and 4 bytes length

  for (uint32_t i = 0; i < len_expected; ++i) {
    Py::Object item;
    std::tie(item, index) = binary_to_term_native(input_str, index, options);
    result.append(item);
  }

  // Read the tail
  Py::Object tail;
  std::tie(tail, index) = binary_to_term_native(input_str, index, options);

  if (options.simple_lists) {
    return std::make_pair((Py::Object) result, index);
  }

  // Construct a slower term.List which can represent tail as well
  Py::Callable list_class(term_mod_.getAttr("List"));
  Py::Object result_List = list_class.apply(Py::TupleN(result, tail));
  return std::make_pair(result_List, index);
}


NativeETFModule::B2TResult
NativeETFModule::_decode_small_atom(size_t index, const B2TOptions& options,
                                    const char* ptr, size_t input_length,
                                    char tag) {
  if (input_length < 2) {
    incomplete_data("decoding length for a small-atom name");
  }
  auto len_expected = codec::read_big_u16(ptr + 1);
  if (len_expected + 2 > input_length) {
    return incomplete_data("decoding text for a small-atom");
  }

  std::string name(ptr + 2, len_expected);
  auto enc = (tag == TAG_SMALL_ATOM_UTF8_EXT) ? "utf8" : "latin-1";

  return std::make_pair(str_to_atom(name, enc, options),
                        index + len_expected + 3);
}

NativeETFModule::B2TResult
NativeETFModule::_decode_atom(size_t index, const B2TOptions& options,
                              const char* ptr, size_t input_length, char tag) {
  if (input_length < 3) {
    return incomplete_data("decoding length for an atom name");
  }
  auto len_expected = codec::read_big_u16(ptr + 1);
  if (len_expected + 3 > input_length) {
    return incomplete_data("decoding text for an atom");
  }

  std::string name(ptr + 3, len_expected);
  auto enc = (tag == TAG_ATOM_UTF8_EXT) ? "utf8" : "latin-1";

  return std::make_pair(str_to_atom(name, enc, options),
                        index + len_expected + 3);
}


Py::Object NativeETFModule::py_term_to_binary_native(const Py::Tuple& args) {
  return Py::None();
}


NativeETFModule::B2TResult
NativeETFModule::etf_error(const char* format, ...) {
  char buffer[512]; // some generous large value
  va_list args;
  va_start (args, format);
  vsprintf(buffer, format, args);
  va_end (args);
  return etf_error(std::string(buffer));
}

Py::Object NativeETFModule::str_to_atom(const std::string& name,
                                        const std::string& encoding,
                                        const B2TOptions& options) {
  if (name == "true") {
    return Py::True();
  }
  if (name == "false") {
    return Py::False();
  }
  if (name == "undefined") {
    return Py::None();
  }
  if (options.atoms_as_strings) {
    return Py::String(name);
  }

  // Py::Dict this_mod(moduleDictionary());
  Py::Callable atom_class(term_mod_.getAttr("Atom"));
  auto args = Py::TupleN(Py::String(name), Py::String(encoding));
  return atom_class.apply(args);
}

} // ns pyrlang


extern "C" PyObject* PyInit_nativeETF() {
#if defined(PY_WIN32_DELAYLOAD_PYTHON_DLL)
  Py::InitialisePythonIndirectPy::Interface();
#endif

  static auto native_etf = new pyrlang::NativeETFModule;

  return native_etf->module().ptr();
}

// symbol required for the debug version
extern "C" PyObject* PyInit_nativeETF_d() {
  return PyInit_nativeETF();
}
