#include "rubuilder/fu/SynchronizedString.h"


rubuilder::fu::SynchronizedString::SynchronizedString() :
bSem_(toolbox::BSem::FULL)
{
}


void rubuilder::fu::SynchronizedString::setValue(std::string value)
{
    bSem_.take();
    value_ = value;
    bSem_.give();
}


void rubuilder::fu::SynchronizedString::setValue(const char *value)
{
    bSem_.take();
    value_ = value;
    bSem_.give();
}


std::string rubuilder::fu::SynchronizedString::getValue()
{
    bSem_.take();
    std::string value = value_;
    bSem_.give();

    return value;
}
