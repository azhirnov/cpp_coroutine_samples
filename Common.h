
#include <vector>
#include <string>
#include <iostream>

// defines '__cpp_impl_coroutine' and '__cpp_lib_coroutine'
#include <version>

#ifndef __cpp_impl_coroutine
#   error coroutines are not supported by compiler
#endif
#ifndef __cpp_lib_coroutine
#   error coroutines are not implemented in std
#endif

#include <coroutine>

#include <cassert>
