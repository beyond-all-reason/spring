/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "gflags/gflags.h"

/* extended define options for gflags; previously carried in a modified gflags tree */

#define DEFINE_VARIABLE_EX(type, shorttype, name, external_name, value, help) \
  namespace fL##shorttype {                                                   \
    static const type FLAGS_nono##name = value;                               \
    /* We always want to export defined variables, dll or no */               \
    GFLAGS_DLL_DEFINE_FLAG type FLAGS_##name = FLAGS_nono##name;              \
    type FLAGS_no##name = FLAGS_nono##name;                                   \
    static GFLAGS_NAMESPACE::FlagRegisterer o_##name(                         \
      external_name, MAYBE_STRIPPED_HELP(help), __FILE__,                     \
      &FLAGS_##name, &FLAGS_no##name);                                        \
  }                                                                           \
  using fL##shorttype::FLAGS_##name

#define DEFINE_bool_EX(name, external_name, val, txt)                   \
  namespace fLB {                                                       \
    typedef ::fLB::CompileAssert FLAG_##name##_value_is_not_a_bool[     \
            (sizeof(::fLB::IsBoolFlag(val)) != sizeof(double))? 1: -1]; \
  }                                                                     \
  DEFINE_VARIABLE_EX(bool, B, name, external_name, val, txt)


#define DEFINE_int32_EX(name, val, txt) \
   DEFINE_VARIABLE_EX(GFLAGS_NAMESPACE::int32, I, \
                   name, external_name, val, txt)

#define DEFINE_uint32_EX(name,val, txt) \
   DEFINE_VARIABLE_EX(GFLAGS_NAMESPACE::uint32, U, \
                   name, external_name, val, txt)

#define DEFINE_int64_EX(name, val, txt) \
   DEFINE_VARIABLE_EX(GFLAGS_NAMESPACE::int64, I64, \
                   name, external_name, val, txt)

#define DEFINE_uint64_EX(name,val, txt) \
   DEFINE_VARIABLE_EX(GFLAGS_NAMESPACE::uint64, U64, \
                   name, external_name, val, txt)

#define DEFINE_double_EX(name, val, txt) \
   DEFINE_VARIABLE_EX(double, D, name, external_name, val, txt)

#define DEFINE_string_EX(name, external_name, val, txt)                     \
  namespace fLS {                                                           \
    using ::fLS::clstring;                                                  \
    using ::fLS::StringFlagDestructor;                                      \
    static union { void* align; char s[sizeof(clstring)]; } s_##name[2];    \
    clstring* const FLAGS_no##name = ::fLS::                                \
                                   dont_pass0toDEFINE_string(s_##name[0].s, \
                                                             val);          \
    static GFLAGS_NAMESPACE::FlagRegisterer o_##name(                       \
        external_name, MAYBE_STRIPPED_HELP(txt), __FILE__,                  \
        FLAGS_no##name, new (s_##name[1].s) clstring(*FLAGS_no##name));     \
    static StringFlagDestructor d_##name(s_##name[0].s, s_##name[1].s);     \
    extern GFLAGS_DLL_DEFINE_FLAG clstring& FLAGS_##name;                   \
    using fLS::FLAGS_##name;                                                \
    clstring& FLAGS_##name = *FLAGS_no##name;                               \
  }                                                                         \
  using fLS::FLAGS_##name
