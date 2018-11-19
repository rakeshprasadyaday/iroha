/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "framework/integration_framework/fake_peer/behaviour/honest.hpp"

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "backend/protobuf/transaction.hpp"
#include "framework/integration_framework/fake_peer/block_storage.hpp"
#include "framework/integration_framework/fake_peer/network/batches_for_round.hpp"
#include "framework/integration_framework/fake_peer/proposal_storage.hpp"
#include "interfaces/iroha_internal/transaction_batch.hpp"
#include "module/shared_model/builders/protobuf/proposal.hpp"

namespace integration_framework {
  namespace fake_peer {

    void HonestBehaviour::processYacMessage(const YacMessagePtr &message) {
      getFakePeer().voteForTheSame(message);
    }

    std::string HonestBehaviour::getName() {
      return "honest behaviour";
    }

    LoaderBlockRequestResult HonestBehaviour::processLoaderBlockRequest(
        const LoaderBlockRequest &request) {
      const auto block_storage = getFakePeer().getBlockStorage();
      if (!block_storage) {
        getLogger()->debug(
            "Got a Loader.retrieveBlock call, but have no block storage!");
        return {};
      }
      const auto block = block_storage->getBlockByHash(*request);
      if (!block) {
        getLogger()->debug(
            "Got a Loader.retrieveBlock call for {}, but have no such block!",
            request->toString());
        return {};
      }
      return *std::static_pointer_cast<shared_model::proto::Block>(block);
    }

    LoaderBlocksRequestResult HonestBehaviour::processLoaderBlocksRequest(
        const LoaderBlocksRequest &request) {
      const auto block_storage = getFakePeer().getBlockStorage();
      if (!block_storage) {
        getLogger()->debug(
            "Got a Loader.retrieveBlocks call, but have no block storage!");
        return {};
      }
      BlockStorage::HeightType current_height = request;
      BlockStorage::BlockPtr block;
      LoaderBlocksRequestResult blocks;
      while ((block = block_storage->getBlockByHeight(current_height++))
             != nullptr) {
        blocks.emplace_back(*block);
      }
      return blocks;
    }

    OrderingProposalRequestResult
    HonestBehaviour::processOrderingProposalRequest(
        const OrderingProposalRequest &request) {
      const auto proposal_storage = getFakePeer().getProposalStorage();
      if (!proposal_storage) {
        getLogger()->debug(
            "Got an OnDemandOrderingService.GetProposal call for round {}, "
            "but have no proposal storage! NOT returning a proposal.",
            request.toString());
        return boost::none;
      }
      return proposal_storage->getProposal(request);
    }

    void HonestBehaviour::processOrderingBatches(
        const BatchesForRound &batches_for_round) {
      auto &fake_peer = getFakePeer();
      auto proposal_storage = fake_peer.getProposalStorage();
      if (!proposal_storage) {
        getLogger()->debug(
            "Got an OnDemandOrderingService.SendBatches call, but have no "
            "proposal storage to store the incoming batches! Creating one.");
        fake_peer.setProposalStorage(std::make_shared<ProposalStorage>());
        proposal_storage = fake_peer.getProposalStorage();
        BOOST_ASSERT_MSG(proposal_storage,
                         "Failed to create a proposal storage!");
      }
      const auto &round = batches_for_round.round;
      const auto &batches = batches_for_round.batches;
      getLogger()->debug(
          "Got an OnDemandOrderingService.SendBatches call, storing the "
          "following batches for round {}: {}",
          round.toString(),
          boost::algorithm::join(
              batches | boost::adaptors::transformed([](const auto &batch) {
                return batch->toString();
              }),
              ",\n"));
      std::vector<shared_model::proto::Transaction> txs;
      auto opt_proposal = proposal_storage->getProposal(round);
      if (opt_proposal) {
        for (const auto &tx : opt_proposal->getTransport().transactions()) {
          txs.emplace_back(tx);
        }
      }
      for (const auto &batch : batches) {
        for (const auto &tx : batch->transactions()) {
          txs.emplace_back(
              std::static_pointer_cast<shared_model::proto::Transaction>(tx)
                  ->getTransport());
        }
      }
      auto new_proposal = shared_model::proto::ProposalBuilder()
                              .height(round.block_round)
                              .createdTime(iroha::time::now())
                              .transactions(txs)
                              .build()
                              .getTransport();
      proposal_storage->storeProposal(
          round,
          std::make_shared<shared_model::proto::Proposal>(
              std::move(new_proposal)));
    }

  }  // namespace fake_peer
}  // namespace integration_framework
