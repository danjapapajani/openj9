/*******************************************************************************
 * Copyright IBM Corp. and others 2000
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#ifndef J9_PERSISTENTINFO_HPP
#define J9_PERSISTENTINFO_HPP

#ifndef J9_PERSISTENTINFO_CONNECTOR
#define J9_PERSISTENTINFO_CONNECTOR
namespace J9 { class PersistentInfo; }
namespace J9 { typedef J9::PersistentInfo PersistentInfoConnector; }
#endif

#include "env/OMRPersistentInfo.hpp"
#include "env/RuntimeAssumptionTable.hpp"

#include <stdint.h>
#if defined(J9VM_OPT_JITSERVER)
#include <string>
#endif /* defined(J9VM_OPT_JITSERVER) */
#include "env/jittypes.h"

class TR_FrontEnd;
class TR_PersistentMemory;
class TR_PersistentCHTable;
class TR_PersistentClassLoaderTable;
class TR_AOTDependencyTable;
class TR_MHJ2IThunkTable;
namespace J9 { class Options; }

enum JitStates {
   UNDEFINED_STATE = 0,
   IDLE_STATE,
   STARTUP_STATE,
   RAMPUP_STATE,
   STEADY_STATE,
   DEEPSTEADY_STATE,
};

enum IprofilerStates {
   IPROFILING_STATE_ON = 1,
   IPROFILING_STATE_GOING_OFF,
   IPROFILING_STATE_OFF,
};

#if defined(J9VM_OPT_JITSERVER)
namespace JITServer
{
enum RemoteCompilationModes
   {
   NONE = 0,
   CLIENT,
   SERVER,
   };

enum ServerMemoryState
   {
   VERY_LOW = 0,
   LOW,
   NORMAL,
   };

enum ServerActiveThreadsState
   {
   VERY_HIGH_THREAD = 0,
   HIGH_THREAD,
   NORMAL_THREAD,
   };

enum CompThreadActivationPolicy
   {
   // Order is important, we use comparison operators
   // on policy states and for indexing into the name array
   SUSPEND = 0,
   MAINTAIN,
   SUBDUE,
   AGGRESSIVE,
   };

static const char *compThreadActivationPolicyNames[] = {
   "SUSPEND",
   "MAINTAIN",
   "SUBDUE",
   "AGGRESSIVE",
   };
}
#endif /* defined(J9VM_OPT_JITSERVER) */

#define MAX_SUPERCLASSES (20000)
namespace J9
{

class PersistentInfo : public OMR::PersistentInfoConnector
   {
   friend class J9::Options;

   public:

   PersistentInfo(TR_PersistentMemory *pm) :
#pragma warning( suppress : 4351 )
         _paddingBefore(),
         _countForRecompile(0),
         _numLoadedClasses(0),
         _classLoadingPhase(false),
         _inlinerTemporarilyRestricted(false),
         _elapsedTime(0),
         _unloadedClassAddresses(NULL),
         _unloadedMethodAddresses(NULL),
         _runtimeAssumptionTable(),
         _persistentCHTable(NULL),
         _visitedSuperClasses(NULL),
         _numVisitedSuperClasses(0),
         _tooManySuperClasses(false),
         _persistentClassLoaderTable(NULL),
         _GCwillBlockOnClassUnloadMonitor(false),
         _timeGCwillBlockOnClassUnloadMonitorWasSet(0),
         _numUnloadedClasses(0),
         _classLoadingPhaseGracePeriod(0),
         _startTime(0),
         _globalClassUnloadID(0),
         _externalStartupEndedSignal(false),
         _disableFurtherCompilation(false),
         _loadFactor(1),
         _jitState(STARTUP_STATE),
         _jitStateChangeSampleCount(0),
         _jitTotalSampleCount(0),
         _jitSampleCountWhenActiveStateEntered(0),
         _jitSampleCountWhenStartupStateEntered(0),
         _jitSampleCountWhenStartupStateExited(0),
         _vmTotalCpuTimeWhenStartupStateEntered(0),
         _vmTotalCpuTimeWhenStartupStateExited(0),
         _lateSCCDisclaimTime(300000000000), // 300s
         _inliningAggressiveness(100),
         _lastTimeSamplerThreadEnteredIdle(0),
         _lastTimeSamplerThreadEnteredDeepIdle(0),
         _lastTimeSamplerThreadWasSuspended(0),
         _lastTimeThreadsWereActive(0),
         _gcMapCheckEventHandleKey(0),
         _statNumGCRBodies(0),
         _statNumGCRSaves(0),
         _invokeExactJ2IThunkTable(NULL),
         _gpuInitMonitor(NULL),
         _runtimeInstrumentationEnabled(false),
         _runtimeInstrumentationRecompilationEnabled(false),
         _aotDependencyTable(NULL),
         _trackAOTDependencies(false),
#if defined(J9VM_OPT_JITSERVER)
         _JITServerAddress("localhost"),
         _JITServerPort(38400),
         _socketTimeoutMs(0),
         _clientUID(0),
         _serverUID(0),
         _hasEverConnectedToServer(false),
         _JITServerMetricsPort(38500),
         _JITServerUseHealthPort(true),
         _JITServerHealthPort(38600),
         _requireJITServer(false),
         _localSyncCompiles(true),
         _JITServerUseAOTCache(false),
         _JITServerAOTCacheName("default"),
         _JITServerUseAOTCachePersistence(false),
         _JITServerAOTCacheDir(),
         _JITServerAOTCacheDelayMethodRelocation(false),
         _JITServerAOTCacheIgnoreLocalSCC(true),
         _doNotRequestJITServerAOTCacheLoad(false),
         _doNotRequestJITServerAOTCacheStore(false),
#endif /* defined(J9VM_OPT_JITSERVER) */
      OMR::PersistentInfoConnector(pm)
      {}

   void setPersistentClassLoaderTable(TR_PersistentClassLoaderTable *table) { _persistentClassLoaderTable = table; }
   TR_PersistentClassLoaderTable *getPersistentClassLoaderTable() { return _persistentClassLoaderTable; }

   void setAOTDependencyTable(TR_AOTDependencyTable *table) { _aotDependencyTable = table; }
   TR_AOTDependencyTable *getAOTDependencyTable() const { return _aotDependencyTable; }

   TR_OpaqueClassBlock **getVisitedSuperClasses() { return _visitedSuperClasses; }
   void clearVisitedSuperClasses() { _tooManySuperClasses = false; _numVisitedSuperClasses = 0; }
   void setVisitedSuperClasses(TR_OpaqueClassBlock **v) { _visitedSuperClasses = v; }
   bool tooManySuperClasses() { return _tooManySuperClasses; }
   void setTooManySuperClasses(bool b) { _tooManySuperClasses = b; }
   int32_t getNumVisitedSuperClasses() {return _numVisitedSuperClasses; }
   void setNumVisitedSuperClasses(int32_t n) {_numVisitedSuperClasses = n; }
   void addSuperClass(TR_OpaqueClassBlock *c)
      {
      if (_visitedSuperClasses && _numVisitedSuperClasses < MAX_SUPERCLASSES)
         _visitedSuperClasses[_numVisitedSuperClasses++] = c;
      else
         _tooManySuperClasses = true;
      }

   bool GCwillBlockOnClassUnloadMonitor() { return _GCwillBlockOnClassUnloadMonitor; }
   void setGCwillBlockOnClassUnloadMonitor() { _GCwillBlockOnClassUnloadMonitor = true; _timeGCwillBlockOnClassUnloadMonitorWasSet = _elapsedTime;}
   void resetGCwillBlockOnClassUnloadMonitor() { _GCwillBlockOnClassUnloadMonitor = false; }

   bool ensureUnloadedAddressSetsAreInitialized();
   void addUnloadedClass(TR_OpaqueClassBlock *clazz, uintptr_t startAddress, uint32_t size);
   bool isUnloadedClass(void *v, bool yesIReallyDontCareAboutHCR); // You probably want isObsoleteClass
   bool isInUnloadedMethod(uintptr_t address);
   void incNumUnloadedClasses(int32_t count) { _numUnloadedClasses += count; }
   int32_t getNumUnloadedClasses() const { return _numUnloadedClasses; }
   TR_AddressSet* getUnloadedClassAddresses() const { return _unloadedClassAddresses; }

   void incNumLoadedClasses() {_numLoadedClasses++;}

   int32_t getClassLoadingPhaseGracePeriod() { return _classLoadingPhaseGracePeriod; }
   void setClassLoadingPhaseGracePeriod(int32_t val) { _classLoadingPhaseGracePeriod = val; }

   uint64_t getStartTime() const { return _startTime;}
   void setStartTime(uint64_t t) {_startTime = t;}

   void setElapsedTime(uint64_t t) {_elapsedTime = t;}
   void updateElapsedTime(uint64_t t) {_elapsedTime += t;}

   int32_t getGlobalClassUnloadID() const {return _globalClassUnloadID;}
   void incGlobalClassUnloadID() {_globalClassUnloadID++;}

   bool getExternalStartupEndedSignal() const { return _externalStartupEndedSignal; }
   void setExternalStartupEndedSignal(bool b) { _externalStartupEndedSignal = b; }

   bool isObsoleteClass(void *v, TR_FrontEnd *fe);
   TR_PseudoRandomNumbersListElement *advanceCurPseudoRandomNumbersListElem();

   bool getDisableFurtherCompilation() const { return _disableFurtherCompilation; }
   void setDisableFurtherCompilation(bool b) { _disableFurtherCompilation = b; }

   uint32_t getLoadFactor() const { return _loadFactor; }
   void setLoadFactor(uint32_t f) { _loadFactor = f; }

   uint8_t getJitState() const { return _jitState; }
   void setJitState(uint8_t v) { _jitState = v; }

   void setJitStateChangeSampleCount(uint32_t t) {_jitStateChangeSampleCount = t;}
   uint32_t getJitStateChangeSampleCount() const { return _jitStateChangeSampleCount;}

   uint32_t getJitTotalSampleCount() const { return _jitTotalSampleCount; }
   void incJitTotalSampleCount() { _jitTotalSampleCount++; }

   void setJitSampleCountWhenStartupStateEntered(uint32_t n) { _jitSampleCountWhenStartupStateEntered = n; }
   uint32_t getJitSampleCountWhenStartupStateEntered() const { return _jitSampleCountWhenStartupStateEntered; }

   void setJitSampleCountWhenStartupStateExited(uint32_t n) { _jitSampleCountWhenStartupStateExited = n; }
   uint32_t getJitSampleCountWhenStartupStateExited() const { return _jitSampleCountWhenStartupStateExited; }

   void setJitSampleCountWhenActiveStateEntered(uint32_t n) { _jitSampleCountWhenActiveStateEntered = n; }
   uint32_t getJitSampleCountWhenActiveStateEntered() const { return _jitSampleCountWhenActiveStateEntered; }

   void setVmTotalCpuTimeWhenStartupStateEntered(int64_t n) { _vmTotalCpuTimeWhenStartupStateEntered = n; }
   int64_t getVmTotalCpuTimeWhenStartupStateEntered() const { return _vmTotalCpuTimeWhenStartupStateEntered; }

   void setVmTotalCpuTimeWhenStartupStateExited(int64_t n) { _vmTotalCpuTimeWhenStartupStateExited = n; }
   int64_t getVmTotalCpuTimeWhenStartupStateExited() const { return _vmTotalCpuTimeWhenStartupStateExited; }

   void setLateSCCDisclaimTime(int64_t t) { _lateSCCDisclaimTime = t; }
   int64_t getLateSCCDisclaimTime() const { return _lateSCCDisclaimTime; }

   void setInliningAggressiveness(int32_t n) { _inliningAggressiveness = n; }
   int32_t getInliningAggressiveness() const { return _inliningAggressiveness; }

   uint64_t getLastTimeSamplerThreadEnteredIdle() const { return _lastTimeSamplerThreadEnteredIdle; }
   void setLastTimeSamplerThreadEnteredIdle(uint64_t t) { _lastTimeSamplerThreadEnteredIdle = t; }

   uint64_t getLastTimeSamplerThreadEnteredDeepIdle() const { return _lastTimeSamplerThreadEnteredDeepIdle; }
   void setLastTimeSamplerThreadEnteredDeepIdle(uint64_t t) { _lastTimeSamplerThreadEnteredDeepIdle = t; }

   uint64_t getLastTimeThreadsWereActive() const { return _lastTimeThreadsWereActive; }
   void setLastTimeThreadsWereActive(uint64_t t) { _lastTimeThreadsWereActive = t; }

   uint64_t getLastTimeSamplerThreadWasSuspended() const { return _lastTimeSamplerThreadWasSuspended; }
   void setLastTimeSamplerThreadWasSuspended(uint64_t t) { _lastTimeSamplerThreadWasSuspended = t; }

   void setGCMapCheckEventHandle(uint32_t handle) {_gcMapCheckEventHandleKey = handle;}
   uint32_t getGCMapCheckEventHandle() {return _gcMapCheckEventHandleKey;}

   int32_t getNumGCRBodies() const { return _statNumGCRBodies; }
   int32_t getNumGCRSaves() const { return _statNumGCRSaves; }

   void incNumGCRBodies() { _statNumGCRBodies++; }
   void incNumGCRSaves() { _statNumGCRSaves++; }

   TR_MHJ2IThunkTable *getInvokeExactJ2IThunkTable(){ return _invokeExactJ2IThunkTable; } // NULL if the platform needs no thunks, so J2I helpers can be called directly
   void setInvokeExactJ2IThunkTable(TR_MHJ2IThunkTable *table){ _invokeExactJ2IThunkTable = table; }


   TR_PersistentCHTable * getPersistentCHTable();
   void setPersistentCHTable(TR_PersistentCHTable *table);

   TR_RuntimeAssumptionTable *getRuntimeAssumptionTable() { return &_runtimeAssumptionTable; }


   TR::Monitor * getGpuInitializationMonitor() { return _gpuInitMonitor; }
   void setGpuInitializationMonitor(TR::Monitor * gpuInitMonitor) {_gpuInitMonitor = gpuInitMonitor;}


   bool isRuntimeInstrumentationEnabled() { return _runtimeInstrumentationEnabled; }
   void setRuntimeInstrumentationEnabled(bool b)
      {
      _runtimeInstrumentationEnabled = b;

      // If RI is disabled, then the exploitations need to be disabled too
      if (b == false)
         {
         _runtimeInstrumentationRecompilationEnabled = false;
         }
      }

   bool isRuntimeInstrumentationRecompilationEnabled() { return _runtimeInstrumentationEnabled && _runtimeInstrumentationRecompilationEnabled; }
   void setRuntimeInstrumentationRecompilationEnabled(bool b)
      {
      if (_runtimeInstrumentationEnabled)
         _runtimeInstrumentationRecompilationEnabled = b;
      else
         _runtimeInstrumentationRecompilationEnabled = false;
      }


   bool getInlinerTemporarilyRestricted() const { return _inlinerTemporarilyRestricted; }
   void setInlinerTemporarilyRestricted(bool b) { _inlinerTemporarilyRestricted = b; }


   bool isClassLoadingPhase() const {return _classLoadingPhase;}
   void setClassLoadingPhase(bool b) {_classLoadingPhase = b;}

   int32_t getNumLoadedClasses() const { return _numLoadedClasses;}
   uint64_t getElapsedTime() const { return _elapsedTime;}

   // these fields are mostly read, rarely written
   /* _countForRecompile can be frequently accessed by *application threads*.
    * If it is placed on a cache line that frequently gets updated, it will be constantly evicted,
    * which can cause a significant performance peanalty.
    */
   uint8_t _paddingBefore[128];
   int32_t _countForRecompile;

  void setTrackAOTDependencies(bool b) { _trackAOTDependencies = b;}
  bool getTrackAOTDependencies() const { return _trackAOTDependencies; }

#if defined(J9VM_OPT_JITSERVER)
   static JITServer::RemoteCompilationModes _remoteCompilationMode; // JITServer::NONE, JITServer::CLIENT, JITServer::SERVER

   static JITServer::RemoteCompilationModes getRemoteCompilationMode() { return _remoteCompilationMode; }
   const std::string &getJITServerAddress() const { return _JITServerAddress; }
   void setJITServerAddress(const char *addr) { _JITServerAddress = addr; }
   uint32_t getSocketTimeout() const { return _socketTimeoutMs; }
   void setSocketTimeout(uint32_t t) { _socketTimeoutMs = t; }
   uint32_t getJITServerPort() const { return _JITServerPort; }
   void setJITServerPort(uint32_t port) { _JITServerPort = port; }
   uint64_t getClientUID() const { return _clientUID; }
   void setClientUID(uint64_t val) { _clientUID = val; }
   uint64_t getServerUID() const { return _serverUID; }
   void setServerUID(uint64_t val) { _serverUID = val; }
   bool hasEverConnectedToServer() const { return _hasEverConnectedToServer; }
   void setHasEverConnectedToServer() { _hasEverConnectedToServer = true; }
   uint32_t getJITServerMetricsPort() const { return _JITServerMetricsPort; }
   void setJITServerMetricsPort(uint32_t port) { _JITServerMetricsPort = port; }
   bool getJITServerUseHealthPort() const { return _JITServerUseHealthPort; }
   void setJITServerUseHealthPort(bool b) { _JITServerUseHealthPort = b; }
   uint32_t getJITServerHealthPort() const { return _JITServerHealthPort; }
   void setJITServerHealthPort(uint32_t port) { _JITServerHealthPort = port; }
   bool getRequireJITServer() const { return _requireJITServer; }
   void setRequireJITServer(bool requireJITServer) { _requireJITServer = requireJITServer; }
   bool isLocalSyncCompiles() const { return _localSyncCompiles; }
   void setLocalSyncCompiles(bool localSyncCompiles) { _localSyncCompiles = localSyncCompiles; }
   bool getJITServerUseAOTCache() const { return _JITServerUseAOTCache; }
   void setJITServerUseAOTCache(bool use) { _JITServerUseAOTCache = use; }
   const std::string &getJITServerAOTCacheName() const { return _JITServerAOTCacheName; }
   void setJITServerAOTCacheName(const char *name) { _JITServerAOTCacheName = name; }
   bool getJITServerUseAOTCachePersistence() const { return _JITServerUseAOTCachePersistence; }
   void setJITServerUseAOTCachePersistence(bool use) { _JITServerUseAOTCachePersistence = use; }
   const std::string &getJITServerAOTCacheDir() const { return _JITServerAOTCacheDir; }
   void setJITServerAOTCacheDir(const char *dir) { _JITServerAOTCacheDir = dir; }
   bool getJITServerAOTCacheDelayMethodRelocation() const { return _JITServerAOTCacheDelayMethodRelocation; }
   void setJITServerAOTCacheDelayMethodRelocation(bool b) { _JITServerAOTCacheDelayMethodRelocation = b; }
   bool getJITServerAOTCacheIgnoreLocalSCC() const { return _JITServerAOTCacheIgnoreLocalSCC; }
   void setJITServerAOTCacheIgnoreLocalSCC(bool b) { _JITServerAOTCacheIgnoreLocalSCC = b; }
   bool doNotRequestJITServerAOTCacheLoad() const { return _doNotRequestJITServerAOTCacheLoad; }
   void setDoNotRequestJITServerAOTCacheLoad(bool b) { _doNotRequestJITServerAOTCacheLoad = b; }
   bool doNotRequestJITServerAOTCacheStore() const { return _doNotRequestJITServerAOTCacheStore; }
   void setDoNotRequestJITServerAOTCacheStore(bool b) { _doNotRequestJITServerAOTCacheStore = b; }
#endif /* defined(J9VM_OPT_JITSERVER) */

   private:
   TR_AddressSet *_unloadedClassAddresses;
   TR_AddressSet *_unloadedMethodAddresses;

   TR_RuntimeAssumptionTable _runtimeAssumptionTable;
   TR_PersistentCHTable *_persistentCHTable;

   TR_PersistentClassLoaderTable *_persistentClassLoaderTable;

   TR_AOTDependencyTable *_aotDependencyTable;

   // these fields are RW

   TR_OpaqueClassBlock **_visitedSuperClasses;
   int32_t _numVisitedSuperClasses;
   bool _tooManySuperClasses;

   uint64_t _timeGCwillBlockOnClassUnloadMonitorWasSet; // ms; set by GC; RAS
   bool _GCwillBlockOnClassUnloadMonitor;

   int32_t _classLoadingPhaseGracePeriod; // in ms

   uint64_t _startTime;   // time when sampling thread started (ms)

   int32_t _numUnloadedClasses;

   int32_t _globalClassUnloadID; // incremented each time GC does a class unload

   bool _externalStartupEndedSignal; // the app will tell us when startup ends

   volatile bool _disableFurtherCompilation;

   uint32_t _loadFactor; // set in samplerThread; increases with active threads, decreases with CPUs

   uint8_t _jitState; // STARTUP IDLE RAMPUP STEADY

   uint32_t _jitTotalSampleCount; // similar to TR::Recompilation::globalSampleCount,
                                  // but always incremented when jitMethodSampleInterrupt is called

   uint32_t _jitStateChangeSampleCount; // caches _jitTotalSampleCount when a state change has occurred

   uint32_t _jitSampleCountWhenActiveStateEntered; // set when we leave IDLE of STARTUP

   uint32_t _jitSampleCountWhenStartupStateEntered; // set when we enter STARTUP from another state

   uint32_t _jitSampleCountWhenStartupStateExited; // set when we exit STARTUP state

   int64_t _vmTotalCpuTimeWhenStartupStateEntered; // set when we enter STARTUP from another state

   int64_t _vmTotalCpuTimeWhenStartupStateExited;  // set when we exit STARTUP state

   int64_t _lateSCCDisclaimTime; // CPU time in ns

   int32_t _inliningAggressiveness;

   // The following four fields are stored here for RAS purposes (easy access from a debugging session)
   //
   uint64_t _lastTimeSamplerThreadEnteredIdle; // used by samplerThread
   uint64_t _lastTimeSamplerThreadEnteredDeepIdle;
   uint64_t _lastTimeSamplerThreadWasSuspended;
   uint64_t _lastTimeThreadsWereActive; // used by samplerThread

   uint32_t _gcMapCheckEventHandleKey;

   // Stats for GCR bodies generated and how many times we avoided generating a GCR body
   //
   int32_t _statNumGCRBodies;
   int32_t _statNumGCRSaves;

   TR_MHJ2IThunkTable *_invokeExactJ2IThunkTable;

   TR::Monitor *_gpuInitMonitor;

   bool _runtimeInstrumentationEnabled;
   bool _runtimeInstrumentationRecompilationEnabled;

   bool _classLoadingPhase;            ///< true, if we detect a large number of classes loaded per second
   bool _inlinerTemporarilyRestricted; ///< do not inline when true; used to restrict cold inliner during startup

   volatile uint64_t _elapsedTime; ///< elapsed time as computed by the sampling thread (ms)
                                   ///< May need adjustment if sampling thread goes to sleep

   int32_t _numLoadedClasses; ///< always increasing

   bool _trackAOTDependencies;

#if defined(J9VM_OPT_JITSERVER)
   std::string _JITServerAddress;
   uint32_t    _JITServerPort;
   uint32_t    _socketTimeoutMs; // timeout for communication sockets used in out-of-process JIT compilation
   uint64_t    _clientUID;
   uint64_t    _serverUID; // At the client, this represents the UID of the server the client is connected to
   bool        _hasEverConnectedToServer; // At the client, true if the client has connected to a server at some point in the past
   uint32_t    _JITServerMetricsPort; // Port for receiving http metrics requests from Prometheus; only used at server
   bool        _JITServerUseHealthPort; // True if the server should open _JITServerHealthPort
   uint32_t    _JITServerHealthPort; // Port for receiving readiness/liveness probes from Kubernetes; only used at server
   bool        _requireJITServer;
   bool        _localSyncCompiles;
   bool        _JITServerUseAOTCache;
   std::string _JITServerAOTCacheName; // Name of the server AOT cache that this client is using
   bool        _JITServerUseAOTCachePersistence; // Whether to persist the JITServer AOT caches at the server
   std::string _JITServerAOTCacheDir;  // Directory where the JITServer persistent AOT caches are located
   bool        _JITServerAOTCacheDelayMethodRelocation; // At the client, whether to delay deserialized method relocation or not
   // At the client, whether or not to use the new AOT cache implementation (with serialization record IDs as SCC offsets)
   bool        _JITServerAOTCacheIgnoreLocalSCC;
   // True if the client should not request AOT cache loads during this server connection
   bool        _doNotRequestJITServerAOTCacheLoad;
   // True if the client should not request AOT cache stores during this server connection
   bool        _doNotRequestJITServerAOTCacheStore;
#endif /* defined(J9VM_OPT_JITSERVER) */
   };

}

#endif
