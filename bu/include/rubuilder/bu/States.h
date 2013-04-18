#ifndef _rubuilder_bu_States_h_
#define _rubuilder_bu_States_h_

#ifdef RUBUILDER_BOOST
#include "rubuilder/boost/statechart/custom_reaction.hpp"
#include "rubuilder/boost/statechart/event.hpp"
#include "rubuilder/boost/statechart/in_state_reaction.hpp"
#include "rubuilder/boost/statechart/state.hpp"
#include "rubuilder/boost/statechart/transition.hpp"
#else
#include <boost/statechart/custom_reaction.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/in_state_reaction.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#endif

#include <boost/mpl/list.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>

#include "xcept/Exception.h"
#include "xcept/tools.h"

#include <string>


namespace rubuilder { namespace bu { // namespace rubuilder::bu


  ///////////////////////////////////////////
  // Forward declarations of state classes //
  ///////////////////////////////////////////

  class Outermost;
  // Outer states:
  class Failed;
  class AllOk;
  // Inner states of AllOk
  class Halted;
  class Active;
  // Inner states of Active
  class Configuring;
  class Configured;
  class Processing;
  // Inner states of Configured
  class Clearing;
  class Ready;
  // Inner states of Processing
  class Enabled;


  ///////////////////
  // State classes //
  ///////////////////

  /**
   * The outermost state
   */
  class Outermost: public utils::RubuilderState<Outermost,StateMachine,AllOk>
  {

  public:

    typedef boost::mpl::list<
    boost::statechart::custom_reaction<BuConfirm>,
    boost::statechart::custom_reaction<BuCache>,
    boost::statechart::custom_reaction<BuAllocate>,
    boost::statechart::custom_reaction<BuCollect>,
    boost::statechart::custom_reaction<BuDiscard>,
    boost::statechart::custom_reaction<EvmLumisection>
     > reactions;

    Outermost(my_context c) : my_state("Outermost", c)
    { safeEntryAction(); }
    virtual ~Outermost()
    { safeExitAction(); }

    inline boost::statechart::result react(const BuConfirm& evt)
    {
      evt.getConfirmMsg()->release();
      LOG4CPLUS_WARN(outermost_context().getLogger(),
        "Discarding an I2O_BU_CONFIRM message.");
      return discard_event();
    }

    inline boost::statechart::result react(const BuCache& evt)
    {
      evt.getCacheMsg()->release();
      LOG4CPLUS_WARN(outermost_context().getLogger(),
        "Discarding an I2O_BU_CACHE message.");
      return discard_event();
    }

    inline boost::statechart::result react(const BuAllocate& evt)
    {
      evt.getAllocateMsg()->release();
      LOG4CPLUS_WARN(outermost_context().getLogger(),
        "Discarding an I2O_BU_ALLOCATE message.");
      return discard_event();
    }

    inline boost::statechart::result react(const BuCollect& evt)
    {
      evt.getCollectMsg()->release();
      LOG4CPLUS_WARN(outermost_context().getLogger(),
        "Discarding an I2O_BU_COLLECT message.");
      return discard_event();
    }

    inline boost::statechart::result react(const BuDiscard& evt)
    {
      evt.getDiscardMsg()->release();
      LOG4CPLUS_WARN(outermost_context().getLogger(),
        "Discarding an I2O_BU_DISCARD message.");
      return discard_event();
    }

    inline boost::statechart::result react(const EvmLumisection& evt)
    {
      evt.getLumisectionMsg()->release();
      LOG4CPLUS_WARN(outermost_context().getLogger(),
        "Discarding an I2O_EVM_LUMISECTION message.");
      return discard_event();
    }

  };


  /**
   * Failed state
   */
  class Failed: public utils::RubuilderState<Failed,Outermost>
  {

  public:

    typedef boost::mpl::list<
    boost::statechart::transition<utils::Fail,Failed>
    > reactions;

    Failed(my_context c) : my_state("Failed", c)
    { safeEntryAction(); }
    virtual ~Failed()
    { safeExitAction(); }

  };

  /**
   * The default state AllOk. Initial state of outer-state Outermost
   */
  class AllOk: public utils::RubuilderState<AllOk,Outermost,Halted>
  {

  public:

    typedef boost::mpl::list<
    boost::statechart::transition<utils::Fail,Failed,RSM,&StateMachine::failEvent>
    > reactions;

    AllOk(my_context c) : my_state("AllOk", c)
    { safeEntryAction(); }
    virtual ~AllOk()
    { safeExitAction(); }

  };


  /**
   * The Halted state. Initial state of outer-state AllOk.
   */
  class Halted: public utils::RubuilderState<Halted,AllOk>
  {

  public:

    typedef boost::mpl::list<
    boost::statechart::transition<utils::Configure,Active>
    > reactions;

    Halted(my_context c) : my_state("Halted", c)
    { safeEntryAction(); }
    virtual ~Halted()
    { safeExitAction(); }

  };


  /**
   * The Active state of outer-state AllOk.
   */
  class Active: public utils::RubuilderState<Active,AllOk,Configuring>
  {

  public:

    typedef boost::mpl::list<
    boost::statechart::transition<utils::Halt,Halted>
    > reactions;

    Active(my_context c) : my_state("Active", c)
    { safeEntryAction(); }
    virtual ~Active()
    { safeExitAction(); }

  };


  /**
   * The Configuring state. Initial state of outer-state Active.
   */
  class Configuring: public utils::RubuilderState<Configuring,Active>
  {

  public:

    typedef boost::mpl::list<
    boost::statechart::transition<utils::ConfigureDone,Configured>
    > reactions;

    Configuring(my_context c) : my_state("Configuring", c)
    { safeEntryAction(); }
    virtual ~Configuring()
    { safeExitAction(); }

    virtual void entryAction();
    virtual void exitAction();
    void activity();
    
  private:
    boost::scoped_ptr<boost::thread> configuringThread_;
    volatile bool doConfiguring_;

  };


  /**
   * The Configured state of the outer-state Active.
   */
  class Configured: public utils::RubuilderState<Configured,Active,Clearing>
  {

  public:

    Configured(my_context c) : my_state("Configured", c)
    { safeEntryAction(); }
    virtual ~Configured()
    { safeExitAction(); }

  };


  /**
   * The Processing state of the outer-state Active.
   */
  class Processing: public utils::RubuilderState<Processing,Active,Enabled>
  {

  public:

    typedef boost::mpl::list<
    boost::statechart::transition<utils::Stop,Configured>,
    boost::statechart::custom_reaction<BuConfirm>,
    boost::statechart::custom_reaction<BuCache>,
    boost::statechart::custom_reaction<BuAllocate>,
    boost::statechart::custom_reaction<BuCollect>,
    boost::statechart::custom_reaction<BuDiscard>,
    boost::statechart::custom_reaction<EvmLumisection>
    > reactions;
    
    Processing(my_context c) : my_state("Processing", c)
    { safeEntryAction(); }
    virtual ~Processing()
    { safeExitAction(); }

    virtual void entryAction();

    inline boost::statechart::result react(const BuConfirm& evt)
    {
      outermost_context().buConfirm(evt.getConfirmMsg());
      return discard_event();
    }

    inline boost::statechart::result react(const BuCache& evt)
    {
      outermost_context().buCache(evt.getCacheMsg());
      return discard_event();
    }

    inline boost::statechart::result react(const BuAllocate& evt)
    {
      outermost_context().buAllocate(evt.getAllocateMsg());
      return discard_event();
    }

    inline boost::statechart::result react(const BuCollect& evt)
    {
      outermost_context().buCollect(evt.getCollectMsg());
      return discard_event();
    }

    inline boost::statechart::result react(const BuDiscard& evt)
    {
      outermost_context().buDiscard(evt.getDiscardMsg());
      return discard_event();
    }

    inline boost::statechart::result react(const EvmLumisection& evt)
    {
      outermost_context().evmLumisection(evt.getLumisectionMsg());
      return discard_event();
    }

  };


  /**
   * The Clearing state. Initial state of outer-state Configured.
   */
  class Clearing: public utils::RubuilderState<Clearing,Configured>
  {

  public:

    typedef boost::mpl::list<
    boost::statechart::transition<utils::ClearDone,Ready>
    > reactions;

    Clearing(my_context c) : my_state("Clearing", c)
    { safeEntryAction(); }
    virtual ~Clearing()
    { safeExitAction(); }
    
    virtual void entryAction();
    virtual void exitAction();
    void activity();
    
  private:
    boost::scoped_ptr<boost::thread> clearingThread_;
    volatile bool doClearing_;
    
  };


  /**
   * The Ready state of outer-state Configured.
   */
  class Ready: public utils::RubuilderState<Ready,Configured>
  {

  public:

    typedef boost::mpl::list<
    boost::statechart::transition<utils::Enable,Processing>
    > reactions;

    Ready(my_context c) : my_state("Ready", c)
    { safeEntryAction(); }
    virtual ~Ready()
    { safeExitAction(); }

    virtual void entryAction()
    { outermost_context().notifyRCMS("Ready"); }

  };


  /**
   * The Enabled state. Initial state of the outer-state Processing.
   */
  class Enabled: public utils::RubuilderState<Enabled,Processing>
  {

  public:

    Enabled(my_context c) : my_state("Enabled", c)
    { safeEntryAction(); }
    virtual ~Enabled()
    { safeExitAction(); }

    virtual void entryAction();
    virtual void exitAction();

  };

  
} } //namespace rubuilder::bu

#endif //_rubuilder_bu_States_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
