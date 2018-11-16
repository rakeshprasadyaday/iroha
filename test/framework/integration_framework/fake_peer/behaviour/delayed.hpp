/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INTEGRATION_FRAMEWORK_FAKE_PEER_BEHAVIOUR_DELAYED_HPP_
#define INTEGRATION_FRAMEWORK_FAKE_PEER_BEHAVIOUR_DELAYED_HPP_

#include <chrono>

#include "framework/integration_framework/fake_peer/behaviour/decorator.hpp"

namespace integration_framework {
  namespace fake_peer {

    class DelayedBehaviour : public BehaviourDecorator {
     public:
      DelayedBehaviour(Behaviour &base_behaviour,
                       std::chrono::milliseconds delay);

      ~DelayedBehaviour() = default;

      void processMstMessage(const MstMessagePtr &message) override;
      void processYacMessage(const YacMessagePtr &message) override;
      void processOsBatch(const OsBatchPtr &batch) override;
      void processOgProposal(const OgProposalPtr &proposal) override;
      LoaderBlockRequestResult processLoaderBlockRequest(
          const LoaderBlockRequest &request) override;
      LoaderBlocksRequestResult processLoaderBlocksRequest(
          const LoaderBlocksRequest &request) override;
      OrderingProposalRequestResult processOrderingProposalRequest(
          const OrderingProposalRequest &request) override;

      std::string getName() override;

     private:
      std::chrono::milliseconds delay_;
    };

  }  // namespace fake_peer
}  // namespace integration_framework

#endif /* INTEGRATION_FRAMEWORK_FAKE_PEER_BEHAVIOUR_DELAYED_HPP_ */
