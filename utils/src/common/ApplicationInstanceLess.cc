#include "rubuilder/utils/ApplicationInstanceLess.h"


bool rubuilder::utils::ApplicationInstanceLess::operator()
(
    xdaq::ApplicationDescriptor *a1,
    xdaq::ApplicationDescriptor *a2
)
const
{
    if(a1->hasInstanceNumber() && a2->hasInstanceNumber())
    {
        return a1->getInstance() < a2->getInstance();
    }
    else
    {
        return false;
    }
}
