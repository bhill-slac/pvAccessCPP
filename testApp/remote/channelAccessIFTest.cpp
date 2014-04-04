#include <stdlib.h>
#include <time.h>
#include <vector>
#include <map>
#include <math.h>

#include <epicsExit.h>
#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/logger.h>
#include <pv/pvAccess.h>
#include <pv/convert.h>
#include <pv/serverContext.h>
#include <pv/clientFactory.h>
#include <pv/clientContextImpl.h>
#include <pv/standardPVField.h>
#include <pv/pvTimeStamp.h>

#include <pv/current_function.h>

#include "channelAccessIFTest.h"

//#define ENABLE_STRESS_TESTS

using namespace std::tr1;


// int value, 1Hz increment by one
std::string ChannelAccessIFTest::TEST_COUNTER_CHANNEL_NAME = "testCounter";
// int value, increment on process
std::string ChannelAccessIFTest::TEST_SIMPLECOUNTER_CHANNEL_NAME = "testSimpleCounter";
// double value, NTScalar
std::string ChannelAccessIFTest::TEST_CHANNEL_NAME = "testValue";
// double value
std::string ChannelAccessIFTest::TEST_VALUEONLY_CHANNEL_NAME = "testValueOnly";
// RPC sum service: int a + int b -> int c
std::string ChannelAccessIFTest::TEST_SUMRPC_CHANNEL_NAME = "testSum";
// double[] value
std::string ChannelAccessIFTest::TEST_ARRAY_CHANNEL_NAME = "testArray1";


int ChannelAccessIFTest::runAllTest() {

#ifdef ENABLE_STRESS_TESTS
  testPlan(157);
#else
  testPlan(152);
#endif

  test_implementation();
  test_providerName();

  test_createEmptyChannel();
  test_createChannelWithInvalidPriority();
  test_createChannel();
  test_recreateChannelOnDestroyedProvider();
  test_findEmptyChannel();
  test_findChannel();
  test_channel();

  test_channelGetWithInvalidChannelAndRequester();
  test_channelGetNoProcess();
  test_channelGetIntProcess();
  test_channelGetTestNoConnection();
  test_channelGetNotYetConnected();

  test_channelPutWithInvalidChannelAndRequester();
  test_channelPutNoProcess();
  test_channelPutIntProcess();
  test_channelPutTestNoConnection();
  test_channelPutNotYetConnected(); 

  test_channelGetFieldAll();
  test_channelGetFieldValue();
  test_channelGetFieldInvalid();

  test_channelProcesstWithInvalidRequesterAndRequest();
  test_channelProcess();
  test_channelProcessNoConnection();

  test_channelPutGetWithInvalidRequesterAndRequest();
  test_channelPutGetNoProcess_putGet();
  test_channelPutGetNoProcess_getPut();
  test_channelPutGetNoProcess_getGet();
  test_channelPutGetNoProcess_destroy();
  test_channelPutGetIntProcess();

  test_channelRPC();
  test_channelRPC_destroy();
  test_channelRPCWithInvalidRequesterAndRequest();

  test_channelMonitorWithInvalidRequesterAndRequest();
  test_channelMonitor(1);

  test_channelArray();
  test_channelArray_destroy();
  test_channelArrayTestNoConnection();

#ifdef ENABLE_STRESS_TESTS
  test_stressConnectDisconnect();
  test_stressConnectGetDisconnect();
  test_stressMonitorAndProcess(); 
#endif

  return testDone();
}


Channel::shared_pointer ChannelAccessIFTest::createChannel(string channelName, bool debug ) 
{

  std::tr1::shared_ptr<SyncChannelRequesterImpl> channelReq(new SyncChannelRequesterImpl(debug));
  Channel::shared_pointer channel = getChannelProvider()->createChannel(channelName, channelReq);
  return channel;
}


Channel::shared_pointer ChannelAccessIFTest::syncCreateChannel(string channelName, bool debug ) 
{

  std::tr1::shared_ptr<SyncChannelRequesterImpl> channelReq(new SyncChannelRequesterImpl(debug));
  Channel::shared_pointer channel = getChannelProvider()->createChannel(channelName, channelReq);
  bool isConnected = channelReq->waitUntilStateChange(getTimeoutSec());
  if (!isConnected) {
    std::cerr << "[" << channelName << "] failed to connect to the channel. " << std::endl;
    return shared_ptr<Channel>();
  }

  return channel;
}


SyncChannelGetRequesterImpl::shared_pointer ChannelAccessIFTest::syncCreateChannelGet(
    Channel::shared_pointer const & channel, string const & request, bool debug ) 
{

  shared_ptr<SyncChannelGetRequesterImpl> 
    channelGetReq(new SyncChannelGetRequesterImpl(channel->getChannelName(), debug));

  PVStructure::shared_pointer pvRequest = CreateRequest::create()->createRequest(request);

  channel->createChannelGet(channelGetReq,pvRequest);
  bool succStatus = channelGetReq->waitUntilGetDone(getTimeoutSec());
  if (!succStatus) {
    std::cerr << "[" << channel->getChannelName() << "] failed to get. " << std::endl;
    return shared_ptr<SyncChannelGetRequesterImpl>();
  }
  return channelGetReq;
}  


SyncChannelPutRequesterImpl::shared_pointer ChannelAccessIFTest::syncCreateChannelPut(
    Channel::shared_pointer const & channel, string const & request, bool debug ) 
{

  shared_ptr<SyncChannelPutRequesterImpl> 
    channelPutReq(new SyncChannelPutRequesterImpl(channel->getChannelName(), debug));


  PVStructure::shared_pointer pvRequest = CreateRequest::create()->createRequest(request);

  channel->createChannelPut(channelPutReq,pvRequest);
  bool succStatus = channelPutReq->waitUntilConnected(getTimeoutSec());

  if (!succStatus) {
    std::cerr << "[" << channel->getChannelName() << "] failed to create a put channel. " << std::endl;
    return shared_ptr<SyncChannelPutRequesterImpl>();
  }
  return channelPutReq;
}  


SyncChannelPutGetRequesterImpl::shared_pointer ChannelAccessIFTest::syncCreateChannelPutGet(
    Channel::shared_pointer const & channel, string const & request, bool debug ) 
{

  shared_ptr<SyncChannelPutGetRequesterImpl> 
    channelPutGetReq(new SyncChannelPutGetRequesterImpl(debug));

  PVStructure::shared_pointer pvRequest = CreateRequest::create()->createRequest(request);

  channel->createChannelPutGet(channelPutGetReq,pvRequest);
  bool succStatus = channelPutGetReq->waitUntilConnected(getTimeoutSec());

  if (!succStatus) {
    std::cerr << "[" << channel->getChannelName() << "] failed to create a put get channel. " << std::endl;
    return shared_ptr<SyncChannelPutGetRequesterImpl>();
  }
  return channelPutGetReq;
}  


SyncChannelRPCRequesterImpl::shared_pointer ChannelAccessIFTest::syncCreateChannelRPC(
    Channel::shared_pointer const & channel, bool debug ) 
{

  shared_ptr<SyncChannelRPCRequesterImpl> 
    channelRPCReq(new SyncChannelRPCRequesterImpl(debug));

  PVStructure::shared_pointer pvRequest = CreateRequest::create()->createRequest(string());

  channel->createChannelRPC(channelRPCReq, pvRequest);
  bool succStatus = channelRPCReq->waitUntilConnected(getTimeoutSec());

  if (!succStatus) {
    std::cerr << "[" << channel->getChannelName() << "] failed to create a RPC channel. " << std::endl;
    return shared_ptr<SyncChannelRPCRequesterImpl>();
  }
  return channelRPCReq;
}  


SyncMonitorRequesterImpl::shared_pointer ChannelAccessIFTest::syncCreateChannelMonitor(
    Channel::shared_pointer const & channel, string const & request, bool debug ) 
{
  shared_ptr<SyncMonitorRequesterImpl> monitorReq(new SyncMonitorRequesterImpl(debug)); 

  PVStructure::shared_pointer pvRequest = CreateRequest::create()->createRequest(request);

  channel->createMonitor(monitorReq, pvRequest);
  bool succStatus = monitorReq->waitUntilConnected(getTimeoutSec());

  if (!succStatus) {
    std::cerr << "[" << channel->getChannelName() << "] failed to create a monitor channel. " << std::endl;
    return shared_ptr<SyncMonitorRequesterImpl>();
  }
  return monitorReq;
}  


SyncChannelArrayRequesterImpl::shared_pointer ChannelAccessIFTest::syncCreateChannelArray(
    Channel::shared_pointer const & channel, PVStructure::shared_pointer pvRequest, bool debug ) 
{
  shared_ptr<SyncChannelArrayRequesterImpl> arrayReq(new SyncChannelArrayRequesterImpl(debug)); 

  channel->createChannelArray(arrayReq, pvRequest);
  bool succStatus = arrayReq->waitUntilConnected(getTimeoutSec());

  if (!succStatus) {
    std::cerr << "[" << channel->getChannelName() << "] failed to create an array channel. " << std::endl;
    return shared_ptr<SyncChannelArrayRequesterImpl>();
  }
  return arrayReq;
}  


void ChannelAccessIFTest::test_implementation() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  testOk(getChannelProvider().get() != 0, "%s: channel provider is provided", CURRENT_FUNCTION );
  ChannelProvider::shared_pointer cp1 = getChannelProvider();
  ChannelProvider::shared_pointer cp2 = getChannelProvider();
  testOk(cp1 == cp2, "%s: calling getChannelProvider twice", CURRENT_FUNCTION);
  testOk(getTimeoutSec() > 0, "%s: timeout is set", CURRENT_FUNCTION);
}


void ChannelAccessIFTest::test_providerName() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  testOk(!getChannelProvider()->getProviderName().empty(), "%s: provider name is set", CURRENT_FUNCTION);
  testOk(getChannelProvider()->getProviderName()== getChannelProvider()->getProviderName(),
      "%s: calling getProviderName twice should return the same provider", CURRENT_FUNCTION);
} 


void ChannelAccessIFTest::test_createEmptyChannel() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  std::tr1::shared_ptr<SyncChannelRequesterImpl> channelReq(new SyncChannelRequesterImpl());

  try {
    Channel::shared_pointer channel = getChannelProvider()->createChannel(String(), channelReq);
    testFail("%s: empty channel name should not be allowed", CURRENT_FUNCTION);
    return;
  } catch(std::runtime_error &) {
  }

  testOk(channelReq->getCreatedCount() == 0, 
      "%s: checking for empty channel name", CURRENT_FUNCTION);
}


void ChannelAccessIFTest::test_createChannelWithInvalidPriority() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  std::tr1::shared_ptr<SyncChannelRequesterImpl> channelReq(new SyncChannelRequesterImpl());

  try {
    Channel::shared_pointer channel = getChannelProvider()->createChannel(String(), channelReq,
        ChannelProvider::PRIORITY_MIN - 1 );
    testFail("%s: invalid priority should not be allowed when creating a channel",CURRENT_FUNCTION);
    return;
  } catch(std::runtime_error &) {
  }

  testOk(channelReq->getCreatedCount() == 0, 
      "%s: checking for invalid priority when creating a new channel",CURRENT_FUNCTION);
}


void ChannelAccessIFTest::test_createChannel() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  std::tr1::shared_ptr<SyncChannelRequesterImpl> channelReq(new SyncChannelRequesterImpl());
  Channel::shared_pointer channel = getChannelProvider()->createChannel(TEST_COUNTER_CHANNEL_NAME, channelReq);
  bool succStatus = channelReq->waitUntilStateChange(getTimeoutSec());
  if (!succStatus) {
    std::cerr << "[" << TEST_COUNTER_CHANNEL_NAME << "] failed to connect. " << std::endl;
    testFail("%s: could not create a channel", CURRENT_FUNCTION);
    return;
  }

  channel->destroy();
  testOk(channelReq->getStatus().isSuccess(), "%s: create a channel", CURRENT_FUNCTION);
}


void ChannelAccessIFTest::test_recreateChannelOnDestroyedProvider() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION); 

  if (isLocal()){
    testOk(true, "%s: recreating channel on destroyed provider", CURRENT_FUNCTION);
    return;
  }

  ChannelProvider::shared_pointer provider = getChannelProvider();
  provider->destroy();
  test_createChannel();
}


void ChannelAccessIFTest::test_findEmptyChannel() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION); 

  try {
    std::tr1::shared_ptr<SyncChannelFindRequesterImpl> channelFindReq(new SyncChannelFindRequesterImpl());
    getChannelProvider()->channelFind(String(), channelFindReq);
    testFail("%s: empty channel name shoud never be allowed when searching for channels", CURRENT_FUNCTION);
    return;

  } catch(std::runtime_error &) {
  }

  testOk(true, "%s: finding an empty channel", CURRENT_FUNCTION);
}


void ChannelAccessIFTest::test_findChannel() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  std::tr1::shared_ptr<SyncChannelFindRequesterImpl> channelFindReq(new SyncChannelFindRequesterImpl());
  getChannelProvider()->channelFind(TEST_COUNTER_CHANNEL_NAME, channelFindReq);
  bool isFindResult = channelFindReq->waitUntilFindResult(getTimeoutSec());
  if (!isFindResult) {
    std::cerr << "[" << TEST_COUNTER_CHANNEL_NAME << "] failed to connect. " << std::endl;
    testFail("%s: could not find a channel", CURRENT_FUNCTION);
    return;
  }
  testSkip(1, "finding a channel not implemented");
}


void ChannelAccessIFTest::test_channel() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  std::tr1::shared_ptr<SyncChannelRequesterImpl> channelReq(new SyncChannelRequesterImpl());
  Channel::shared_pointer channel = getChannelProvider()->createChannel(TEST_COUNTER_CHANNEL_NAME, channelReq);
  bool succStatus = channelReq->waitUntilStateChange(getTimeoutSec());
  if (!succStatus) {
    std::cerr << "[" << channel->getChannelName() << "] failed to connect. " << std::endl;
    testFail("%s: could not create a channel", CURRENT_FUNCTION);
    return;
  }

  testOk(channelReq->getCreatedCount() == 1, "%s: channel created count", CURRENT_FUNCTION);
  testOk(channelReq->getStatus().isSuccess(), "%s: succ created a channel", CURRENT_FUNCTION);
  testOk(channelReq->getStateChangeCount() == 1, "%s: channel state change count", CURRENT_FUNCTION);
  testOk(channel->getChannelName() == TEST_COUNTER_CHANNEL_NAME, "%s: channel name match", CURRENT_FUNCTION);
  testOk(channel->getChannelRequester() == channelReq, "%s: channel requesters match", CURRENT_FUNCTION);
  //testOk(channel->getRequesterName() == channelRequesterPtr->getRequesterName(), 
  //    " channel requester's name match");
  testSkip(1, "channel requester's name mismatch");
  testOk(channel->getProvider() == getChannelProvider(), "%s: channel provider match", CURRENT_FUNCTION);
  testOk(!channel->getRemoteAddress().empty(), "%s: channel remote address set", CURRENT_FUNCTION);
  testOk(channel->isConnected(), "%s: channel connected ", CURRENT_FUNCTION);
  testOk(channel->getConnectionState() == Channel::CONNECTED , 
      "%s: channel connection state connected ", CURRENT_FUNCTION);

  testDiag("%s: destroying the channel", CURRENT_FUNCTION); 
  channel->destroy();

  succStatus = channelReq->waitUntilStateChange(getTimeoutSec());
  if (!succStatus) {
    std::cerr << "[" << channel->getChannelName() << "] failed to register a destroy event " << std::endl;
    testFail("%s: a destroy event was not caught for the testing channel ", CURRENT_FUNCTION);
    return;
  }

  testOk(channelReq->getCreatedCount() == 1, 
      "%s: channel created count should be the same on the destroyed channel", CURRENT_FUNCTION);
  testOk(channelReq->getStateChangeCount() == 3, 
      "%s: channel state change count should increase on the destroyed channel", CURRENT_FUNCTION);
  testOk(!channel->isConnected(), "%s: channel should not be connected ", CURRENT_FUNCTION);
  testOk(channel->getConnectionState() == Channel::DESTROYED , 
      "%s: channel connection state DESTROYED ", CURRENT_FUNCTION);

  testDiag("%s: destroying the channel yet again", CURRENT_FUNCTION); 
  channel->destroy();

  succStatus = channelReq->waitUntilStateChange(getTimeoutSec());
  if (succStatus) {
    std::cerr << "[" << channel->getChannelName() << "] registered a duplicate destroy event " << std::endl;
    testFail("%s: a destroy event was caught for the testing channel that was destroyed twice ", 
        CURRENT_FUNCTION);
    return;
  }

  testOk(channelReq->getCreatedCount() == 1, 
      "%s: channel created count should be the same on the yet again destroyed channel", CURRENT_FUNCTION);

  testOk(!channel->isConnected(), "%s: yet again destroyed channel should not be connected ", CURRENT_FUNCTION);
  testOk(channel->getConnectionState() == Channel::DESTROYED , 
      "%s: yet again destroyed channel connection state DESTROYED ", CURRENT_FUNCTION);
}


void ChannelAccessIFTest::test_channelGetWithInvalidChannelAndRequester() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  Channel::shared_pointer channel = syncCreateChannel(TEST_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  testSkip(1, " creating a channel get with a null channel");
  //SyncChannelGetRequesterImpl::shared_pointer channelGetReq =  synCreateChannelGet(Channel::shared_pointer(), 
  //    string("field(timeStamp,value)"));


  testSkip(1, " creating a channel get with an empty request");
  //SyncChannelGetRequesterImpl::shared_pointer channelGetReq =  syncCreateChannelGet(Channel::shared_pointer(), 
  //   string());

  channel->destroy();
}


void ChannelAccessIFTest::test_channelGetNoProcess() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "field(timeStamp,value)";

  Channel::shared_pointer channel = syncCreateChannel(TEST_VALUEONLY_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncChannelGetRequesterImpl::shared_pointer channelGetReq =  syncCreateChannelGet(channel,request); 

  if (!channelGetReq.get()) {
    testFail("%s: channel get not created ", CURRENT_FUNCTION);
    return;
  }

  testOk(channelGetReq->getBitSet()->cardinality() == 1, "%s: bitset cardinality", CURRENT_FUNCTION);      
  testOk(channelGetReq->getBitSet()->get(0) == true, "%s: bitset get(0)", CURRENT_FUNCTION); 

  bool succStatus = channelGetReq->syncGet(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: sync get failed ", CURRENT_FUNCTION);
    return;
  } 

  testOk(channelGetReq->getBitSet()->cardinality() == 0, 
      "%s: bitset cardinality after first sync get", CURRENT_FUNCTION);

  succStatus = channelGetReq->syncGet(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: sync get failed ", CURRENT_FUNCTION);
    return;
  } 

  testOk(channelGetReq->getBitSet()->cardinality() == 0, 
      "%s: bitset cardinality after second sync get", CURRENT_FUNCTION);

  channelGetReq->getChannelGet()->destroy();
  testOk(channelGetReq->syncGet(false, getTimeoutSec()) == false, 
      "%s: after the channel get destroy, sync get must fail", CURRENT_FUNCTION); 

  channel->destroy();
}


void ChannelAccessIFTest::test_channelGetIntProcess() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  Channel::shared_pointer channel = syncCreateChannel(TEST_SIMPLECOUNTER_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  test_channelGetIntProcessInternal(channel, CURRENT_FUNCTION);

  channel->destroy();
}


void ChannelAccessIFTest::test_channelGetNotYetConnected() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  Channel::shared_pointer channel = createChannel(TEST_SIMPLECOUNTER_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  test_channelGetIntProcessInternal(channel, CURRENT_FUNCTION);

  channel->destroy();
}


void ChannelAccessIFTest::test_channelGetIntProcessInternal(Channel::shared_pointer channel, 
    string const & testMethodName) {

  string request = "record[process=true]field(timeStamp,value)"; 

  SyncChannelGetRequesterImpl::shared_pointer channelGetReq = syncCreateChannelGet(channel, request);
  if (!channelGetReq.get()) {
    testFail("%s: channel get not created ", testMethodName.c_str());
    return;
  }

  std::tr1::shared_ptr<PVInt> value = channelGetReq->getPVStructure()->getIntField("value");

  PVTimeStamp pvTimeStamp;

  testOk(pvTimeStamp.attach(channelGetReq->getPVStructure()->getStructureField("timeStamp")),
      "%s: attaching a timestamp", testMethodName.c_str());

  TimeStamp timeStamp;

  bool succStatus = channelGetReq->syncGet(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: sync get failed ", testMethodName.c_str());
    return;
  } 

  pvTimeStamp.get(timeStamp);

  testOk(channelGetReq->getBitSet()->cardinality() == 1, "%s: bitset cardinality", testMethodName.c_str());      
  testOk(channelGetReq->getBitSet()->get(0) == true, "%s: bitset get(0)", testMethodName.c_str()); 

  int numOfTimes = 3;

  for (int i = 0; i < numOfTimes; i++) {

    int previousValue = value->get();
    long previousTimestampSec = timeStamp.getSecondsPastEpoch();

    epicsThreadSleep(1.0);

    bool succStatus = channelGetReq->syncGet(false, getTimeoutSec());
    if (!succStatus) {
      testFail("%s: sync get failed ", testMethodName.c_str());
      return;
    } 

    pvTimeStamp.get(timeStamp);

    testOk((previousValue +1)/*%11*/ == value->get(), "%s: testing the counter value change", 
        testMethodName.c_str());      
    testOk(timeStamp.getSecondsPastEpoch() > previousTimestampSec, 
        "%s: testing the timestamp change", testMethodName.c_str());      
  }     

  channelGetReq->getChannelGet()->destroy();
  testOk(channelGetReq->syncGet(false, getTimeoutSec()) == false, 
      "%s: after the channel get destroy, sync get must fail", testMethodName.c_str()); 

  channel->destroy();
}


void ChannelAccessIFTest::test_channelGetTestNoConnection() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "field(timeStamp,value)"; 

  Channel::shared_pointer channel = syncCreateChannel(TEST_COUNTER_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncChannelGetRequesterImpl::shared_pointer channelGetReq = syncCreateChannelGet(channel, request);
  if (!channelGetReq.get()) {
    testFail("%s: channel get not created ", CURRENT_FUNCTION);
    return;
  }

  channel->destroy();

  bool succStatus = channelGetReq->syncGet(false, getTimeoutSec());
  if (!succStatus) {
    testOk(true, "%s: get failed on destroy channel ", CURRENT_FUNCTION);
  }
  else {
    testFail("%s: get should fail on destroy channel ", CURRENT_FUNCTION);
  } 
}


void ChannelAccessIFTest::test_channelPutWithInvalidChannelAndRequester() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  Channel::shared_pointer channel = syncCreateChannel(TEST_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  testSkip(1, " creating a channel put with a null channel: seg fault");
  //SyncChannelPutRequesterImpl::shared_pointer channelPutReq =  syncCreateChannelPut(Channel::shared_pointer(), 
  //    string("field(timeStamp,value)"));


  testSkip(1, " creating a channel put with an empty request: seg fault");
  //SyncChannelPutRequesterImpl::shared_pointer channelPutReq1 =  syncCreateChannelPut(channel, 
  //   string());

  channel->destroy();
}


void ChannelAccessIFTest::test_channelPutNoProcess() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "field(value)";

  Channel::shared_pointer channel = syncCreateChannel(TEST_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncChannelPutRequesterImpl::shared_pointer channelPutReq =  syncCreateChannelPut(channel,request); 

  if (!channelPutReq.get()) {
    testFail("%s: channel put not created ", CURRENT_FUNCTION);
    return;
  }

  std::tr1::shared_ptr<PVDouble> value = channelPutReq->getPVStructure()->getDoubleField("value");
  if (!value.get()) {
    testFail("%s: getting double value field failed ", CURRENT_FUNCTION);
    return;
  }

  double initVal = 123.0;

  value->put(initVal);
  channelPutReq->getBitSet()->set(value->getFieldOffset());

  bool succStatus = channelPutReq->syncPut(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: sync put failed ", CURRENT_FUNCTION);
    return;
  } 

  succStatus = channelPutReq->syncGet(getTimeoutSec());
  if (!succStatus) {
    testFail("%s: sync get failed ", CURRENT_FUNCTION);
    return;
  } 


  testOk(initVal == value->get(), "%s: initial value matches the get value ", CURRENT_FUNCTION);

  value->put(initVal + 2);
  channelPutReq->getBitSet()->clear();

  succStatus = channelPutReq->syncPut(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: second sync put failed ", CURRENT_FUNCTION);
    return;
  } 

  succStatus = channelPutReq->syncGet(getTimeoutSec());
  if (!succStatus) {
    testFail("%s: second sync get failed ", CURRENT_FUNCTION);
    return;
  } 

  if (isLocal()) {
    testOk(initVal+2 == value->get(), 
        "%s: value should not change since bitset is not set unless is local ", CURRENT_FUNCTION);
  }
  else {
    testOk(initVal == value->get(), "%s: value should not change since bitset is not set ", CURRENT_FUNCTION);
  } 

  value->put(initVal + 1);
  channelPutReq->getBitSet()->set(value->getFieldOffset());

  succStatus = channelPutReq->syncPut(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: third sync put failed ", CURRENT_FUNCTION);
    return;
  } 

  succStatus = channelPutReq->syncGet(getTimeoutSec());
  if (!succStatus) {
    testFail("%s: third sync get failed ", CURRENT_FUNCTION);
    return;
  } 

  testOk(initVal+1 == value->get(), "%s: changed value matches the get value ", CURRENT_FUNCTION);

  testDiag("%s: Ok, let's destroy the put channel", CURRENT_FUNCTION);

  succStatus = channelPutReq->syncPut(true, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: destroying sync put failed ", CURRENT_FUNCTION);
    return;
  } 

  channelPutReq->getChannelPut()->destroy();

  succStatus = channelPutReq->syncPut(true, getTimeoutSec());
  if (!succStatus) {
    testOk(true, "%s: put on destroyed put channel should not succeed ", CURRENT_FUNCTION);
  }
  else {
    testFail("%s: put on destroyed put channel did succeed ", CURRENT_FUNCTION);
  } 

  channel->destroy();
}


void ChannelAccessIFTest::test_channelPutIntProcess() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  Channel::shared_pointer channel = syncCreateChannel(TEST_SIMPLECOUNTER_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  test_channelPutIntProcessInternal(channel, CURRENT_FUNCTION);

  channel->destroy();
}


void ChannelAccessIFTest::test_channelPutNotYetConnected() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  Channel::shared_pointer channel = createChannel(TEST_SIMPLECOUNTER_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  test_channelPutIntProcessInternal(channel, CURRENT_FUNCTION);

  channel->destroy();
}


void ChannelAccessIFTest::test_channelPutIntProcessInternal(Channel::shared_pointer channel, 
    string const & testMethodName) {

  string request = "record[process=true]field(value)";

  SyncChannelPutRequesterImpl::shared_pointer channelPutReq =  syncCreateChannelPut(channel,request); 

  if (!channelPutReq.get()) {
    testFail("%s: channel put not created ", testMethodName.c_str());
    return;
  }

  std::tr1::shared_ptr<PVInt> value = channelPutReq->getPVStructure()->getIntField("value");
  if (!value.get()) {
    testFail("%s: getting int value field failed ", testMethodName.c_str());
    return;
  }

  int initVal = 3;

  value->put(initVal);
  channelPutReq->getBitSet()->set(value->getFieldOffset());

  bool succStatus = channelPutReq->syncPut(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: sync put failed ", testMethodName.c_str());
    return;
  } 

  succStatus = channelPutReq->syncGet(getTimeoutSec());
  if (!succStatus) {
    testFail("%s: sync get failed ", testMethodName.c_str());
    return;
  } 

  testOk(initVal + 1 == value->get(), "%s: value changed +1 due to processing ", testMethodName.c_str());

  value->put(initVal + 3);
  channelPutReq->getBitSet()->clear();

  succStatus = channelPutReq->syncPut(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: second sync put failed ", testMethodName.c_str());
    return;
  } 

  succStatus = channelPutReq->syncGet(getTimeoutSec());
  if (!succStatus) {
    testFail("%s: second sync get failed ", testMethodName.c_str());
    return;
  } 

  if (isLocal()) {
    testOk(initVal+4 == value->get(), 
        "%s: value changed due to processing unless is local ", testMethodName.c_str());
  }
  else {
    testOk(initVal+2 == value->get(), "%s: value not changed ", testMethodName.c_str());
  } 


  testDiag("%s: Ok, let's destroy the put channel", testMethodName.c_str());

  succStatus = channelPutReq->syncPut(true, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: destroying sync put failed ", testMethodName.c_str());
    return;
  } 

  channelPutReq->getChannelPut()->destroy();

  succStatus = channelPutReq->syncPut(true, getTimeoutSec());
  if (!succStatus) {
    testOk(true, "%s: put on destroyed put channel should not succeed ", testMethodName.c_str());
  }
  else {
    testFail("%s: put on destroyed put channel did succeed ", testMethodName.c_str());
  } 

  channel->destroy();
}


void ChannelAccessIFTest::test_channelPutTestNoConnection() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "field(timeStamp,value)"; 

  Channel::shared_pointer channel = syncCreateChannel(TEST_COUNTER_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncChannelPutRequesterImpl::shared_pointer channelPutReq = syncCreateChannelPut(channel, request);
  if (!channelPutReq.get()) {
    testFail("%s: channel put not created ", CURRENT_FUNCTION);
    return;
  }

  channel->destroy();

  bool succStatus = channelPutReq->syncPut(false, getTimeoutSec());
  if (!succStatus) {
    testOk(true, "%s: put failed on destroy channel ", CURRENT_FUNCTION);
  }
  else {
    testFail("%s: put should fail on destroy channel ", CURRENT_FUNCTION);
  }

  channel->destroy();
}


void ChannelAccessIFTest::test_channelGetFieldAll() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  Channel::shared_pointer channel = syncCreateChannel(TEST_COUNTER_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  shared_ptr<SyncGetFieldRequesterImpl> 
    channelGetFieldReq(new SyncGetFieldRequesterImpl());

  channel->getField(channelGetFieldReq, String());

  bool succStatus = channelGetFieldReq->waitUntilGetDone(getTimeoutSec()); 
  if (!succStatus) {
    testFail("%s: get all fields failed ", CURRENT_FUNCTION);
  }
  else {
    testOk(true, "%s: get all fields succeeded ", CURRENT_FUNCTION);
    testOk(channelGetFieldReq->getField()->getType() == structure, 
        "%s: field type is structure ", CURRENT_FUNCTION);
  }

  channel->destroy();

}


void ChannelAccessIFTest::test_channelGetFieldValue() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  Channel::shared_pointer channel = syncCreateChannel(TEST_COUNTER_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  shared_ptr<SyncGetFieldRequesterImpl> 
    channelGetFieldReq(new SyncGetFieldRequesterImpl());

  channel->getField(channelGetFieldReq, "value");

  bool succStatus = channelGetFieldReq->waitUntilGetDone(getTimeoutSec()); 
  if (!succStatus) {
    testFail("%s: get field value failed ", CURRENT_FUNCTION);
  }
  else {
    testOk(true, "%s: get field value succeeded ", CURRENT_FUNCTION);
    testOk(channelGetFieldReq->getField()->getType() == scalar, 
        "%s: field type is scalar", CURRENT_FUNCTION);
  }

  channel->destroy();

}


void ChannelAccessIFTest::test_channelGetFieldInvalid() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  Channel::shared_pointer channel = syncCreateChannel(TEST_COUNTER_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  shared_ptr<SyncGetFieldRequesterImpl> 
    channelGetFieldReq(new SyncGetFieldRequesterImpl());

  channel->getField(channelGetFieldReq, "invalid");

  bool succStatus = channelGetFieldReq->waitUntilGetDone(getTimeoutSec()); 
  if (!succStatus) {
    testOk(true, "%s: get invalid field should not succeed ", CURRENT_FUNCTION);
  }
  else {
    testFail("%s: get invalid field succeded ", CURRENT_FUNCTION);
  }

  channel->destroy();

}


void ChannelAccessIFTest::test_channelProcesstWithInvalidRequesterAndRequest() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  Channel::shared_pointer channel = syncCreateChannel(TEST_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  ChannelProcess::shared_pointer channelProcess = 
    channel->createChannelProcess(ChannelProcessRequester::shared_pointer(),
        epics::pvData::PVStructure::shared_pointer()); 

  testSkip(1, " creating a channel process get with a null requester/request");
  /*
  if (!channelProcess.get()) {
    testOk(true, "%s: creating channel process with null requester/request should not succeed ", CURRENT_FUNCTION);
  }
  else {
    testFail("%s: creating channel process with null requester/request succeded ", CURRENT_FUNCTION);
  }
*/

  channel->destroy();
}


void ChannelAccessIFTest::test_channelProcess() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "field(value)"; 

  Channel::shared_pointer channel = syncCreateChannel(TEST_SIMPLECOUNTER_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncChannelGetRequesterImpl::shared_pointer channelGetReq = syncCreateChannelGet(channel, request);
  if (!channelGetReq.get()) {
    testFail("%s: channel get not created ", CURRENT_FUNCTION);
    return;
  }

  bool succStatus = channelGetReq->syncGet(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: sync get failed ", CURRENT_FUNCTION);
    return;
  }
  else {
    testOk(true, "%s: syncGet the initial state", CURRENT_FUNCTION);
  } 

  shared_ptr<SyncChannelProcessRequesterImpl> 
    channelProcessReq(new SyncChannelProcessRequesterImpl());


  ChannelProcess::shared_pointer channelProcess = 
    channel->createChannelProcess(channelProcessReq,
        epics::pvData::PVStructure::shared_pointer()); 

  succStatus = channelProcessReq->waitUntilConnected(getTimeoutSec());
  if (!succStatus) {
    testFail("%s: a channel process is not connected ", CURRENT_FUNCTION);
    return;
  }
  else {
    testOk(true,"%s: a process channel with an empty request connected", CURRENT_FUNCTION);
  } 


  succStatus = channelGetReq->syncGet(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: second sync get failed ", CURRENT_FUNCTION);
    return;
  } 

  testOk(channelGetReq->getBitSet()->cardinality() == 0, "%s: bitset cardinality is 0", CURRENT_FUNCTION);      

  succStatus = channelProcessReq->syncProcess(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: syncProcess failed ", CURRENT_FUNCTION);
    return;
  } 
  else {
    testOk(true, "%s: a sync process succeeded", CURRENT_FUNCTION);      
  }

  succStatus = channelGetReq->syncGet(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: third sync get failed ", CURRENT_FUNCTION);
    return;
  } 


  testOk(channelGetReq->getBitSet()->cardinality() == 1, 
      "%s: bitset cardinality should change after process", CURRENT_FUNCTION);      

  testDiag("%s: destroying the process channel", CURRENT_FUNCTION);
  channelProcessReq->getChannelProcess()->destroy();

  succStatus = channelProcessReq->syncProcess(false, getTimeoutSec());
  if (!succStatus) {
    testOk(true, "%s: processing on a destroyed process channel should not succeed", CURRENT_FUNCTION);      
  } 
  else {
    testFail("%s: processing on a destroyed process channel succeed ", CURRENT_FUNCTION);
  }

  channel->destroy();
}


void ChannelAccessIFTest::test_channelProcessNoConnection() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "field(value)"; 

  Channel::shared_pointer channel = syncCreateChannel(TEST_COUNTER_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncChannelGetRequesterImpl::shared_pointer channelGetReq = syncCreateChannelGet(channel, request);
  if (!channelGetReq.get()) {
    testFail("%s: channel get not created ", CURRENT_FUNCTION);
    return;
  }

  testDiag("%s: destroying the channel", CURRENT_FUNCTION);
  channel->destroy();

  bool succStatus = channelGetReq->syncGet(false, getTimeoutSec());
  if (!succStatus) {
    testOk(true, "%s: processing on a destroyed channel should not succeed", CURRENT_FUNCTION);      
  } 
  else {
    testFail("%s: processing on a destroyed channel succeed ", CURRENT_FUNCTION);
  }
}


void ChannelAccessIFTest::test_channelPutGetWithInvalidRequesterAndRequest() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "putField(value)getField(timeStamp,value)"; 

  Channel::shared_pointer channel = syncCreateChannel(TEST_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  testSkip(1, " creating a channel put get with a null channel");
  //SyncChannelPutGetRequesterImpl::shared_pointer channelPutGetReq =  
  //  syncCreateChannelPutGet(Channel::shared_pointer(),request);

  testSkip(1, " creating a channel put get with an empty request");
  //SyncChannelPutGetRequesterImpl::shared_pointer channelPutGetReq1 =  
  //syncCreateChannelPutGet(channel,String());

  channel->destroy();
} 


void ChannelAccessIFTest::test_channelPutGetNoProcess_putGet() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "putField(value)getField(timeStamp,value)"; 

  Channel::shared_pointer channel = syncCreateChannel(TEST_VALUEONLY_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncChannelPutGetRequesterImpl::shared_pointer channelPutGetReq =  
    syncCreateChannelPutGet(channel,request);
  if (!channelPutGetReq.get()) {
    testFail("%s: creating a channel putget failed ", CURRENT_FUNCTION);
    return;
  }


  std::tr1::shared_ptr<PVDouble> putValue = channelPutGetReq->getPVPutStructure()->getDoubleField("value");
  if (!putValue.get()) {
    testFail("%s: getting put double value field failed ", CURRENT_FUNCTION);
    return;
  }

  double initVal = 321.0;
  putValue->put(initVal);

  std::tr1::shared_ptr<PVDouble> getValuePtr = channelPutGetReq->getPVGetStructure()->getDoubleField("value");
  if (!getValuePtr.get()) {
    testFail("%s: getting get double value field failed ", CURRENT_FUNCTION);
    return;
  }

  bool succStatus = channelPutGetReq->syncPutGet(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: syncPutGet failed ", CURRENT_FUNCTION);
    return;
  } 
  else {
    testOk(true, "%s: a syncPutGet succeeded", CURRENT_FUNCTION);      
  }

  testOk(getValuePtr->get() == initVal, "value after syncPutGet should be initVal");

  testDiag("%s: testing putGet", CURRENT_FUNCTION);

  putValue->put(initVal + 1.0);

  succStatus = channelPutGetReq->syncPutGet(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: second syncPutGet failed ", CURRENT_FUNCTION);
    return;
  } 
  else {
    testOk(true, "%s: a syncPutGet succeeded", CURRENT_FUNCTION);      
  }

  testOk(getValuePtr->get() == initVal + 1.0, "value after syncPutGet should be initVal + 1");

  channel->destroy();
}


void ChannelAccessIFTest::test_channelPutGetNoProcess_getPut() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "putField(value)getField(timeStamp,value)"; 

  Channel::shared_pointer channel = syncCreateChannel(TEST_VALUEONLY_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncChannelPutGetRequesterImpl::shared_pointer channelPutGetReq =  
    syncCreateChannelPutGet(channel,request);
  if (!channelPutGetReq.get()) {
    testFail("%s: creating a channel putget failed ", CURRENT_FUNCTION);
    return;
  }


  std::tr1::shared_ptr<PVDouble> putValue = channelPutGetReq->getPVPutStructure()->getDoubleField("value");
  if (!putValue.get()) {
    testFail("%s: getting put double value field failed ", CURRENT_FUNCTION);
    return;
  }

  double initVal = 321.0;
  putValue->put(initVal);

  std::tr1::shared_ptr<PVDouble> getValuePtr = channelPutGetReq->getPVGetStructure()->getDoubleField("value");
  if (!getValuePtr.get()) {
    testFail("%s: getting get double value field failed ", CURRENT_FUNCTION);
    return;
  }

  testDiag("%s: first put the initial pvPutStructure into the record", CURRENT_FUNCTION);

  bool succStatus = channelPutGetReq->syncPutGet(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: syncPutGet failed ", CURRENT_FUNCTION);
    return;
  } 
  else {
    testOk(true, "%s: a syncPutGet succeeded", CURRENT_FUNCTION);      
  }

  testDiag("%s: get the current data from the record and put it into pvPutStructure", CURRENT_FUNCTION);

  succStatus = channelPutGetReq->syncGetPut(getTimeoutSec());
  if (!succStatus) {
    testFail("%s: syncGetPut failed ", CURRENT_FUNCTION);
    return;
  } 
  else {
    testOk(true, "%s: a syncGetPut succeeded", CURRENT_FUNCTION);      
  }

  testOk(putValue->get() == initVal, "value after syncGetPut should be initVal");

  channel->destroy();
}


void ChannelAccessIFTest::test_channelPutGetNoProcess_getGet() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "putField(value)getField(timeStamp,value)"; 

  Channel::shared_pointer channel = syncCreateChannel(TEST_VALUEONLY_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncChannelPutGetRequesterImpl::shared_pointer channelPutGetReq =  
    syncCreateChannelPutGet(channel,request);
  if (!channelPutGetReq.get()) {
    testFail("%s: creating a channel putget failed ", CURRENT_FUNCTION);
    return;
  }


  std::tr1::shared_ptr<PVDouble> putValue = channelPutGetReq->getPVPutStructure()->getDoubleField("value");
  if (!putValue.get()) {
    testFail("%s: getting put double value field failed ", CURRENT_FUNCTION);
    return;
  }

  double initVal = 432.0;
  putValue->put(initVal);

  std::tr1::shared_ptr<PVDouble> getValuePtr = channelPutGetReq->getPVGetStructure()->getDoubleField("value");
  if (!getValuePtr.get()) {
    testFail("%s: getting get double value field failed ", CURRENT_FUNCTION);
    return;
  }

  testDiag("%s: first put the initial pvPutStructure into the record", CURRENT_FUNCTION);

  bool succStatus = channelPutGetReq->syncPutGet(false, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: syncPutGet failed ", CURRENT_FUNCTION);
    return;
  } 
  else {
    testOk(true, "%s: a syncPutGet succeeded", CURRENT_FUNCTION);      
  }

  testDiag("%s: get the current data from the record and put it into pvGetStructure", CURRENT_FUNCTION);

  succStatus = channelPutGetReq->syncGetGet(getTimeoutSec());
  if (!succStatus) {
    testFail("%s: syncGetGet failed ", CURRENT_FUNCTION);
    return;
  } 
  else {
    testOk(true, "%s: a syncGetGet succeeded", CURRENT_FUNCTION);      
  }

  testOk(getValuePtr->get() == initVal, "value after syncGetGet should be initVal");

  channel->destroy();
}


void ChannelAccessIFTest::test_channelPutGetNoProcess_destroy() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "putField(value)getField(timeStamp,value)"; 

  Channel::shared_pointer channel = syncCreateChannel(TEST_VALUEONLY_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncChannelPutGetRequesterImpl::shared_pointer channelPutGetReq =  
    syncCreateChannelPutGet(channel,request);
  if (!channelPutGetReq.get()) {
    testFail("%s: creating a channel putget failed ", CURRENT_FUNCTION);
    return;
  }

  bool succStatus = channelPutGetReq->syncPutGet(true, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: syncPutGet failed ", CURRENT_FUNCTION);
    return;
  } 
  else {
    testOk(true, "%s: syncPutGet succeeded", CURRENT_FUNCTION);      
  }

  testDiag("%s: destroying a channel putget", CURRENT_FUNCTION);
  channelPutGetReq->getChannelPutGet()->destroy();


  succStatus = channelPutGetReq->syncPutGet(true, getTimeoutSec());
  if (!succStatus) {
    testOk(true, "%s: syncPutGet on a destroyed channel should not succeed", CURRENT_FUNCTION);      
  } 
  else {
    testFail("%s: syncPutGet on a destroyed channel succeeded ", CURRENT_FUNCTION);
  }

  channel->destroy();
}


void ChannelAccessIFTest::test_channelPutGetIntProcess() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "record[process=true]putField(value)getField(timeStamp,value)"; 

  Channel::shared_pointer channel = syncCreateChannel(TEST_SIMPLECOUNTER_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncChannelPutGetRequesterImpl::shared_pointer channelPutGetReq =  
    syncCreateChannelPutGet(channel,request);
  if (!channelPutGetReq.get()) {
    testFail("%s: creating a channel putget failed ", CURRENT_FUNCTION);
    return;
  }


  std::tr1::shared_ptr<PVInt> putValue = channelPutGetReq->getPVPutStructure()->getIntField("value");
  if (!putValue.get()) {
    testFail("%s: getting put int value field failed ", CURRENT_FUNCTION);
    return;
  }

  int initVal = 3;
  putValue->put(initVal);

  std::tr1::shared_ptr<PVInt> getValuePtr = channelPutGetReq->getPVGetStructure()->getIntField("value");
  if (!getValuePtr.get()) {
    testFail("%s: getting get int value field failed ", CURRENT_FUNCTION);
    return;
  }

  PVTimeStamp pvTimeStamp;

  testOk(pvTimeStamp.attach(channelPutGetReq->getPVGetStructure()->getStructureField("timeStamp")),
      "%s: attaching a timestamp", CURRENT_FUNCTION);

  TimeStamp timeStamp;

  bool succStatus = channelPutGetReq->syncGetGet(getTimeoutSec());
  if (!succStatus) {
    testFail("%s: syncGetGet failed ", CURRENT_FUNCTION);
    return;
  } 
  else {
    testOk(true, "%s: a syncGetGet succeeded", CURRENT_FUNCTION);      
  }

  pvTimeStamp.get(timeStamp);

  int numOfTimes = 3;

  for (int i = 0; i < numOfTimes; i++) {

    int previousValue = getValuePtr->get();
    long previousTimestampSec = timeStamp.getSecondsPastEpoch();

    //cout << "previousValue:" << previousValue << " previousTimestampSec:" << previousTimestampSec << endl;
    //cout << "next val:" << ((previousValue +1) % 11) << endl;


    putValue->put((previousValue + 1) /*% 11*/);

    succStatus = channelPutGetReq->syncPutGet(i ==  numOfTimes, getTimeoutSec());
    if (!succStatus) {
      testFail("%s: syncPutGet failed ", CURRENT_FUNCTION);
      return;
    } 

    epicsThreadSleep(1.0);

    pvTimeStamp.get(timeStamp);

    int testValue = (previousValue +1 + 1) /*% 11*/; //+1 (new value) +1 (process)

    //cout << "Testing1:" << testValue << " == " << getValuePtr->get() << endl;
    //cout << "Testing2:" << timeStamp.getSecondsPastEpoch() << ">" << previousTimestampSec << endl; 
    testOk( testValue == getValuePtr->get(), "%s: testing the counter value change", 
        CURRENT_FUNCTION);      
    testOk(timeStamp.getSecondsPastEpoch() > previousTimestampSec, 
        "%s: testing the timestamp change", CURRENT_FUNCTION);      
  }     

  channel->destroy();
}


void ChannelAccessIFTest::test_channelRPC() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "record[process=true]putField(value)getField(timeStamp,value)"; 

  Channel::shared_pointer channel = syncCreateChannel(TEST_SUMRPC_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncChannelRPCRequesterImpl::shared_pointer channelRPCReq = syncCreateChannelRPC(channel);
  if (!channelRPCReq.get()) {
    testFail("%s: creating a channel rpc failed ", CURRENT_FUNCTION);
    return;
  }

  bool succStatus = channelRPCReq->syncRPC(
      createSumArgumentStructure(1,2),false, getTimeoutSec());
  if (succStatus) {
    testOk(true, "%s: calling sum rpc service successfull", CURRENT_FUNCTION);
    testOk(channelRPCReq->getLastResponse()->getIntField("c")->get() == 3, 
        "%s: rpc service returned correct sum", CURRENT_FUNCTION);
  }
  else {
    testFail("%s: error calling syncRPC ", CURRENT_FUNCTION);
  }

  channel->destroy();
}


void ChannelAccessIFTest::test_channelRPC_destroy() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "record[process=true]putField(value)getField(timeStamp,value)"; 

  Channel::shared_pointer channel = syncCreateChannel(TEST_SUMRPC_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncChannelRPCRequesterImpl::shared_pointer channelRPCReq = syncCreateChannelRPC(channel);
  if (!channelRPCReq.get()) {
    testFail("%s: creating a channel rpc failed ", CURRENT_FUNCTION);
    return;
  }

  testDiag("%s: destroying the channel rpc", CURRENT_FUNCTION); 
  channelRPCReq->getChannelRPC()->destroy();

  bool succStatus = channelRPCReq->syncRPC(
      createSumArgumentStructure(1,2),false, getTimeoutSec());
  if (succStatus) {
    testFail("%s: rpc call succeded on a destroyed channel ", CURRENT_FUNCTION);
  }
  else {
    testOk(true, "%s: rpc call should not succeed on a destroyed channel ", CURRENT_FUNCTION);
  }

  channel->destroy();
}


void ChannelAccessIFTest::test_channelRPCWithInvalidRequesterAndRequest() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "record[process=true]putField(value)getField(timeStamp,value)"; 

  Channel::shared_pointer channel = syncCreateChannel(TEST_SUMRPC_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  testSkip(1, " creating a channel rpc with a null channel");
  //SyncChannelRPCRequesterImpl::shared_pointer channelRPCReq = 
  //syncCreateChannelRPC(Channel::shared_pointer());

  SyncChannelRPCRequesterImpl::shared_pointer channelRPCReq = syncCreateChannelRPC(channel);
  if (!channelRPCReq.get()) {
    testFail("%s: creating a channel rpc failed ", CURRENT_FUNCTION);
    return;
  }


  testSkip(1, " skiping a rpc request with an empty pvArguments");
  //bool succStatus = channelRPCReq->syncRPC(
  //     PVStructure::shared_pointer(), false, getTimeoutSec());
  // if (!succStatus) {
  //   testOk(true, "%s: calling sum rpc service with an empty pvArguments should not succeed", CURRENT_FUNCTION);
  // }
  // else {
  //   testFail("%s: rpc request with an empty pvArguments succeeded ", CURRENT_FUNCTION);
  // }

  channel->destroy();
} 


void ChannelAccessIFTest::test_channelMonitorWithInvalidRequesterAndRequest() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "record[queueSize=3]field(timeStamp)"; 

  Channel::shared_pointer channel = syncCreateChannel(TEST_COUNTER_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  testSkip(true, " creating a monitoring channel with a null channel");

  //SyncMonitorRequesterImpl::shared_pointer monitorReq =  
  //  syncCreateChannelMonitor(Channel::shared_pointer(), request);


  SyncMonitorRequesterImpl::shared_pointer monitorReq1 =  
    syncCreateChannelMonitor(channel, string());
  if (monitorReq1.get()) {
    testOk(true, "%s: a monitor requester with an empty request created  ", CURRENT_FUNCTION);
  }
  else {
    testFail("%s: a monitor requester with an empty request not created ", CURRENT_FUNCTION);
  } 

  channel->destroy();
} 


void ChannelAccessIFTest::test_channelMonitor(int queueSize) {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);
  ostringstream ostream;
  ostream << queueSize;
  string request = "record[queueSize=";
  request.append(ostream.str());
  request.append("]field(timeStamp,value)");

  Channel::shared_pointer channel = syncCreateChannel(TEST_COUNTER_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncMonitorRequesterImpl::shared_pointer monitorReq =  
    syncCreateChannelMonitor(channel,request);
  if (!monitorReq.get()) {
    testFail("%s: creating a channel monitor failed ", CURRENT_FUNCTION);
    return;
  }

  testOk(monitorReq->getMonitorCounter() == 0, "%s: monitoring counter before calling start should be zero", 
      CURRENT_FUNCTION);

  monitorReq->getChannelMonitor()->start();

  bool succStatus = monitorReq->waitUntilMonitor(getTimeoutSec());
  if (!succStatus) {
    testFail("%s: no monitoring event happened ", CURRENT_FUNCTION);
    return;
  }
  else {
    testOk(monitorReq->getMonitorCounter() == 1, "%s: monitor event happened", CURRENT_FUNCTION);
    testOk(monitorReq->getChangedBitSet()->cardinality() == 1, "%s: monitor cardinality is 1", CURRENT_FUNCTION);
    testOk(monitorReq->getChangedBitSet()->get(0) == true, "%s: changeBitSet get(0) is true ", CURRENT_FUNCTION);
  }

  std::tr1::shared_ptr<PVField> valueField = monitorReq->getPVStructure()->getSubField("value");
  std::tr1::shared_ptr<PVField> previousValue = getPVDataCreate()->createPVField(valueField->getField());

  ConvertPtr convert = getConvert();
  convert->copy(valueField, previousValue);

  testOk(valueField->equals(*previousValue.get()) == true , "%s: value field equals to a previous value", 
      CURRENT_FUNCTION);

  for (int i = 2; i < 5; i++ ) {

    succStatus = monitorReq->waitUntilMonitor(getTimeoutSec());
    if (!succStatus) {
      testFail("%s: no further monitoring event happened ", CURRENT_FUNCTION);
      return;
    }

    testOk(monitorReq->getMonitorCounter() == i, "%s: monitor event happened for i=%d", CURRENT_FUNCTION, i);

    if (queueSize == 1 ) {
      testOk(monitorReq->getChangedBitSet()->cardinality() == 1, "%s: monitor cardinality is 1 (queue size = 1)", 
          CURRENT_FUNCTION);
      testOk(monitorReq->getChangedBitSet()->get(0) == true, "%s: changeBitSet get(0) is true (queue size = 1)", 
          CURRENT_FUNCTION); 
    }
    else {
      testOk(monitorReq->getChangedBitSet()->cardinality() == 2, "%s: monitor cardinality is 1 (queue size != 1)", 
          CURRENT_FUNCTION);
      testOk(monitorReq->getChangedBitSet()->get(1) == true, "%s: changeBitSet get(1) is true (queue size != 1)", 
          CURRENT_FUNCTION); 
      testOk(monitorReq->getChangedBitSet()->get(4) == true, "%s: changeBitSet get(4) is true (queue size != 1)", 
          CURRENT_FUNCTION); 

    } 


    valueField = monitorReq->getPVStructure()->getSubField("value");

    testOk(valueField->equals(*previousValue.get()) == false , "%s: value field not equals to a previous value", 
        CURRENT_FUNCTION);

    convert->copy(valueField, previousValue);
  } 


  monitorReq->getChannelMonitor()->stop(); 
  epicsThreadSleep(1.0);
  int mc = monitorReq->getMonitorCounter();
  epicsThreadSleep(2.0);
  testOk(mc == monitorReq->getMonitorCounter(), "%s: after monitor stop the counter should not increase",
      CURRENT_FUNCTION);
  channel->destroy();
}


void ChannelAccessIFTest::test_channelArray() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  Channel::shared_pointer channel = syncCreateChannel(TEST_ARRAY_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncChannelArrayRequesterImpl::shared_pointer arrayReq =  
    syncCreateChannelArray(channel, createArrayPvRequest());
  if (!arrayReq.get()) {
    testFail("%s: creating a channel array failed ", CURRENT_FUNCTION);
    return;
  }

  PVDoubleArrayPtr array = static_pointer_cast<PVDoubleArray>(arrayReq->getArray());

  bool succStatus = arrayReq->syncGet(false, 0, 2, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: an array syncGet failed (1) ", CURRENT_FUNCTION);
    return;
  }

  unsigned numOfElements = 5;
  PVDoubleArray::svector newdata;

  //inserting 1.1 2.2 3.3 4.4 5.5 
  for (unsigned i = 1 ; i <= numOfElements; i++) {
    double val = i+i*0.1;
    newdata.push_back(val);
  }

  array->replace(freeze(newdata));

  succStatus = arrayReq->syncPut(false, 0, 0, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: an array syncPut failed (2) ", CURRENT_FUNCTION);
    return;
  }

  succStatus = arrayReq->syncGet(false, 0, 0, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: an array syncGet failed (3) ", CURRENT_FUNCTION);
    return;
  }

  PVDoubleArray::const_svector data(array->view());
  testOk(data.size() == numOfElements, "%s: data array size should be %d", 
      CURRENT_FUNCTION, numOfElements);


  //checking 1.1 2.2 3.3 4.4 5.5
  for (unsigned i = 0; i < numOfElements; i++) {
    int ii = i + 1;
    testOk(data[i] == (ii+ii*0.1), "%s: data slot %d should be %f", CURRENT_FUNCTION, i, (ii+ii*0.1));
  } 


  succStatus = arrayReq->syncPut(false, 4, data.size(), getTimeoutSec());
  if (!succStatus) {
    testFail("%s: an array syncPut failed (5)", CURRENT_FUNCTION);
    return;
  }

  succStatus = arrayReq->syncGet(false, 3, 3, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: an array syncGet failed (6) ", CURRENT_FUNCTION);
    return;
  }


  PVDoubleArrayPtr array1 = static_pointer_cast<PVDoubleArray>(arrayReq->getArray());
  PVDoubleArray::const_svector data1(array1->view());
  testOk(data1[0] == 4.4 , "%s: 1.check 0: %f", CURRENT_FUNCTION, data1[0]);
  testOk(data1[1] == 1.1 , "%s: 1.check 1: %f", CURRENT_FUNCTION, data1[1]);
  // TODO java put can aut-extend array, C++ implementation does not
  testOk(data1.size() == 2 , "%s: data1.size() == 2", CURRENT_FUNCTION);
  //testOk(data1[2] == 2.2 , "%s: check 2: %f", CURRENT_FUNCTION, data1[2]);


  succStatus = arrayReq->syncSetLength(false, 3, 0, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: an array setLength failed ", CURRENT_FUNCTION);
    return;
  }

  succStatus = arrayReq->syncGet(false, 0, 0, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: an array syncGet failed (7) ", CURRENT_FUNCTION);
    return;
  }

  //checking 1.1 2.2 3.3
  PVDoubleArrayPtr array2 = static_pointer_cast<PVDoubleArray>(arrayReq->getArray());
  PVDoubleArray::const_svector data2(array2->view());
  testOk(data2.size() == 3, "%s: data size after calling setLength should be 3 ", 
      CURRENT_FUNCTION);
  testOk(data2[0] == 1.1 , "%s:  2.check 0: %f", CURRENT_FUNCTION, data2[0]);
  testOk(data2[1] == 2.2 , "%s:  2.check 1: %f", CURRENT_FUNCTION, data2[1]);
  testOk(data2[2] == 3.3,  "%s:  2.check 2: %f", CURRENT_FUNCTION, data2[2]);

  size_t currentLength = 3;
  size_t newCap = 2;
  succStatus = arrayReq->syncSetLength(false, currentLength, newCap, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: an array setLength failed (2) ", CURRENT_FUNCTION);
    return;
  }

  succStatus = arrayReq->syncGet(false, 0, 0, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: an array syncGet failed (8) ", CURRENT_FUNCTION);
    return;
  }

  //checking 1.1 2.2
  PVDoubleArrayPtr array3 = static_pointer_cast<PVDoubleArray>(arrayReq->getArray());
  PVDoubleArray::const_svector data3(array3->view());
  testOk(data3.size() == newCap, 
      "%s: data size after calling setLength should be %zu", CURRENT_FUNCTION, newCap);
  testOk(data3[0] == 1.1 , "%s: 3.check 0: %f", CURRENT_FUNCTION, data3[0]);
  testOk(data3[1] == 2.2 , "%s: 3.check 1: %f", CURRENT_FUNCTION, data3[1]);


  size_t bigCapacity = 10000;
  succStatus = arrayReq->syncSetLength(false, bigCapacity, bigCapacity, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: an array setLength failed (3) ", CURRENT_FUNCTION);
    return;
  }

  succStatus = arrayReq->syncGet(false, 0, 0, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: an array syncGet failed (9) ", CURRENT_FUNCTION);
    return;
  }
  
  PVDoubleArrayPtr array4 = static_pointer_cast<PVDoubleArray>(arrayReq->getArray());
  PVDoubleArray::const_svector data4(array4->view());
  testOk(data4.size() == bigCapacity, "%s: data size after calling setLength should be %zu", 
      CURRENT_FUNCTION, bigCapacity);
  testOk(data4[0] == 1.1 , "%s: 4.check 0: %f", CURRENT_FUNCTION, data4[0]);
  testOk(data4[1] == 2.2 , "%s: 4.check 1: %f", CURRENT_FUNCTION, data4[1]);
  /*
  if (data4.size() == bigCapacity) {
    size_t i = newCap;
    for (; i < bigCapacity; i++) {
      if (data4[i] != 0.0) {  // NOTE: floating-point number check, introduce epsilon value
        break;
      }
    }
    if (i == bigCapacity)
      testOk("%s: 4.check: all data 0.0", CURRENT_FUNCTION);
    else
      testFail("%s: 4.check: data at %zu should be 0.0 but was %f", CURRENT_FUNCTION, i, data4[i]);
  }
  else {
    testFail("%s: will not check the rest of the array if the size is not correct", CURRENT_FUNCTION);
  }
  */


  channel->destroy();
}


void ChannelAccessIFTest::test_channelArrayTestNoConnection() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  Channel::shared_pointer channel = syncCreateChannel(TEST_ARRAY_CHANNEL_NAME);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }


  SyncChannelArrayRequesterImpl::shared_pointer arrayReq =  
    syncCreateChannelArray(channel, createArrayPvRequest());
  if (!arrayReq.get()) {
    testFail("%s: creating a channel array failed ", CURRENT_FUNCTION);
    return;
  }

  testDiag("%s: Ok, let's destroy the channel", CURRENT_FUNCTION);

  channel->destroy();

  bool succStatus = arrayReq->syncGet(false, 1, 2, getTimeoutSec());
  if (succStatus) {
    testFail("%s: an array syncGet should fail after the channel had been destroyed ", CURRENT_FUNCTION);
  }
  else {
    testOk(true, "%s: an array syncGet failed on the destroyed channel", CURRENT_FUNCTION);
  }


  succStatus = arrayReq->syncPut(false, 1, 2, getTimeoutSec());
  if (succStatus) {
    testFail("%s: an array syncPut should fail after the channel had been destroyed ", CURRENT_FUNCTION);
  }
  else {
    testOk(true, "%s: an array syncPut failed on the destroyed channel", CURRENT_FUNCTION);
  }
  
  
  succStatus = arrayReq->syncSetLength(false, 1, 2, getTimeoutSec());
  if (succStatus) {
    testFail("%s: an array syncSetLength should fail after the channel had been destroyed ", CURRENT_FUNCTION);
  }
  else {
    testOk(true, "%s: an array syncSetLength failed on the destroyed channel", CURRENT_FUNCTION);
  }


}


void ChannelAccessIFTest::test_channelArray_destroy() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  bool debug = false;

  Channel::shared_pointer channel = syncCreateChannel(TEST_ARRAY_CHANNEL_NAME, debug);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncChannelArrayRequesterImpl::shared_pointer arrayReq =  
    syncCreateChannelArray(channel, createArrayPvRequest());
  if (!arrayReq.get()) {
    testFail("%s: creating a channel array failed ", CURRENT_FUNCTION);
    return;
  }

  bool succStatus = arrayReq->syncGet(false, 0, 2, getTimeoutSec());
  if (!succStatus) {
    testFail("%s: an array syncGet failed ", CURRENT_FUNCTION);
    return;
  }

  testDiag("%s: Destroying the channels", CURRENT_FUNCTION);

  channel->destroy();

  arrayReq->getChannelArray()->destroy();

  succStatus = arrayReq->syncGet(false, 0, 2, getTimeoutSec());
  if (!succStatus) {
    testOk(true,"%s: an array sync get should not succeed on a destroyed channel", CURRENT_FUNCTION);
  }
  else {
    testFail("%s: an array syncGet failed ", CURRENT_FUNCTION);
  }

}


void ChannelAccessIFTest::test_stressConnectDisconnect() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  int count = 300;
  bool debug = false;

  testDiag("%s: stress test connect-disconnect %d times", CURRENT_FUNCTION, count);

  for (int i = 0; i < count; i++) {
    Channel::shared_pointer channel = syncCreateChannel(TEST_VALUEONLY_CHANNEL_NAME, debug);
    if (!channel.get()) {
      testFail("%s: channel not created (%d) ", CURRENT_FUNCTION, i);
      return;
    }
    channel->destroy();
  }

  testOk(true, "%s: stress test connect-disconnect successfull", CURRENT_FUNCTION);
}


void ChannelAccessIFTest::test_stressConnectGetDisconnect() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "field(value)";  
  int count = 300;
  bool debug  = false;


  testDiag("%s: stress testing connect-get-disconnect %d times", CURRENT_FUNCTION, count);

  for (int i = 0; i < count; i++) {

    Channel::shared_pointer channel = syncCreateChannel(TEST_VALUEONLY_CHANNEL_NAME, debug);
    if (!channel.get()) {
      testFail("%s: channel not created (%d) ", CURRENT_FUNCTION, i);
      return;
    }


    SyncChannelGetRequesterImpl::shared_pointer channelGetReq = syncCreateChannelGet(channel, request, debug);
    if (!channelGetReq.get()) {
      testFail("%s: channel get not created (%d) ", CURRENT_FUNCTION, i);
      return;
    }

    bool succStatus = channelGetReq->syncGet(true, getTimeoutSec());
    if (!succStatus) {
      testFail("%s: sync get failed (%d) ", CURRENT_FUNCTION, i);
      return;
    } 

    channel->destroy();
  }

  testOk(true, "%s: stress test connect-disconnect successfull", CURRENT_FUNCTION);
}


void ChannelAccessIFTest::test_stressMonitorAndProcess() {

  testDiag("BEGIN TEST %s:", CURRENT_FUNCTION);

  string request = "record[queueSize=3]field(timeStamp,value,alarm.severity.choices)";
  bool debug = false;

  Channel::shared_pointer channel = syncCreateChannel(TEST_SIMPLECOUNTER_CHANNEL_NAME, debug);
  if (!channel.get()) {
    testFail("%s: channel not created ", CURRENT_FUNCTION);
    return;
  }

  SyncMonitorRequesterImpl::shared_pointer monitorReq =  
    syncCreateChannelMonitor(channel, request, debug);
  if (!monitorReq.get()) {
    testFail("%s: creating a channel monitor failed ", CURRENT_FUNCTION);
    return;
  }

  testOk(monitorReq->getMonitorCounter() == 0, "%s: monitoring counter before calling start should be zero", 
      CURRENT_FUNCTION);


  shared_ptr<SyncChannelProcessRequesterImpl> 
    channelProcessReq(new SyncChannelProcessRequesterImpl(debug));

  ChannelProcess::shared_pointer channelProcess = 
    channel->createChannelProcess(channelProcessReq,
        epics::pvData::PVStructure::shared_pointer()); 

  bool succStatus = channelProcessReq->waitUntilConnected(getTimeoutSec());
  if (!succStatus) {
    testFail("%s: a channel process is not connected ", CURRENT_FUNCTION);
    return;
  }


  monitorReq->getChannelMonitor()->start();

  succStatus = monitorReq->waitUntilMonitor(getTimeoutSec());
  if (!succStatus) {
    testFail("%s: no monitoring event happened ", CURRENT_FUNCTION);
    return;
  }
  else {
    testOk(monitorReq->getMonitorCounter() == 1, "%s: monitor event happened", CURRENT_FUNCTION);
  }

  int count = 50000;

  testDiag("%s: stress testing monitor-process %d times", CURRENT_FUNCTION, count);

  for (int i = 2; i < count; i++ ) {

    succStatus = channelProcessReq->syncProcess(false, getTimeoutSec());
    if (!succStatus) {
      testFail("%s: syncProcess failed (%d) ", CURRENT_FUNCTION, i);
      return;
    } 

    while(monitorReq->getMonitorCounter() < i) {
      monitorReq->waitUntilMonitor(getTimeoutSec());
    }        

    int counter = monitorReq->getMonitorCounter();

    if (counter != i ) {
      testFail("%s: monitor counter != i. monitorCounter: %d, i:%d", CURRENT_FUNCTION, counter, i);
      return;
    }
  } 

  testOk(true, "%s: stress testing monitor-process successfull", CURRENT_FUNCTION);
}


PVStructure::shared_pointer ChannelAccessIFTest::createSumArgumentStructure(int a, int b) {

  int i = 0;
  StringArray fieldNames(2);
  FieldConstPtrArray fields(2);
  fieldNames[i] = "a";
  fields[i++] = getFieldCreate()->createScalar(pvInt);
  fieldNames[i] = "b";
  fields[i++] = getFieldCreate()->createScalar(pvInt);

  PVStructure::shared_pointer pvArguments(
      getPVDataCreate()->createPVStructure(
        getFieldCreate()->createStructure(fieldNames, fields)));

  pvArguments->getIntField("a")->put(a);
  pvArguments->getIntField("b")->put(b);

  return pvArguments;

}


PVStructure::shared_pointer ChannelAccessIFTest::createArrayPvRequest() {
  
  StringArray fieldNames; fieldNames.push_back("field");
  FieldConstPtrArray fields; fields.push_back(getFieldCreate()->createScalar(pvString));
  PVStructure::shared_pointer pvRequest(getPVDataCreate()->createPVStructure(
        getFieldCreate()->createStructure(fieldNames, fields)));

  PVString::shared_pointer pvFieldName = pvRequest->getStringField("field");
  pvFieldName->put("value");
  return pvRequest;
}
