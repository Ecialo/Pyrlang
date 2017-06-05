#include "library.h"
#include "byte_codec.h"

namespace pyrlang {


void B2TOptions::parse(const Py::Dict& pyopts) {
  if (pyopts.hasKey("binaries_as_bytes")) {
    binaries_as_bytes = pyopts.getItem("binaries_as_bytes").as_bool();
  }
  if (pyopts.hasKey("atoms_as_strings")) {
    atoms_as_strings = pyopts.getItem("atoms_as_strings").as_bool();
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
  options.parse(args[2]);

  // XXX: Possibly copying to a string can be avoided
  std::string input_str = Py::Bytes(args[0]).as_std_string();
  size_t index = (unsigned long) Py::Long(args[1]);
  const char* data = input_str.data() + index;
  size_t input_length = input_str.length() - index;

  char tag = data[0];
  switch (tag) {
    case TAG_ATOM_EXT:
    case TAG_ATOM_UTF8_EXT: {
      if (input_length < 3) {
        return incomplete_data("decoding length for an atom name");
      }
      auto len_expected = codec::read_big_u16(data + 1);
      if (len_expected + 3 > input_length) {
        return incomplete_data("decoding text for an atom");
      }

      std::string name(data + 3, len_expected);
      auto enc = (tag == TAG_ATOM_UTF8_EXT) ? "utf8" : "latin-1";
      return Py::TupleN(str_to_atom(name, enc, options),
                        Py::Long(index + len_expected + 3));
    }

    case TAG_SMALL_ATOM_EXT:
    case TAG_SMALL_ATOM_UTF8_EXT: {
      if (input_length < 2) {
        incomplete_data("decoding length for a small-atom name");
      }
      auto len_expected = codec::read_big_u16(data + 1);
      if (len_expected + 2 > input_length) {
        return incomplete_data("decoding text for a small-atom");
      }

      std::string name(data + 2, len_expected);
      auto enc = (tag == TAG_SMALL_ATOM_UTF8_EXT) ? "utf8" : "latin-1";
      return Py::TupleN(str_to_atom(name, enc, options),
                        Py::Long(index + len_expected + 3));
    }

    default:
      break;
  }

  return etf_error("Unknown tag %d", tag);
}

Py::Object NativeETFModule::py_term_to_binary_native(const Py::Tuple& args) {
  return Py::None();
}

Py::Object NativeETFModule::etf_error(const char* format, ...) {
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
