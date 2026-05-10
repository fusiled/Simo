#include "Simo/core/Time.h"

#include <iostream>

namespace Simo {
Time Time::zero = Time(0);
Time Time::one = Time(1);

SIMO_PUBLIC std::ostream& operator<<(std::ostream& out, const Time& e) {
  out << "Time( " << e.to_picoseconds() << " ps)";
  return out;  // Return the stream to allow chaining
}
}  // namespace Simo