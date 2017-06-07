#pragma once

#ifdef _MSC_VER
// disable warning C4786: symbol greater than 255 character,
// nessesary to ignore as <map> causes lots of warning
#pragma warning(disable: 4786)
#endif

#include <Python.h>

#include "CXX/Objects.hxx"
#include "CXX/Extensions.hxx"

namespace pyrlang {

struct B2TOptions {
  bool simple_binaries = true;
  bool atoms_as_strings = false;
  bool simple_lists = true;

  void parse(const Py::Dict& pyopts);
};


class NativeETFModule : public Py::ExtensionModule<NativeETFModule> {
private:
  Py::Module term_mod_;

public:
  // Base must be called  with the name of file on disk e.g. XXX.so or XXX.pyd
  NativeETFModule()
    : Py::ExtensionModule<NativeETFModule>("nativeETF"),
      term_mod_("Pyrlang.term")
  {
    add_varargs_method("binary_to_term_native",
                       &NativeETFModule::py_binary_to_term_native,
                       "Decode bytes into a Python value, 131 tag header must "
                         "either be already stripped or not be preset");

    add_varargs_method("term_to_binary_native",
                       &NativeETFModule::py_term_to_binary_native,
                       "Encode to bytes but do not add the 131 tag header");

    // after initialize the moduleDictionary with exist
    initialize("documentation for the simple2 module");

//    Py::Dict d(moduleDictionary());
//    d["xxx"] = Py::asObject(new pysvn_enum<xxx_t>());
//    d["var"] = Py::String("var value");
  }

  virtual ~NativeETFModule() {}

private:
  using B2TResult = std::pair<Py::Object, size_t>;

  B2TResult etf_error(const char* format, ...);

  B2TResult etf_error(const std::string& what) {
//  Py::Dict this_module(moduleDictionary());
//  Py::Callable exception_class = this_module.getItem("ETFDecodeException");
//  Py::Object exception = exception_class.apply(Py::TupleN(Py::String(what)));
//  throw exception;
    throw Py::RuntimeError(what);
  }

  B2TResult incomplete_data(const char* what) {
    return etf_error("Incomplete data at %s", what);
  }

  Py::Object py_binary_to_term_native(const Py::Tuple& args);

  B2TResult
  binary_to_term_native(const std::string& data, size_t index,
                        const B2TOptions& options);

  Py::Object py_term_to_binary_native(const Py::Tuple& args);

  Py::Object str_to_atom(const std::string& basic_string,
                         const std::string& encoding,
                         const B2TOptions& options);

  B2TResult _decode_atom(size_t index,
                         const B2TOptions& options,
                         const char* ptr,
                         size_t input_length,
                         char tag);

  B2TResult
  _decode_small_atom(size_t index, const B2TOptions& options, const char* ptr,
                     size_t input_length, char tag);

  B2TResult _decode_list(const std::string& input_str, size_t index,
                         const B2TOptions& options, const char* ptr,
                         size_t input_length);

  B2TResult _decode_tuple(const std::string& input_str, size_t index,
                          const B2TOptions& options, int tuple_size);

  B2TResult _decode_pid(const std::string& input_str, size_t index,
                        const B2TOptions& options, const char* ptr,
                        size_t input_length);
};

//class ETFDecodeException: public Py::BaseException {
//public:
//  ETFDecodeException(const std::string& what): BaseException(what) {}
//};
//
//class ETFEncodeException: public Py::BaseException {
//public:
//  ETFDecodeException(const std::string& what): BaseException(what) {}
//};

static constexpr uint8_t ETF_VERSION_TAG = 131;

static constexpr uint8_t TAG_NEW_FLOAT_EXT = 70;
static constexpr uint8_t TAG_BIT_BINARY_EXT = 77;
static constexpr uint8_t TAG_COMPRESSED = 80;
static constexpr uint8_t TAG_SMALL_INT = 97;
static constexpr uint8_t TAG_INT = 98;
static constexpr uint8_t TAG_FLOAT_EXT = 99;
static constexpr uint8_t TAG_ATOM_EXT = 100;
static constexpr uint8_t TAG_PID_EXT = 103;
static constexpr uint8_t TAG_SMALL_TUPLE_EXT = 104;
static constexpr uint8_t TAG_LARGE_TUPLE_EXT = 105;
static constexpr uint8_t TAG_NIL_EXT = 106;
static constexpr uint8_t TAG_STRING_EXT = 107;
static constexpr uint8_t TAG_LIST_EXT = 108;
static constexpr uint8_t TAG_BINARY_EXT = 109;
static constexpr uint8_t TAG_NEW_FUN_EXT = 112;
static constexpr uint8_t TAG_NEW_REF_EXT = 114;
static constexpr uint8_t TAG_SMALL_ATOM_EXT = 115;
static constexpr uint8_t TAG_MAP_EXT = 116;
static constexpr uint8_t TAG_ATOM_UTF8_EXT = 118;
static constexpr uint8_t TAG_SMALL_ATOM_UTF8_EXT = 119;

} // ns pyrlang
