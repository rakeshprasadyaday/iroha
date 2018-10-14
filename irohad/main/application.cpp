/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "main/application.hpp"

#include "ametsuchi/impl/postgres_ordering_service_persistent_state.hpp"
#include "ametsuchi/impl/wsv_restorer_impl.hpp"
#include "backend/protobuf/common_objects/proto_common_objects_factory.hpp"
#include "backend/protobuf/proto_block_json_converter.hpp"
#include "backend/protobuf/proto_proposal_factory.hpp"
#include "backend/protobuf/proto_transport_factory.hpp"
#include "backend/protobuf/proto_tx_status_factory.hpp"
#include "consensus/yac/impl/supermajority_checker_impl.hpp"
#include "interfaces/iroha_internal/transaction_batch_factory_impl.hpp"
#include "interfaces/iroha_internal/transaction_batch_parser_impl.hpp"
#include "multi_sig_transactions/gossip_propagation_strategy.hpp"
#include "multi_sig_transactions/mst_processor_impl.hpp"
#include "multi_sig_transactions/mst_propagation_strategy_stub.hpp"
#include "multi_sig_transactions/mst_time_provider_impl.hpp"
#include "multi_sig_transactions/storage/mst_storage_impl.hpp"
#include "torii/impl/command_service_impl.hpp"
#include "torii/impl/status_bus_impl.hpp"
#include "validators/field_validator.hpp"

using namespace iroha;
using namespace iroha::ametsuchi;
using namespace iroha::simulator;
using namespace iroha::validation;
using namespace iroha::network;
using namespace iroha::synchronizer;
using namespace iroha::torii;
using namespace iroha::consensus::yac;

using namespace std::chrono_literals;

/**
 * Configuring iroha daemon
 */
Irohad::Irohad(const std::string &block_store_dir,
               const std::string &pg_conn,
               size_t torii_port,
               size_t internal_port,
               size_t max_proposal_size,
               std::chrono::milliseconds proposal_delay,
               std::chrono::milliseconds vote_delay,
               const shared_model::crypto::Keypair &keypair,
               bool is_mst_supported)
    : block_store_dir_(block_store_dir),
      pg_conn_(pg_conn),
      torii_port_(torii_port),
      internal_port_(internal_port),
      max_proposal_size_(max_proposal_size),
      proposal_delay_(proposal_delay),
      vote_delay_(vote_delay),
      is_mst_supported_(is_mst_supported),
      keypair(keypair) {
  log_ = logger::log("IROHAD");
  log_->info("created");
}

Irohad::Irohad(Irohad &&irohad)
    : block_store_dir_(std::move(irohad.block_store_dir_)),
      pg_conn_(std::move(irohad.pg_conn_)),
      torii_port_(std::move(irohad.torii_port_)),
      internal_port_(std::move(irohad.internal_port_)),
      max_proposal_size_(std::move(irohad.max_proposal_size_)),
      proposal_delay_(std::move(irohad.proposal_delay_)),
      vote_delay_(std::move(irohad.vote_delay_)),
      is_mst_supported_(std::move(irohad.is_mst_supported_)),
      crypto_signer_(std::move(irohad.crypto_signer_)),
      batch_parser(std::move(irohad.batch_parser)),
      stateful_validator(std::move(irohad.stateful_validator)),
      chain_validator(std::move(irohad.chain_validator)),
      wsv_restorer_(std::move(irohad.wsv_restorer_)),
      async_call_(std::move(irohad.async_call_)),
      common_objects_factory_(std::move(irohad.common_objects_factory_)),
      transaction_batch_factory_(std::move(irohad.transaction_batch_factory_)),
      ordering_gate(std::move(irohad.ordering_gate)),
      simulator(std::move(irohad.simulator)),
      consensus_result_cache_(std::move(irohad.consensus_result_cache_)),
      block_loader(std::move(irohad.block_loader)),
      consensus_gate(std::move(irohad.consensus_gate)),
      synchronizer(std::move(irohad.synchronizer)),
      pcs(std::move(irohad.pcs)),
      transaction_factory(std::move(irohad.transaction_factory)),
      mst_processor(std::move(irohad.mst_processor)),
      pending_txs_storage_(std::move(irohad.pending_txs_storage_)),
      status_bus_(std::move(irohad.status_bus_)),
      command_service(std::move(irohad.command_service)),
      command_service_transport(std::move(irohad.command_service_transport)),
      query_service(std::move(irohad.query_service)),
      ordering_init(std::move(irohad.ordering_init)),
      yac_init(std::move(irohad.yac_init)),
      loader_init(std::move(irohad.loader_init)),
      mst_transport(std::move(irohad.mst_transport)),
      log_(logger::log("IROHAD")),
      storage(std::move(irohad.storage)),
      keypair(std::move(irohad.keypair)) {
  log_->info("move ctor");
}

/**
 * Initializing iroha daemon
 */
void Irohad::init() {
  pcs->on_proposal().subscribe(
      [this](auto) { log_->info("~~~~~~~~~| PROPOSAL ^_^ |~~~~~~~~~ "); });

  pcs->on_commit().subscribe(
      [this](auto) { log_->info("~~~~~~~~~| COMMIT =^._.^= |~~~~~~~~~ "); });
}

/**
 * Dropping iroha daemon storage
 */
void Irohad::dropStorage() {
  storage->reset();
  storage->createOsPersistentState() |
      [](const auto &state) { state->resetState(); };
}

void Irohad::resetOrderingService() {
  if (not(storage->createOsPersistentState() |
          [](const auto &state) { return state->resetState(); }))
    log_->error("cannot reset ordering service storage");
}

bool Irohad::restoreWsv() {
  return wsv_restorer_->restoreWsv(*storage).match(
      [](iroha::expected::Value<void> v) { return true; },
      [&](iroha::expected::Error<std::string> &error) {
        log_->error(error.error);
        return false;
      });
}

/**
 * Run iroha daemon
 */
void Irohad::run() {
  using iroha::expected::operator|;

  // Initializing torii server
  std::string ip = "0.0.0.0";
  torii_server =
      std::make_unique<ServerRunner>(ip + ":" + std::to_string(torii_port_));

  // Initializing internal server
  internal_server =
      std::make_unique<ServerRunner>(ip + ":" + std::to_string(internal_port_));

  // Run torii server
  (torii_server->append(command_service_transport).append(query_service).run() |
   [&](const auto &port) {
     log_->info("Torii server bound on port {}", port);
     if (is_mst_supported_) {
       internal_server->append(mst_transport);
     }
     // Run internal server
     return internal_server->append(ordering_init.ordering_gate_transport)
         .append(ordering_init.ordering_service_transport)
         .append(yac_init.consensus_network)
         .append(loader_init.service)
         .run();
   })
      .match(
          [&](const auto &port) {
            log_->info("Internal server bound on port {}", port.value);
            log_->info("===> iroha initialized");
          },
          [&](const expected::Error<std::string> &e) { log_->error(e.error); });
}

Irohad::~Irohad() {
  // TODO andrei 17.09.18: IR-1710 Verify that all components' destructors are
  // called in irohad destructor
  storage->freeConnections();
}
