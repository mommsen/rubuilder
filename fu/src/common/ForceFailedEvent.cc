#include "rubuilder/fu/ForceFailedEvent.h"


rubuilder::fu::ForceFailedEvent::ForceFailedEvent
(
    const std::string &reason,
    void              *originator
) :
toolbox::Event("Fail", originator)
{
    reason_ = reason;
}


std::string rubuilder::fu::ForceFailedEvent::getReason()
{
    return reason_;
}
