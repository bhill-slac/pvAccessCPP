#ifndef SYNCTESTREQUESTERS_HPP
#define SYNCTESTREQUESTERS_HPP


#include <pv/pvAccess.h>
#include <pv/event.h>

using namespace epics::pvAccess;
using namespace epics::pvData;
using namespace std;
using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;


class SyncBaseRequester {

  public:

    bool waitUntilGetDone(double timeOut)
    {

      bool signaled = waitUntilEvent(timeOut);
      if (!signaled)
        return false;

      Lock lock(m_getStatusMutex);
      return m_getStatus;
    }


    bool waitUntilConnected(double timeOut)
    {

      bool signaled = waitUntilEvent(timeOut);
      if (!signaled)
        return false;

      Lock lock(m_connectedStatusMutex);
      return m_connectedStatus;
    }

    virtual ~SyncBaseRequester() {} ;

  
  protected:

    const bool m_debug; 

    SyncBaseRequester(bool debug = true):
      m_debug(debug), 
      m_connectedStatus(false), 
      m_getStatus(false),
      m_putStatus(false) {}


    bool waitUntilPutDone(double timeOut)
    {

      bool signaled = waitUntilEvent(timeOut);
      if (!signaled)
        return false;

      Lock lock(m_putStatusMutex);
      return m_putStatus;
    }


    bool waitUntilProcessDone(double timeOut)
    {

      bool signaled = waitUntilEvent(timeOut);
      if (!signaled)
        return false;

      Lock lock(m_processStatusMutex);
      return m_processStatus;
    }


    void setConnectedStatus(bool status) {
      Lock lock(m_connectedStatusMutex);
      m_connectedStatus = status;
    }


    bool getConnectedStatus() {
      Lock lock(m_connectedStatusMutex);
      return m_connectedStatus;
    }


    void setGetStatus(bool status) {
      Lock lock(m_getStatusMutex);
      m_getStatus = status;
    }


    bool getGetStatus() {
      Lock lock(m_getStatusMutex);
      return m_getStatus;
    }


    void setPutStatus(bool status) {
      Lock lock(m_putStatusMutex);
      m_putStatus = status;
    }


    bool getPutStatus() {
      Lock lock(m_putStatusMutex);
      return m_putStatus;
    }


    void setProcessStatus(bool status) {
      Lock lock(m_processStatusMutex);
      m_processStatus = status;
    }


    bool getProcessStatus() {
      Lock lock(m_processStatusMutex);
      return m_processStatus;
    }


    void signalEvent() {
      m_event.signal();
    }


    bool waitUntilEvent(double timeOut)
    {
      bool signaled = m_event.wait(timeOut);
      if (!signaled)
      {
        if (m_debug)
          std::cerr  << "wait until event timeout" << std::endl;

        return false;
      }

      return true;
    }

  private:

    epics::pvData::Event m_event;
    bool m_connectedStatus;
    bool m_getStatus;
    bool m_putStatus;
    bool m_processStatus;
    Mutex m_connectedStatusMutex;
    Mutex m_getStatusMutex;
    Mutex m_putStatusMutex;
    Mutex m_processStatusMutex;
};


class SyncChannelRequesterImpl : public epics::pvAccess::ChannelRequester, public SyncBaseRequester
{
  public:

    SyncChannelRequesterImpl(bool debug = true): SyncBaseRequester(debug),  
    m_createdCount(0), m_stateChangeCount(0) {}


    bool waitUntilStateChange(double timeOut) 
    {
      return waitUntilEvent(timeOut);
    }


    int getCreatedCount() {
      Lock lock(m_pointerMutex);
      return m_createdCount;
    }


    int getStateChangeCount() {
      Lock lock(m_pointerMutex);
      return m_stateChangeCount;
    }


    Status getStatus() {
      Lock lock(m_pointerMutex);
      return m_status;
    }


    virtual string getRequesterName() 
    {
      return "SyncChannelRequesterImpl";
    }


    virtual void message(string const &, epics::pvData::MessageType ) 
    {
    }


    virtual void channelCreated(
        const epics::pvData::Status& status, 
        epics::pvAccess::Channel::shared_pointer const & channel) 
    {
      if (m_debug)
        std::cout << getRequesterName() << "." << "channelCreated(" << status.toString() << ")" << std::endl;

      Lock lock(m_pointerMutex);
      m_status = status;

      if (status.isSuccess())
      {
        m_createdCount++;
      }
      else
      {
        if (m_debug)
          std::cerr << "[" << channel->getChannelName() << "] failed to create a channel: " << std::endl;
      }
    }


    virtual void channelStateChange(
        epics::pvAccess::Channel::shared_pointer const & , 
        epics::pvAccess::Channel::ConnectionState connectionState) 
    {

      if (m_debug)
        std::cout << getRequesterName() << "." << "channelStateChange:" << connectionState << std::endl;

      {
        Lock lock(m_pointerMutex);
        m_stateChangeCount++;
      }

      signalEvent();
    }

  private:
    Mutex m_pointerMutex;
    int m_createdCount;
    int m_stateChangeCount;
    Status m_status;
};


class SyncChannelFindRequesterImpl : public ChannelFindRequester, public SyncBaseRequester
{
  public:

    SyncChannelFindRequesterImpl(bool debug = true): SyncBaseRequester(debug), m_isFound(false) {}


    bool waitUntilFindResult(double timeOut) 
    {
      return waitUntilEvent(timeOut);
    }


    Status getStatus() {
      Lock lock(m_pointerMutex);
      return m_status;
    }


    bool isChannelFound() {
      Lock lock(m_pointerMutex);
      return m_isFound;
    }


    virtual void channelFindResult(const epics::pvData::Status& status,
        const epics::pvAccess::ChannelFind::shared_pointer&, bool wasFound)
    {
      if (m_debug)
        std::cout << "channelFindResult(" << status.toString() << ")" << std::endl;

      {
        Lock lock(m_pointerMutex);
        m_status = status;
        m_isFound = wasFound;
      }

      signalEvent();
    }

  private:
    Mutex m_pointerMutex;
    Status m_status;
    bool m_isFound;
};


class SyncChannelGetRequesterImpl : public ChannelGetRequester, public SyncBaseRequester
{
  public:

    typedef std::tr1::shared_ptr<SyncChannelGetRequesterImpl> shared_pointer; 


    SyncChannelGetRequesterImpl(string channelName, bool debug = true): 
      SyncBaseRequester(debug),
      m_channelName(channelName)  {}


    bool syncGet(bool lastRequest, long timeOut) {
      m_channelGet->get(lastRequest);
      return waitUntilGetDone(timeOut);
    }


    PVStructure::shared_pointer getPVStructure()
    {
      Lock lock(m_pointerMutex);
      return m_pvStructure;
    }


    BitSet::shared_pointer getBitSet() {
      Lock lock(m_pointerMutex);
      return m_bitSet;
    }


    ChannelGet::shared_pointer getChannelGet() {
      Lock lock(m_pointerMutex);
      return m_channelGet;
    }


    virtual string getRequesterName()
    {
      return "SyncChannelGetRequesterImpl";
    }


    virtual void message(string const & message, MessageType messageType)
    {
      if (m_debug)
        std::cerr << "[" 
          << getRequesterName() 
          << "] message(" 
          << message << ", " 
          << getMessageTypeName(messageType) 
          << ")" 
          << std::endl;
    }


    virtual void channelGetConnect(
        const epics::pvData::Status& status,ChannelGet::shared_pointer const & channelGet,
        epics::pvData::PVStructure::shared_pointer const & pvStructure, 
        epics::pvData::BitSet::shared_pointer const & bitSet)
    {
      if (m_debug)
        std::cout << getRequesterName() << "." << "channelGetConnect(" << status.toString() << ")" << std::endl;

      if (status.isSuccess())
      {
        {
          Lock lock(m_pointerMutex);
          m_channelGet = channelGet;
          m_pvStructure = pvStructure;
          m_bitSet = bitSet;
        }
        channelGet->get(false);
      }
      else
      {
        signalEvent();
      }
    }


    virtual void getDone(const epics::pvData::Status& status)
    {
      if (m_debug)
        std::cout << getRequesterName() << "." << "getDone(" << status.toString() << ")" << std::endl;

      setGetStatus(status.isSuccess());
      signalEvent();
    }


  private:
    Mutex m_pointerMutex;
    ChannelGet::shared_pointer m_channelGet;
    PVStructure::shared_pointer m_pvStructure;
    BitSet::shared_pointer m_bitSet;
    string m_channelName;
};



class SyncChannelPutRequesterImpl : public ChannelPutRequester, public SyncBaseRequester
{
  public: 

    typedef std::tr1::shared_ptr<SyncChannelPutRequesterImpl> shared_pointer; 

    SyncChannelPutRequesterImpl(string const & channelName, bool debug = true): 
      SyncBaseRequester(debug), m_channelName(channelName) {}  


    bool syncPut(bool lastRequest, long timeOut) 
    {

      if (!getConnectedStatus()) {
        return false;
      }

      m_channelPut->put(lastRequest);
      return waitUntilPutDone(timeOut);
    } 


    bool syncGet(long timeOut) 
    {

      if (!getConnectedStatus()) {
        return false;
      }

      m_channelPut->get();
      return waitUntilGetDone(timeOut);
    } 


    PVStructure::shared_pointer getPVStructure()
    {
      Lock lock(m_pointerMutex);
      return m_pvStructure;
    }


    BitSet::shared_pointer getBitSet() {
      Lock lock(m_pointerMutex);
      return m_bitSet;
    }


    ChannelPut::shared_pointer getChannelPut() {
      Lock lock(m_pointerMutex);
      return m_channelPut;
    }


    virtual string getRequesterName()
    {
      return "SyncChannelPutRequesterImpl";
    }


    virtual void message(string const & message,MessageType messageType)
    {
      if (m_debug)
        std::cout << "[" << getRequesterName() << "] message(" << message << ", " 
          << getMessageTypeName(messageType) << ")" << std::endl;
    }


    virtual void channelPutConnect(const epics::pvData::Status& status,
        ChannelPut::shared_pointer const & channelPut,
        epics::pvData::PVStructure::shared_pointer const & pvStructure,
        epics::pvData::BitSet::shared_pointer const & bitSet)
    {

      if (m_debug)
        std::cout << getRequesterName() << "." << "channelPutConnect(" << status.toString() << ")" << std::endl;

      if (status.isSuccess())
      {

        {
          Lock lock(m_pointerMutex);
          m_channelPut = channelPut;
          m_pvStructure = pvStructure;
          m_bitSet = bitSet;
        }
        setConnectedStatus(true);
      }
      else
      {
        setConnectedStatus(false);
      }

      signalEvent();
    }


    virtual void getDone(const epics::pvData::Status& status)
    {
      if (m_debug)
        std::cout << getRequesterName() << "." << "getDone(" << status.toString() << ")" << std::endl;

      setGetStatus(status.isSuccess());
      signalEvent();      
    }


    virtual void putDone(const epics::pvData::Status& status)
    {
      if (m_debug)
        std::cout << getRequesterName() << "." << "putDone(" << status.toString() << ")" << std::endl;

      setPutStatus(status.isSuccess()); 
      signalEvent();
    }


  private:

    Mutex m_pointerMutex;
    ChannelPut::shared_pointer m_channelPut;
    epics::pvData::PVStructure::shared_pointer m_pvStructure;
    epics::pvData::BitSet::shared_pointer m_bitSet;
    string m_channelName; 
};


class SyncGetFieldRequesterImpl : public GetFieldRequester, public SyncBaseRequester
{
  public:

    typedef std::tr1::shared_ptr<SyncChannelPutRequesterImpl> shared_pointer; 


    SyncGetFieldRequesterImpl(bool debug = true): SyncBaseRequester(debug) {}


    FieldConstPtr getField() {
      Lock lock(m_pointerMutex);
      return m_field;  
    }


    virtual string getRequesterName()
    {
      return "SyncGetFieldRequesterImpl";
    };


    virtual void message(string const & message,MessageType /*messageType*/)
    {
      if (m_debug)
        std::cout << "[" << getRequesterName() << "] message(" << message << endl;
    }


    virtual void getDone(const epics::pvData::Status& status,epics::pvData::FieldConstPtr const & field)
    {

      if (m_debug)
        std::cout << getRequesterName() << "." << "getDone(" << status.toString() << endl;

      if (status.isSuccess() && field)
      {
        {
          Lock lock(m_pointerMutex);
          m_field = field;
        }
        setGetStatus(true);
      }
      else {
        setGetStatus(false);
      }

      signalEvent();
    }

  private:
    Mutex m_pointerMutex;
    FieldConstPtr m_field;
};


class SyncChannelProcessRequesterImpl : public ChannelProcessRequester, public SyncBaseRequester
{
  public:

    typedef std::tr1::shared_ptr<SyncChannelPutRequesterImpl> shared_pointer; 


    SyncChannelProcessRequesterImpl(bool debug = true): SyncBaseRequester(debug) {}


    bool syncProcess(bool lastRequest, double timeOut) {
      if(!getConnectedStatus()) {
        return false;
      }

      m_channelProcess->process(lastRequest);
      return waitUntilProcessDone(timeOut);
    }


    ChannelProcess::shared_pointer getChannelProcess() {
      Lock lock(m_pointerMutex);
      return m_channelProcess;
    }


    virtual string getRequesterName()
    {
      return "ProcessRequesterImpl";
    };


    virtual void message(string const & message,MessageType /*messageType*/)
    {
      if (m_debug)
        std::cout << "[" << getRequesterName() << "] message(" << message << std::endl;
    }


    virtual void channelProcessConnect(const epics::pvData::Status& status,
        ChannelProcess::shared_pointer const & channelProcess)
    {

      if (m_debug)
        std::cout << getRequesterName() << "." << "channelProcessConnect(" << status.toString() << ")" << std::endl;

      if (status.isSuccess())
      {
        {
          Lock lock(m_pointerMutex);
          m_channelProcess = channelProcess;
        }
        setConnectedStatus(true);
      }
      else
      {
        setConnectedStatus(false);
      }

      signalEvent();
    }


    virtual void processDone(const epics::pvData::Status& status)
    {
      if (m_debug)
        std::cout << getRequesterName() << "." << "processDone(" << status.toString() << ")" << std::endl;

      setProcessStatus(status.isSuccess());
      signalEvent();
    }


  private:
    Mutex m_pointerMutex;
    ChannelProcess::shared_pointer m_channelProcess;
};


class SyncChannelPutGetRequesterImpl : public ChannelPutGetRequester, public SyncBaseRequester
{
  public:

    typedef std::tr1::shared_ptr<SyncChannelPutGetRequesterImpl> shared_pointer; 


    SyncChannelPutGetRequesterImpl(bool debug = true):
      SyncBaseRequester(debug),  
      m_putGetStatus(false),
      m_getPutStatus(false),
      m_getGetStatus(false) {}


    bool syncGetPut(double timeOut) {

      if(!getConnectedStatus()) {
        return false;
      }

      m_channelPutGet->getPut();
      return waitUntilGetPutDone(timeOut);
    }


    bool syncPutGet(bool lastRequest, double timeOut) {

      if(!getConnectedStatus()) {
        return false;
      }

      m_channelPutGet->putGet(lastRequest);
      return waitUntilPutGetDone(timeOut);
    }


    bool syncGetGet(double timeOut) {

      if(!getConnectedStatus()) {
        return false;
      }

      m_channelPutGet->getGet();
      return waitUntilGetGetDone(timeOut);
    }


    PVStructure::shared_pointer getPVPutStructure()
    {
      Lock lock(m_pointerMutex);
      return m_putData;
    }


    PVStructure::shared_pointer getPVGetStructure()
    {
      Lock lock(m_pointerMutex);
      return m_getData;
    }


    ChannelPutGet::shared_pointer getChannelPutGet()
    {
      Lock lock(m_pointerMutex);
      return m_channelPutGet;
    }


    virtual string getRequesterName()
    {
      return "SyncChannelGetPutRequesterImpl";
    };


    virtual void message(string const & message,MessageType messageType)
    {
      if (m_debug)
        std::cout << "[" << getRequesterName() << "] message(" << 
          message << ", " << getMessageTypeName(messageType) << ")" << std::endl;
    }


    virtual void channelPutGetConnect(const epics::pvData::Status& status, 
        ChannelPutGet::shared_pointer const & channelPutGet,
        epics::pvData::PVStructure::shared_pointer const & putData,
        epics::pvData::PVStructure::shared_pointer const & getData)
    {
      if (m_debug)
        std::cout << getRequesterName() << "." << "channelGetPutConnect(" 
          << status.toString() << ")" << std::endl;

      if (status.isSuccess())
      {
        {
          Lock lock(m_pointerMutex);
          m_channelPutGet = channelPutGet;
          m_putData = putData;
          m_getData = getData;
        }
        setConnectedStatus(true);
      }
      else
      {
        setConnectedStatus(false);
      }

      signalEvent();     
    }


    virtual void getGetDone(const epics::pvData::Status& status)
    {
      if (m_debug)
        std::cout << getRequesterName() << "." << "getGetDone(" << status.toString() << ")" << std::endl;

      {
        Lock lock(m_pointerMutex);
        m_getGetStatus = status.isSuccess();
      }

      signalEvent();      
    }


    virtual void getPutDone(const epics::pvData::Status& status)
    {
      if (m_debug)
        std::cout << getRequesterName() << "." << "getPutDone(" << status.toString() << ")" << std::endl;

      {
        Lock lock(m_pointerMutex);
        m_getPutStatus = status.isSuccess();
      }

      signalEvent();
    }


    virtual void putGetDone(const epics::pvData::Status& status)
    {
      if (m_debug)
        std::cout << getRequesterName() << "." << "putGetDone(" << status.toString() << ")" << std::endl;

      {
        Lock lock(m_pointerMutex);
        m_putGetStatus = status.isSuccess();
      }

      signalEvent();
    }


  private:

    bool waitUntilPutGetDone(double timeOut)
    {

      bool signaled = waitUntilEvent(timeOut);
      if (!signaled)
        return false;

      Lock lock(m_pointerMutex);
      return m_putGetStatus;
    }


    bool waitUntilGetPutDone(double timeOut)
    {

      bool signaled = waitUntilEvent(timeOut);
      if (!signaled)
        return false;

      Lock lock(m_pointerMutex);
      return m_getPutStatus;
    }


    bool waitUntilGetGetDone(double timeOut)
    {

      bool signaled = waitUntilEvent(timeOut);
      if (!signaled)
        return false;

      Lock lock(m_pointerMutex);
      return m_getGetStatus;
    }


    bool m_putGetStatus;
    bool m_getPutStatus;
    bool m_getGetStatus;

    Mutex m_pointerMutex;
    ChannelPutGet::shared_pointer m_channelPutGet;
    epics::pvData::PVStructure::shared_pointer m_putData;
    epics::pvData::PVStructure::shared_pointer m_getData;
};


class SyncChannelRPCRequesterImpl : public ChannelRPCRequester, public SyncBaseRequester
{

  public:

    typedef std::tr1::shared_ptr<SyncChannelRPCRequesterImpl> shared_pointer; 


    SyncChannelRPCRequesterImpl(bool debug = true) : SyncBaseRequester(debug),  m_done(false) {}


    bool syncRPC( epics::pvData::PVStructure::shared_pointer const & pvArguments, 
        boolean lastRequest, long timeOut) {

      if(!getConnectedStatus()) {
        return false;
      }

      m_channelRPC->request(pvArguments, lastRequest);
      return waitUntilRPC(timeOut);
    }   


    PVStructure::shared_pointer getLastResponse()
    {
      Lock lock(m_pointerMutex);
      return m_lastResponse;
    }


    ChannelRPC::shared_pointer getChannelRPC() {
      Lock lock(m_pointerMutex);
      return m_channelRPC;
    }


    virtual string getRequesterName()
    {
      return "SyncChannelRPCRequesterImpl";
    }


    virtual void message(string const & message, MessageType messageType)
    {
      if (m_debug)
        std::cerr << "[" << getRequesterName() << "] message(" << message << ", " 
          << getMessageTypeName(messageType) << ")" << std::endl;
    }


    virtual void channelRPCConnect(const epics::pvData::Status& status,
        ChannelRPC::shared_pointer const & channelRPC)
    {

      if (m_debug)
        std::cout << getRequesterName() << "." << "channelRPCConnect(" 
          << status.toString() << ")" << std::endl;

      if (status.isSuccess())
      {
        {
          Lock lock(m_pointerMutex);
          m_channelRPC = channelRPC;
        }

        setConnectedStatus(true);
      }
      else
      {
        setConnectedStatus(false);
      }

      signalEvent();
    }


    virtual void requestDone (const epics::pvData::Status &status, 
        epics::pvData::PVStructure::shared_pointer const &pvResponse)
    {

      if (m_debug)
        std::cout << getRequesterName() << "." << "requestDone(" 
          << status.toString() << ")" << std::endl;

      if (status.isSuccess())
      {
        Lock lock(m_pointerMutex);
        m_lastResponse = pvResponse;
      }

      {
        Lock lock(m_pointerMutex);
        m_done = status.isSuccess();
      }

      signalEvent();
    }


  private:

    bool waitUntilRPC(double timeOut)
    {
      bool signaled = waitUntilEvent(timeOut);
      if (!signaled)
      {
        return false;
      }

      Lock lock(m_pointerMutex);
      return m_done;
    }

    bool m_done;
    ChannelRPC::shared_pointer m_channelRPC;
    Mutex m_pointerMutex;
    PVStructure::shared_pointer m_lastResponse;
};


class SyncMonitorRequesterImpl: public MonitorRequester, public SyncBaseRequester
{
  public:

    typedef std::tr1::shared_ptr<SyncMonitorRequesterImpl> shared_pointer; 


    SyncMonitorRequesterImpl(bool debug = true): 
      SyncBaseRequester(debug),
      m_monitorCounter(0), 
      m_monitorStatus(false) {}


    int getMonitorCounter() {
      Lock lock(m_pointerMutex);
      return m_monitorCounter;
    }


    PVStructure::shared_pointer getPVStructure() {
      Lock lock(m_pointerMutex);
      return m_pvStructure;
    }



    std::tr1::shared_ptr<Monitor> getChannelMonitor() {
      Lock lock(m_pointerMutex);
      return m_monitor;
    }


    BitSet::shared_pointer getChangedBitSet() {
      Lock lock(m_pointerMutex);
      return m_changedBitSet;
    } 


    BitSet::shared_pointer getOverrunBitSet() {
      Lock lock(m_pointerMutex);
      return m_overrunBitSet;
    } 


    bool waitUntilMonitor(double timeOut)
    {
      {
        Lock lock(m_pointerMutex);
        m_monitorStatus = false;
      }


      bool signaled = waitUntilEvent(timeOut);
      if (!signaled) {

        if (m_debug)
          std::cerr << getRequesterName() << ".waitUntilMonitor:" << " timeout occurred" << endl;  

        return false;
      }

      Lock lock(m_pointerMutex);
      return m_monitorStatus;
    }


    virtual string getRequesterName()
    {
      return "SyncMonitorRequesterImpl";
    }


    virtual void message(string const & message, MessageType messageType)
    {
      if (m_debug)
        std::cerr << "[" << getRequesterName() << "] message(" << message << ", " 
          << getMessageTypeName(messageType) << ")" << std::endl;
    }


    virtual void monitorConnect(const epics::pvData::Status& status, Monitor::shared_pointer const & monitor, 
        StructureConstPtr const & /*structure*/)
    {
      if (m_debug)
        std::cout << getRequesterName() << "." << "monitorConnect(" << status.toString() << ")" << std::endl;

      if (status.isSuccess())
      {
        {
          Lock lock(m_pointerMutex);
          m_monitor = monitor;
        }

        setConnectedStatus(true);
      }
      else
      {
        setConnectedStatus(false);
      }

      signalEvent();
    }


    virtual void monitorEvent(MonitorPtr const & monitor)
    {
      if (m_debug)
        std::cout << getRequesterName() << "." << "monitorEvent" << std::endl;

      MonitorElement::shared_pointer element = monitor->poll();

      {
        Lock lock(m_pointerMutex);
        m_monitorStatus = true;
        m_pvStructure = element->pvStructurePtr;
        m_changedBitSet = element->changedBitSet;
        m_overrunBitSet = element->overrunBitSet;
        m_monitorCounter++;
      }

      monitor->release(element);
      signalEvent();
    }


    virtual void unlisten(MonitorPtr const & /*monitor*/)
    {
      if (m_debug)
        std::cout << getRequesterName() << "." << "unlisten" << std::endl;
    }


  private:
    int m_monitorCounter;
    bool m_monitorStatus;
    MonitorPtr m_monitor;
    Mutex m_pointerMutex;
    PVStructure::shared_pointer m_pvStructure;
    BitSet::shared_pointer m_changedBitSet;
    BitSet::shared_pointer m_overrunBitSet; 
}; 


class SyncChannelArrayRequesterImpl : public ChannelArrayRequester, public SyncBaseRequester
{

  public:

    typedef std::tr1::shared_ptr<SyncChannelArrayRequesterImpl> shared_pointer; 

    SyncChannelArrayRequesterImpl(bool debug = true) : 
      SyncBaseRequester(debug), 
      m_getArrayStatus(false),
      m_putArrayStatus(false),
      m_lengthArrayStatus(false)  {}
    
    
    bool syncPut(bool lastRequest, int offset, int count, long timeOut) 
    {

      if (!getConnectedStatus()) {
        return false;
      }

      m_channelArray->putArray(lastRequest, offset, count);
      return waitUntilPutArrayDone(timeOut);
    } 


    bool syncGet(bool lastRequest, int offset, int count, long timeOut) 
    {

      if (!getConnectedStatus()) {
        return false;
      }

      m_channelArray->getArray(lastRequest, offset, count);
      return waitUntilGetArrayDone(timeOut);
    } 


    bool syncSetLength(bool lastRequest, int length, int capacity, long timeOut) 
    {

      if (!getConnectedStatus()) {
        return false;
      }

      m_channelArray->setLength(lastRequest, length, capacity);
      return waitUntilSetLengthDone(timeOut);
    } 


    ChannelArray::shared_pointer getChannelArray() 
    {
      Lock lock(m_pointerMutex);
      return m_channelArray;
    }
    
    
    epics::pvData::PVArray::shared_pointer getArray() 
    {
      Lock lock(m_pointerMutex);
      return m_pvArray;
    }


    virtual String getRequesterName()
    {
      return "SynChannelArrayRequesterImpl";
    } 


    virtual void message(String const & message,MessageType messageType)
    {
      if (m_debug)
        std::cout << "[" << getRequesterName() << "] message(" << message << ", " 
          << getMessageTypeName(messageType) << ")" << std::endl;
    }


    virtual void channelArrayConnect(const epics::pvData::Status& status,
        ChannelArray::shared_pointer const & channelArray,
        epics::pvData::PVArray::shared_pointer const & pvArray)
    {
      if (m_debug) 
        std::cout << getRequesterName() << ".channelArrayConnect(" << status.toString() << ")" << std::endl;
      if (status.isSuccess())
      {
        {
          Lock lock(m_pointerMutex);
          m_channelArray = channelArray;
          m_pvArray = pvArray;
        }

        setConnectedStatus(true);
      }
      else
      {
        setConnectedStatus(false);
      }

      signalEvent();
    }


    virtual void getArrayDone(const epics::pvData::Status& status)
    {
      if (m_debug)
        std::cout << getRequesterName()  << ".getArrayDone(" << status.toString() << ")" << std::endl;
      
      Lock lock(m_pointerMutex);
      m_getArrayStatus = status.isSuccess();
      signalEvent();
    }


    virtual void putArrayDone(const epics::pvData::Status& status)
    {
      if (m_debug)
        std::cout << getRequesterName() << ".putArrayDone(" << status.toString() << ")" << std::endl;

      Lock lock(m_pointerMutex);
      m_putArrayStatus = status.isSuccess();
      signalEvent();
    }


    virtual void setLengthDone(const epics::pvData::Status& status)
    { 
      if (m_debug)
        std::cout << getRequesterName() << ".setLengthDone(" << status.toString() << ")" << std::endl;
      
      Lock lock(m_pointerMutex);
      m_lengthArrayStatus = status.isSuccess();
      signalEvent();
    }


  private:
 
    bool waitUntilGetArrayDone(double timeOut)
    {
      {
        Lock lock(m_pointerMutex);
        m_getArrayStatus = false;
      }


      bool signaled = waitUntilEvent(timeOut);
      if (!signaled) {
        return false;
      }

      Lock lock(m_pointerMutex);
      return m_getArrayStatus;
    }


    bool waitUntilPutArrayDone(double timeOut)
    {
      {
        Lock lock(m_pointerMutex);
        m_putArrayStatus = false;
      }


      bool signaled = waitUntilEvent(timeOut);
      if (!signaled) {
        return false;
      }

      Lock lock(m_pointerMutex);
      return m_putArrayStatus;
    }
 

    bool waitUntilSetLengthDone(double timeOut)
    {
      {
        Lock lock(m_pointerMutex);
        m_lengthArrayStatus = false;
      }


      bool signaled = waitUntilEvent(timeOut);
      if (!signaled) {
        return false;
      }

      Lock lock(m_pointerMutex);
      return m_lengthArrayStatus;
    }


    bool m_getArrayStatus;
    bool m_putArrayStatus;
    bool m_lengthArrayStatus;
    Mutex m_pointerMutex;
    ChannelArray::shared_pointer m_channelArray;
    epics::pvData::PVArray::shared_pointer m_pvArray;
};

#endif