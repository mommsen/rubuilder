#ifndef _rubuilder_rui_StateMachine_h_
#define _rubuilder_rui_StateMachine_h_

#include <boost/shared_ptr.hpp>

#ifdef RUBUILDER_BOOST
#include "rubuilder/boost/statechart/event_base.hpp"
#else
#include <boost/statechart/event_base.hpp>
#endif

#include "rubuilder/utils/Exception.h"
#include "rubuilder/utils/RubuilderStateMachine.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"


namespace rubuilder { namespace rui { // namespace rubuilder::rui
  
  class RUI;
  class StateMachine;
  class Outermost;

  ///////////////////////
  // The state machine //
  ///////////////////////

  typedef utils::RubuilderStateMachine<StateMachine,Outermost> RSM;
  class StateMachine: public RSM
  {
    
  public:
    
    StateMachine
    (
      xdaq::Application*,
      boost::shared_ptr<RUI>
    );

    boost::shared_ptr<RUI> rui() const { return rui_; }

  private:
    
    boost::shared_ptr<RUI> rui_;

  };
  
} } //namespace rubuilder::rui

#endif //_rubuilder_rui_StateMachine_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
