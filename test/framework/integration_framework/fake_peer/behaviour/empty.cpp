#include "framework/integration_framework/fake_peer/behaviour/empty.hpp"

namespace integration_framework {
  namespace fake_peer {

    void EmptyBehaviour::processMstMessage(const MstMessagePtr &message) {}
    void EmptyBehaviour::processYacMessage(const YacMessagePtr &message) {}
    void EmptyBehaviour::processOsBatch(const OsBatchPtr &batch) {}
    void EmptyBehaviour::processOgProposal(const OgProposalPtr &proposal) {}
    LoaderBlockRequestResult EmptyBehaviour::processLoaderBlockRequest(
        const LoaderBlockRequest &request) {
      return {};
    }
    LoaderBlocksRequestResult EmptyBehaviour::processLoaderBlocksRequest(
        const LoaderBlocksRequest &request) {
      return {};
    }
    OrderingProposalRequestResult
    EmptyBehaviour::processOrderingProposalRequest(
        const OrderingProposalRequest &request) {
      return {};
    }
    void EmptyBehaviour::processOrderingBatches(
        const BatchesForRound &batches_for_round) {}

    std::string EmptyBehaviour::getName() {
      return "empty behaviour";
    }

  }  // namespace fake_peer
}  // namespace integration_framework
