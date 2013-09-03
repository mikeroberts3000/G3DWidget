#ifndef QT_UTIL_HPP
#define QT_UTIL_HPP

#include "Assert.hpp"

#define MOJO_QT_SAFE( expression ) do { bool success = expression ; MOJO_RELEASE_ASSERT(success); } while (0)

#endif
