/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MAIN_IROHAD_BUILDER
#define MAIN_IROHAD_BUILDER

#include "ametsuchi/impl/storage_impl.hpp"
#include "consensus/consensus_block_cache.hpp"
#include "cryptography/crypto_provider/crypto_model_signer.hpp"
#include "cryptography/keypair.hpp"
#include "interfaces/common_objects/common_objects_factory.hpp"
#include "interfaces/iroha_internal/transaction_batch_factory.hpp"
#include "logger/logger.hpp"
#include "main/application.hpp"
#include "main/impl/block_loader_init.hpp"
#include "main/impl/consensus_init.hpp"
#include "main/impl/ordering_init.hpp"
#include "main/server_runner.hpp"
#include "mst.grpc.pb.h"
#include "multi_sig_transactions/mst_processor.hpp"
#include "multi_sig_transactions/transport/mst_transport_grpc.hpp"
#include "network/block_loader.hpp"
#include "network/consensus_gate.hpp"
#include "network/impl/peer_communication_service_impl.hpp"
#include "network/ordering_gate.hpp"
#include "network/peer_communication_service.hpp"
#include "pending_txs_storage/impl/pending_txs_storage_impl.hpp"
#include "simulator/block_creator.hpp"
#include "simulator/impl/simulator.hpp"
#include "synchronizer/impl/synchronizer_impl.hpp"
#include "synchronizer/synchronizer.hpp"
#include "torii/command_service.hpp"
#include "torii/impl/command_service_transport_grpc.hpp"
#include "torii/processor/query_processor_impl.hpp"
#include "torii/processor/transaction_processor_impl.hpp"
#include "torii/query_service.hpp"
#include "validation/chain_validator.hpp"
#include "validation/impl/chain_validator_impl.hpp"
#include "validation/impl/stateful_validator_impl.hpp"
#include "validation/stateful_validator.hpp"

// cpp
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

namespace iroha {
  class IrohadBuilder : public Irohad {
   public:
    IrohadBuilder(const std::string &block_store_dir,
                  const std::string &pg_conn,
                  size_t torii_port,
                  size_t internal_port,
                  size_t max_proposal_size,
                  std::chrono::milliseconds proposal_delay,
                  std::chrono::milliseconds vote_delay,
                  const shared_model::crypto::Keypair &keypair,
                  bool is_mst_supported)
        : Irohad(block_store_dir,
                 pg_conn,
                 torii_port,
                 internal_port,
                 max_proposal_size,
                 proposal_delay,
                 vote_delay,
                 keypair,
                 is_mst_supported) {}

    IrohadBuilder(Irohad &&irohad) : Irohad(std::move(irohad)) {}

    // clang-format off
    void setCryptoSigner(std::shared_ptr<shared_model::crypto::CryptoModelSigner<>> crypto_signer_) {this->crypto_signer_ = crypto_signer_;}
    void setBatchParser(std::shared_ptr<shared_model::interface::TransactionBatchParser> batch_parser) {this->batch_parser = batch_parser;}
    void setStorage(std::shared_ptr<iroha::ametsuchi::Storage> storage) {this->storage = storage;}
    void setStatefulValidator(std::shared_ptr<iroha::validation::StatefulValidator> stateful_validator) {this->stateful_validator = stateful_validator;}
    void setChainValidator(std::shared_ptr<iroha::validation::ChainValidator> chain_validator) {this->chain_validator = chain_validator;}
    void setWsvRestorer(std::shared_ptr<iroha::ametsuchi::WsvRestorer> wsv_restorer_) {this->wsv_restorer_ = wsv_restorer_;}
    void setAsyncCall(std::shared_ptr<iroha::network::AsyncGrpcClient<google::protobuf::Empty>> async_call_) {this->async_call_ = async_call_;}
    void setCommonObjectsFactory(std::shared_ptr<shared_model::interface::CommonObjectsFactory> common_objects_factory_) {this->common_objects_factory_ = common_objects_factory_;}
    void setTransactionBatchFactory(std::shared_ptr<shared_model::interface::TransactionBatchFactory> transaction_batch_factory_) {this->transaction_batch_factory_ = transaction_batch_factory_;}
    void setOrderingGate(std::shared_ptr<iroha::network::OrderingGate> ordering_gate) {this->ordering_gate = ordering_gate;}
    void setSimulator(std::shared_ptr<iroha::simulator::Simulator> simulator) {this->simulator = simulator;}
    void setConsensusResultCache(std::shared_ptr<iroha::consensus::ConsensusResultCache> consensus_result_cache_) {this->consensus_result_cache_ = consensus_result_cache_;}
    void setBlockLoader(std::shared_ptr<iroha::network::BlockLoader> block_loader) {this->block_loader = block_loader;}
    void setConsensusGate(std::shared_ptr<iroha::network::ConsensusGate> consensus_gate) {this->consensus_gate = consensus_gate;}
    void setSynchronizer(std::shared_ptr<iroha::synchronizer::Synchronizer> synchronizer) {this->synchronizer = synchronizer;}
    void setPcs(std::shared_ptr<iroha::network::PeerCommunicationService> pcs) {this->pcs = pcs;}
    void setTransactionFactory(std::shared_ptr<shared_model::interface::AbstractTransportFactory<shared_model::interface::Transaction, iroha::protocol::Transaction>> transaction_factory) {this->transaction_factory = transaction_factory;}
    void setMstTransport(std::shared_ptr<iroha::network::MstTransportGrpc> mst_transport) {this->mst_transport = mst_transport;}
    void setMstProcessor(std::shared_ptr<iroha::MstProcessor> mst_processor) {this->mst_processor = mst_processor;}
    void setPendingTxsStorage(std::shared_ptr<iroha::PendingTransactionStorage> pending_txs_storage_) {this->pending_txs_storage_ = pending_txs_storage_;}
    void setStatusBus(std::shared_ptr<iroha::torii::StatusBus> status_bus_) {this->status_bus_ = status_bus_;}
    void setCommandService(std::shared_ptr<::torii::CommandService> command_service) {this->command_service = command_service;}
    void setCommandServiceTransport(std::shared_ptr<::torii::CommandServiceTransportGrpc> command_service_transport) {this->command_service_transport = command_service_transport;}
    void setQueryService(std::shared_ptr<::torii::QueryService> query_service) {this->query_service = query_service;}
    // clang-format on

    bool initStorage() {
      if (common_objects_factory_ == nullptr) {
        common_objects_factory_ =
            std::make_shared<shared_model::proto::ProtoCommonObjectsFactory<
                shared_model::validation::FieldValidator>>();
      }
      auto block_converter =
          std::make_shared<shared_model::proto::ProtoBlockJsonConverter>();
      auto storageResult =
          ametsuchi::StorageImpl::create(block_store_dir_,
                                         pg_conn_,
                                         common_objects_factory_,
                                         std::move(block_converter));
      storageResult.match(
          [&](expected::Value<std::shared_ptr<ametsuchi::StorageImpl>>
                  &_storage) { storage = _storage.value; },
          [&](expected::Error<std::string> &error) {
            log_->error(error.error);
          });
      return storage != nullptr;
    }

    Irohad build() {
      auto log_ = logger::log("Irohad Builder");
      if (storage == nullptr) {
        log_->warn("Storage must be initialized in advance, skipping building");
        return std::move(*this);
      }

      if (crypto_signer_ == nullptr) {
        crypto_signer_ =
            std::make_shared<shared_model::crypto::CryptoModelSigner<>>(
                keypair);
      }
      if (batch_parser == nullptr) {
        batch_parser = std::make_shared<
            shared_model::interface::TransactionBatchParserImpl>();
      }
      if (stateful_validator == nullptr) {
        auto factory =
            std::make_unique<shared_model::proto::ProtoProposalFactory<
                shared_model::validation::DefaultProposalValidator>>();
        stateful_validator =
            std::make_shared<validation::StatefulValidatorImpl>(
                std::move(factory), batch_parser);
      }
      if (chain_validator == nullptr) {
        chain_validator = std::make_shared<validation::ChainValidatorImpl>(
            std::make_shared<consensus::yac::SupermajorityCheckerImpl>());
      }
      if (wsv_restorer_ == nullptr) {
        wsv_restorer_ = std::make_shared<iroha::ametsuchi::WsvRestorerImpl>();
      }
      if (async_call_ == nullptr) {
        async_call_ = std::make_shared<
            network::AsyncGrpcClient<google::protobuf::Empty>>();
      }
      if (transaction_batch_factory_ == nullptr) {
        transaction_batch_factory_ = std::make_shared<
            shared_model::interface::TransactionBatchFactoryImpl>();
      }
      if (common_objects_factory_ == nullptr) {
        common_objects_factory_ =
            std::make_shared<shared_model::proto::ProtoCommonObjectsFactory<
                shared_model::validation::FieldValidator>>();
      }
      if (ordering_gate == nullptr) {
        ordering_gate =
            ordering_init.initOrderingGate(storage,
                                           max_proposal_size_,
                                           proposal_delay_,
                                           storage,
                                           storage,
                                           transaction_batch_factory_,
                                           async_call_);
      }
      if (simulator == nullptr) {
        auto block_factory =
            std::make_unique<shared_model::proto::ProtoBlockFactory>(
                //  Block factory in simulator uses UnsignedBlockValidator
                //  because it is not required to check signatures of block
                //  here, as they will be checked when supermajority of peers
                //  will sign the block. It is also not required to validate
                //  signatures of transactions here because they are validated
                //  in the ordering gate, where they are received from the
                //  ordering service.
                std::make_unique<
                    shared_model::validation::DefaultUnsignedBlockValidator>());
        simulator =
            std::make_shared<simulator::Simulator>(ordering_gate,
                                                   stateful_validator,
                                                   storage,
                                                   storage,
                                                   crypto_signer_,
                                                   std::move(block_factory));
      }
      if (consensus_result_cache_ == nullptr) {
        consensus_result_cache_ =
            std::make_shared<consensus::ConsensusResultCache>();
      }
      if (block_loader == nullptr) {
        block_loader = loader_init.initBlockLoader(
            storage, storage, consensus_result_cache_);
      }
      if (consensus_gate == nullptr) {
        consensus_gate = yac_init.initConsensusGate(storage,
                                                    simulator,
                                                    block_loader,
                                                    keypair,
                                                    consensus_result_cache_,
                                                    vote_delay_,
                                                    async_call_,
                                                    common_objects_factory_);
      }
      if (synchronizer == nullptr) {
        synchronizer = std::make_shared<synchronizer::SynchronizerImpl>(
            consensus_gate, chain_validator, storage, block_loader);
      }
      if (pcs == nullptr) {
        pcs = std::make_shared<network::PeerCommunicationServiceImpl>(
            ordering_gate, synchronizer, simulator);
        ordering_gate->setPcs(*pcs);
      }
      if (transaction_factory == nullptr) {
        std::unique_ptr<shared_model::validation::AbstractValidator<
            shared_model::interface::Transaction>>
            transaction_validator = std::make_unique<
                shared_model::validation::
                    DefaultOptionalSignedTransactionValidator>();
        transaction_factory =
            std::make_shared<shared_model::proto::ProtoTransportFactory<
                shared_model::interface::Transaction,
                shared_model::proto::Transaction>>(
                std::move(transaction_validator));
      }
      if (mst_transport == nullptr) {
        mst_transport = std::make_shared<iroha::network::MstTransportGrpc>(
            async_call_,
            common_objects_factory_,
            transaction_factory,
            batch_parser,
            transaction_batch_factory_);
      }
      if (mst_processor == nullptr) {
        auto mst_completer = std::make_shared<DefaultCompleter>();
        auto mst_storage = std::make_shared<MstStorageStateImpl>(mst_completer);
        // TODO: IR-1317 @l4l (02/05/18) magics should be replaced with options
        // via cli parameters
        std::shared_ptr<iroha::PropagationStrategy> mst_propagation;
        if (is_mst_supported_) {
          mst_propagation = std::make_shared<GossipPropagationStrategy>(
              storage,
              std::chrono::seconds(5) /*emitting period*/,
              2 /*amount per once*/);
        } else {
          mst_propagation = std::make_shared<iroha::PropagationStrategyStub>();
        }

        auto mst_time = std::make_shared<MstTimeProviderImpl>();
        auto fair_mst_processor = std::make_shared<FairMstProcessor>(
            mst_transport, mst_storage, mst_propagation, mst_time);
        mst_processor = fair_mst_processor;
        mst_transport->subscribe(fair_mst_processor);
      }
      if (pending_txs_storage_ == nullptr) {
        pending_txs_storage_ = std::make_shared<PendingTransactionStorageImpl>(
            mst_processor->onStateUpdate(),
            mst_processor->onPreparedBatches(),
            mst_processor->onExpiredBatches());
      }
      if (status_bus_ == nullptr) {
        status_bus_ = std::make_shared<torii::StatusBusImpl>();
      }

      auto status_factory =
          std::make_shared<shared_model::proto::ProtoTxStatusFactory>();
      if (command_service_transport == nullptr) {
        command_service_transport =
            std::make_shared<::torii::CommandServiceTransportGrpc>(
                command_service,
                status_bus_,
                std::chrono::seconds(1),
                2 * proposal_delay_,
                status_factory,
                transaction_factory,
                batch_parser,
                transaction_batch_factory_);
      }
      if (command_service == nullptr) {
        auto tx_processor = std::make_shared<torii::TransactionProcessorImpl>(
            pcs, mst_processor, status_bus_);
        command_service = std::make_shared<::torii::CommandServiceImpl>(
            tx_processor, storage, status_bus_, status_factory);
      }
      if (query_service == nullptr) {
        auto query_processor = std::make_shared<torii::QueryProcessorImpl>(
            storage, storage, pending_txs_storage_);

        query_service =
            std::make_shared<::torii::QueryService>(query_processor);
      }

      log_->info("build completed");

      return std::move(*this);
    }
  };
}  // namespace iroha

#endif  // MAIN_IROHAD_BUILDER
