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
  bool binaries_as_bytes = false;
  bool atoms_as_strings = false;

  void parse(const Py::Dict& pyopts);
};

class NativeETFModule : public Py::ExtensionModule<NativeETFModule> {
public:
  NativeETFModule()
    : Py::ExtensionModule<NativeETFModule>(
    "nativeETF") // this must be name of the file on disk e.g. XXX.so or XXX.pyd
  {
    add_varargs_method("binary_to_term",
                       &NativeETFModule::py_binary_to_term,
                       "Decode bytes with 131 tag byte into a Python value");

    add_varargs_method("binary_to_term_2",
                       &NativeETFModule::py_binary_to_term_2,
                       "Decode bytes into a Python value, 131 tag header must "
                         "either be already stripped or not be preset");

    add_varargs_method("term_to_binary",
                       &NativeETFModule::py_term_to_binary,
                       "Encode to bytes and add the 131 tag byte");

    add_varargs_method("term_to_binary_2",
                       &NativeETFModule::py_term_to_binary_2,
                       "Encode to bytes but do not add the 131 tag header");

    // after initialize the moduleDictionary with exist
    initialize("documentation for the simple2 module");

    Py::Dict d(moduleDictionary());
//    d["xxx"] = Py::asObject(new pysvn_enum<xxx_t>());
//    d["var"] = Py::String("var value");
  }

  virtual ~NativeETFModule() {}

private:
  Py::Object py_binary_to_term(const Py::Tuple& args);

  Py::Object py_binary_to_term_2(const Py::Tuple& args);

  Py::Object py_term_to_binary(const Py::Tuple& args) {
    return Py::None();
  }

  Py::Object py_term_to_binary_2(const Py::Tuple& args) {
    return Py::None();
  }
};

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
