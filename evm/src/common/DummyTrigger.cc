#include "rubuilder/evm/TRGproxyHandlers.h"


void rubuilder::evm::TRGproxyHandlers::DummyTrigger::configure(const Configuration& conf)
{
  superFragmentGenerator_.configure(
    conf.fedSourceIds, conf.usePlayback, conf.playbackDataFile,
    conf.dummyBlockSize, conf.dummyFedPayloadSize,  conf.dummyFedPayloadStdDev);
}


bool rubuilder::evm::TRGproxyHandlers::DummyTrigger::getNextTrigger(toolbox::mem::Reference*& bufRef)
{
  return ( doRequestTriggers_ && superFragmentGenerator_.getData(bufRef) );
}


void rubuilder::evm::TRGproxyHandlers::DummyTrigger::stopRequestingTriggers()
{
  doRequestTriggers_ = false;
}


void rubuilder::evm::TRGproxyHandlers::DummyTrigger::reset()
{
  doRequestTriggers_ = true;
  superFragmentGenerator_.reset();
}

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
