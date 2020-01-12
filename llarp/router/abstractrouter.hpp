#ifndef LLARP_ABSTRACT_ROUTER_HPP
#define LLARP_ABSTRACT_ROUTER_HPP

#include <config/key_manager.hpp>
#include <memory>
#include <util/types.hpp>
#include <util/status.hpp>
#include <router/i_outbound_message_handler.hpp>
#include <vector>
#include <ev/ev.h>
#include <functional>
#include <router_contact.hpp>

struct llarp_buffer_t;
struct llarp_dht_context;
struct llarp_nodedb;
struct llarp_threadpool;

namespace llarp
{
  class Logic;
  struct Config;
  struct RouterID;
  struct ILinkMessage;
  struct ILinkSession;
  struct PathID_t;
  struct Profiling;
  struct SecretKey;
  struct Signature;
  struct IOutboundMessageHandler;
  struct IOutboundSessionMaker;
  struct ILinkManager;
  struct I_RCLookupHandler;

  namespace exit
  {
    struct Context;
  }

  namespace path
  {
    struct PathContext;
  }

  namespace routing
  {
    struct IMessageHandler;
  }

  namespace service
  {
    struct Context;
  }

  namespace thread
  {
    class ThreadPool;
  }

  struct AbstractRouter
  {
    virtual ~AbstractRouter() = default;

    virtual bool
    HandleRecvLinkMessageBuffer(ILinkSession *from,
                                const llarp_buffer_t &msg) = 0;

    virtual std::shared_ptr< Logic >
    logic() const = 0;

    virtual llarp_dht_context *
    dht() const = 0;

    virtual llarp_nodedb *
    nodedb() const = 0;

    virtual const path::PathContext &
    pathContext() const = 0;

    virtual path::PathContext &
    pathContext() = 0;

    virtual const RouterContact &
    rc() const = 0;

    virtual exit::Context &
    exitContext() = 0;

    virtual std::shared_ptr< KeyManager >
    keyManager() const = 0;

    virtual const SecretKey &
    identity() const = 0;

    virtual const SecretKey &
    encryption() const = 0;

    virtual Profiling &
    routerProfiling() = 0;

    virtual llarp_ev_loop_ptr
    netloop() const = 0;

    virtual std::shared_ptr< thread::ThreadPool >
    threadpool() = 0;

    virtual std::shared_ptr< thread::ThreadPool >
    diskworker() = 0;

    virtual service::Context &
    hiddenServiceContext() = 0;

    virtual const service::Context &
    hiddenServiceContext() const = 0;

    virtual IOutboundMessageHandler &
    outboundMessageHandler() = 0;

    virtual IOutboundSessionMaker &
    outboundSessionMaker() = 0;

    virtual ILinkManager &
    linkManager() = 0;

    virtual I_RCLookupHandler &
    rcLookupHandler() = 0;

    virtual bool
    Sign(Signature &sig, const llarp_buffer_t &buf) const = 0;

    virtual bool
    Configure(Config *conf, llarp_nodedb *nodedb) = 0;

    virtual bool
    IsServiceNode() const = 0;

    virtual bool
    StartJsonRpc() = 0;

    virtual bool
    Run() = 0;

    virtual bool
    IsRunning() const = 0;

    virtual bool
    LooksAlive() const = 0;

    /// stop running the router logic gracefully
    virtual void
    Stop() = 0;

    /// pump low level links
    virtual void
    PumpLL() = 0;

    virtual bool
    IsBootstrapNode(RouterID r) const = 0;

    virtual const byte_t *
    pubkey() const = 0;

    /// connect to N random routers
    virtual void
    ConnectToRandomRouters(int N) = 0;
    /// inject configuration and reconfigure router
    virtual bool
    Reconfigure(Config *conf) = 0;

    virtual bool
    TryConnectAsync(RouterContact rc, uint16_t tries) = 0;

    /// validate new configuration against old one
    /// return true on 100% valid
    /// return false if not 100% valid
    virtual bool
    ValidateConfig(Config *conf) const = 0;

    /// called by link when a remote session has no more sessions open
    virtual void
    SessionClosed(RouterID remote) = 0;

    /// returns system clock milliseconds since epoch
    virtual llarp_time_t
    Now() const = 0;

    /// returns milliseconds since started
    virtual llarp_time_t
    Uptime() const = 0;

    virtual bool
    GetRandomGoodRouter(RouterID &r) = 0;

    virtual bool
    SendToOrQueue(const RouterID &remote, const ILinkMessage *msg,
                  SendStatusHandler handler = nullptr) = 0;

    virtual void
    PersistSessionUntil(const RouterID &remote, llarp_time_t until) = 0;

    virtual bool
    ParseRoutingMessageBuffer(const llarp_buffer_t &buf,
                              routing::IMessageHandler *h,
                              const PathID_t &rxid) = 0;

    /// count the number of service nodes we are connected to
    virtual size_t
    NumberOfConnectedRouters() const = 0;

    /// count the number of clients that are connected to us
    virtual size_t
    NumberOfConnectedClients() const = 0;

    virtual bool
    GetRandomConnectedRouter(RouterContact &result) const = 0;

    virtual void
    HandleDHTLookupForExplore(RouterID remote,
                              const std::vector< RouterContact > &results) = 0;

    /// lookup router by pubkey
    /// if we are a service node this is done direct otherwise it's done via
    /// path
    virtual void
    LookupRouter(RouterID remote, RouterLookupHandler resultHandler) = 0;

    /// check if newRc matches oldRC and update local rc for this remote contact
    /// if valid
    /// returns true on valid and updated
    /// returns false otherwise
    virtual bool
    CheckRenegotiateValid(RouterContact newRc, RouterContact oldRC) = 0;

    /// set router's service node whitelist
    virtual void
    SetRouterWhitelist(const std::vector< RouterID > &routers) = 0;

    /// visit each connected link session
    virtual void
    ForEachPeer(std::function< void(const ILinkSession *, bool) > visit,
                bool randomize) const = 0;

    virtual bool
    ConnectionToRouterAllowed(const RouterID &router) const = 0;

    /// return true if we have at least 1 session to this router in either
    /// direction
    virtual bool
    HasSessionTo(const RouterID &router) const = 0;

    virtual util::StatusObject
    ExtractStatus() const = 0;

    void
    EnsureRouter(RouterID router, RouterLookupHandler handler);
  };
}  // namespace llarp

#endif
