#pragma once

#include <memory>

// This is a kludge to get around SRecord's use of boost::shared_ptr, if the
// user does not have boost installed. Just pretend we have boost and alias
// boost::shared_ptr to its std replacement, until an upstream change to
// srecord to use std can happen.

namespace boost {
template<typename T>
using shared_ptr=  std::shared_ptr<T>;

}  // namespace boost