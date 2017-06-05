#include "library.h"

namespace pyrlang {


void B2TOptions::parse(const Py::Dict& pyopts) {
  if (pyopts.hasKey("binaries_as_bytes")) {
    binaries_as_bytes = pyopts.getItem("binaries_as_bytes").as_bool();
  }
  if (pyopts.hasKey("atoms_as_strings")) {
    atoms_as_strings = pyopts.getItem("atoms_as_strings").as_bool();
  }
}


Py::Object NativeETFModule::py_binary_to_term(const Py::Tuple& args) {
  args.verify_length(1, 2);

  // XXX: Possibly copying to a string can be avoided
  Py::Bytes data_b(args[0]);
  std::string data = data_b.as_std_string();

  std::cout << data.length() << " -- " << data << std::endl;

  if (data[0] != ETF_VERSION_TAG) {

  }

  return Py::None();
}

Py::Object NativeETFModule::py_binary_to_term_2(const Py::Tuple& args) {
  args.verify_length(1, 2);
  Py::Bytes data(args[0]);
  B2TOptions options;

  // Arg1 can be None or a dict with 'binaries_as_bytes' and
  // 'atoms_as_strings' optional bool keys.
  if (args.length() > 1 && not args[1].isNone()) {
    options.parse(args[1]);
  }

  return Py::None();
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
