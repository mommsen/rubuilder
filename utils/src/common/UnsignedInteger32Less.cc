#include "rubuilder/utils/UnsignedInteger32Less.h"


bool rubuilder::utils::UnsignedInteger32Less::operator()
(
    const xdata::UnsignedInteger32 &ul1,
    const xdata::UnsignedInteger32 &ul2
)
const
{
    return ul1.value_ < ul2.value_;
}
