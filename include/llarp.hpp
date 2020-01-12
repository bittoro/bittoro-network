#ifndef LLARP_HPP
#define LLARP_HPP
#include <llarp.h>
#include <util/fs.hpp>
#include <util/types.hpp>
#include <ev/ev.hpp>

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct llarp_ev_loop;
struct llarp_nodedb;
struct llarp_nodedb_iter;
struct llarp_main;

namespace llarp
{
  class Logic;
  struct AbstractRouter;
  struct Config;
  struct Crypto;
  struct CryptoManager;
  struct MetricsConfig;
  struct RouterContact;
  namespace thread
  {
    class ThreadPool;
  }

  namespace metrics
  {
    class DefaultManagerGuard;
    class PublisherScheduler;
  }  // namespace metrics

  namespace thread
  {
    class Scheduler;
  }

  struct Context
  {
    /// get context from main pointer
    static Context *
    Get(llarp_main *);

    Context();
    ~Context();

    // These come first, in this order.
    // This ensures we get metric collection on shutdown
    std::unique_ptr< thread::Scheduler > m_scheduler;
    std::unique_ptr< metrics::DefaultManagerGuard > m_metricsManager;
    std::unique_ptr< metrics::PublisherScheduler > m_metricsPublisher;

    std::unique_ptr< Crypto > crypto;
    std::unique_ptr< CryptoManager > cryptoManager;
    std::unique_ptr< AbstractRouter > router;
    std::shared_ptr< thread::ThreadPool > worker;
    std::shared_ptr< Logic > logic;
    std::unique_ptr< Config > config;
    std::unique_ptr< llarp_nodedb > nodedb;
    llarp_ev_loop_ptr mainloop;
    std::string nodedb_dir;

    bool
    LoadConfig(const std::string &fname);

    void
    Close();

    int
    LoadDatabase();

    int
    Setup();

    int
    Run(llarp_main_runtime_opts opts);

    void
    HandleSignal(int sig);

    bool
    Configure();

    bool
    IsUp() const;

    bool
    LooksAlive() const;

    /// close async
    void
    CloseAsync();

    /// wait until closed and done
    void
    Wait();

    /// call a function in logic thread
    /// return true if queued for calling
    /// return false if not queued for calling
    bool
    CallSafe(std::function< void(void) > f);

   private:
    void
    SetPIDFile(const std::string &fname);

    bool
    WritePIDFile() const;

    void
    RemovePIDFile() const;

    void
    SigINT();

    bool
    ReloadConfig();

    void
    setupMetrics(const MetricsConfig &metricsConfig);

    std::string configfile;
    std::string pidfile;
    std::unique_ptr< std::promise< void > > closeWaiter;
  };
}  // namespace llarp

#endif
