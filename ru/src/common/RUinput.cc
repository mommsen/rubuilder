#include "rubuilder/ru/RUinput.h"
#include "rubuilder/utils/DumpUtility.h"
#include "rubuilder/utils/Exception.h"


rubuilder::ru::RUinput::RUinput
(
  xdaq::Application* app
) :
app_(app),
logger_(app->getApplicationLogger()),
handler_(new FBOproxy(app) ),
acceptI2Omessages_(false)
{
  resetMonitoringCounters();
}


void rubuilder::ru::RUinput::inputSourceChanged()
{
  const std::string inputSource = inputSource_.toString();

  LOG4CPLUS_INFO(logger_, "Setting input source to " + inputSource);

  if ( inputSource == "FBO" )
  {
    handler_.reset( new FBOproxy(app_) );
  }
  else if ( inputSource == "FEROL" )
  {
    handler_.reset( new FEROLproxy(app_) );
  }
  else if ( inputSource == "FEROL2" )
  {
    handler_.reset( new FEROL2proxy(app_) );
  }
  else if ( inputSource == "Local" )
  {
    handler_.reset( new DummyInputData(app_) );
  }
  else
  {
    XCEPT_RAISE(exception::Configuration,
      "Unknown input source " + inputSource + " requested.");
  }

  if ( inputSource != "Local" && generateDummySuperFragments_ )
  {
    XCEPT_RAISE(exception::Configuration,
      "Requested dummy data but input source is '" + 
      inputSource + "' instead of 'Local'");
  }

  configure();
  resetMonitoringCounters();
}


void rubuilder::ru::RUinput::I2Ocallback(toolbox::mem::Reference* bufRef)
{
  if ( acceptI2Omessages_ )
    handler_->I2Ocallback(bufRef);
}


bool rubuilder::ru::RUinput::getData
(
  const rubuilder::utils::EvBid& evbId,
  toolbox::mem::Reference*& bufRef
)
{
  if ( handler_->getData(evbId, bufRef) )
  {
    dumpFragmentToLogger(bufRef);
    return true;
  }
  return false;
}


void rubuilder::ru::RUinput::dumpFragmentToLogger(toolbox::mem::Reference* bufRef) const
{
  if ( ! dumpFragmentsToLogger_.value_ ) return;

  std::stringstream oss;
  utils::DumpUtility::dump(oss, bufRef);
  LOG4CPLUS_INFO(logger_, oss.str());
}


void rubuilder::ru::RUinput::configure()
{
  InputHandler::Configuration conf;
  conf.blockFIFOCapacity = blockFIFOCapacity_.value_;
  conf.dropInputData = dropInputData_.value_;
  conf.dummyBlockSize = dummyBlockSize_.value_;
  conf.dummyFedPayloadSize = dummyFedPayloadSize_.value_;
  conf.dummyFedPayloadStdDev = dummyFedPayloadStdDev_.value_;
  conf.fedSourceIds = fedSourceIds_;
  conf.usePlayback = usePlayback_.value_;
  conf.playbackDataFile = playbackDataFile_.value_;
  handler_->configure(conf);
}


void rubuilder::ru::RUinput::clear()
{
  acceptI2Omessages_ = false;
  handler_->clear();
}


void rubuilder::ru::RUinput::appendConfigurationItems(utils::InfoSpaceItems& params)
{
  blockFIFOCapacity_ = 65535;
  inputSource_ = "FBO";
  dumpFragmentsToLogger_ = false;
  dropInputData_ = false;
  generateDummySuperFragments_ = false;
  usePlayback_ = false;
  playbackDataFile_ = "";  
  dummyBlockSize_ = 4096;
  dummyFedPayloadSize_ = 2048;
  dummyFedPayloadStdDev_ = 0;
  
  // Default is 8 FEDs per super-fragment
  // Trigger has FED source id 0, RU0 has 1 to 8, RU1 has 9 to 16, etc.
  const uint32_t instance = app_->getApplicationDescriptor()->getInstance();
  const uint32_t firstSourceId = (instance * 8) + 1;
  const uint32_t lastSourceId  = (instance * 8) + 8;

  for (uint32_t sourceId=firstSourceId; sourceId<=lastSourceId; ++sourceId)
  {
    fedSourceIds_.push_back(sourceId);
  }
  
  inputParams_.clear();
  inputParams_.add("blockFIFOCapacity", &blockFIFOCapacity_);
  inputParams_.add("inputSource", &inputSource_, utils::InfoSpaceItems::change);
  inputParams_.add("dumpFragmentsToLogger", &dumpFragmentsToLogger_);
  inputParams_.add("dropInputData", &dropInputData_);
  inputParams_.add("generateDummySuperFragments", &generateDummySuperFragments_);
  inputParams_.add("usePlayback", &usePlayback_);
  inputParams_.add("playbackDataFile", &playbackDataFile_);
  inputParams_.add("dummyBlockSize", &dummyBlockSize_);
  inputParams_.add("dummyFedPayloadSize", &dummyFedPayloadSize_);
  inputParams_.add("dummyFedPayloadStdDev", &dummyFedPayloadStdDev_);
  inputParams_.add("fedSourceIds", &fedSourceIds_);

  params.add(inputParams_);
}


void rubuilder::ru::RUinput::appendMonitoringItems(utils::InfoSpaceItems& items)
{
  lastEventNumberFromRUI_ = 0;
  i2oEVMRUDataReadyCount_ = 0;

  items.add("lastEventNumberFromRUI", &lastEventNumberFromRUI_);
  items.add("i2oEVMRUDataReadyCount", &i2oEVMRUDataReadyCount_);
}


void rubuilder::ru::RUinput::updateMonitoringItems()
{
  lastEventNumberFromRUI_ = handler_->lastEventNumber();
  i2oEVMRUDataReadyCount_ = handler_->fragmentsCount();
}


void rubuilder::ru::RUinput::resetMonitoringCounters()
{
  handler_->resetMonitoringCounters();
}


uint64_t rubuilder::ru::RUinput::fragmentsCount() const
{
  return handler_->fragmentsCount();
}


void rubuilder::ru::RUinput::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>RUinput - " << inputSource_.toString() << "</p>"    << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;

  handler_->printHtml(out);

  inputParams_.printHtml("Configuration", out);

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
