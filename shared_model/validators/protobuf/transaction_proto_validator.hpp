/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IROHA_TRANSACTION_PROTO_VALIDATOR_HPP
#define IROHA_TRANSACTION_PROTO_VALIDATOR_HPP

#include "transaction.pb.h"
#include "validators/abstract_validator.hpp"

namespace shared_model {
  namespace validation {

    class TransactionProtoValidator
        : public AbstractValidator<iroha::protocol::Transaction> {
     private:
      template <typename CommandType>
      void validateCommand(const CommandType& command, ReasonsGroupName& tx_reason){}

      template <>
      void validateCommand(const iroha::protocol::CreateRole&, ReasonsGroupName& tx_reason) {

      }

     public:
      Answer validate(const iroha::protocol::Transaction &transaction) const {
        Answer answer;
        std::string tx_reason_name = "TransactionProto";
        ReasonsGroupType tx_reason(tx_reason_name, GroupedReasons());

        for (const auto &command :
             transaction.payload().reduced_payload().commands()) {
          switch (command.command_case()) {
            case iroha::protocol::Command::kAddAssetQuantity:
              tx_reason.second.push_back()
              break;
            case iroha::protocol::Command::kAddPeer:break;
            case iroha::protocol::Command::kAddSignatory:break;
            case iroha::protocol::Command::kAppendRole:break;
            case iroha::protocol::Command::kCreateAccount:break;
            case iroha::protocol::Command::kCreateAsset:break;
            case iroha::protocol::Command::kCreateDomain:break;
            case iroha::protocol::Command::kCreateRole:break;
            case iroha::protocol::Command::kDetachRole:break;
            case iroha::protocol::Command::kGrantPermission:break;
            case iroha::protocol::Command::kRemoveSignatory:break;
            case iroha::protocol::Command::kRevokePermission:break;
            case iroha::protocol::Command::kSetAccountDetail:break;
            case iroha::protocol::Command::kSetAccountQuorum:break;
            case iroha::protocol::Command::kSubtractAssetQuantity:break;
            case iroha::protocol::Command::kTransferAsset:break;
            case iroha::protocol::Command::COMMAND_NOT_SET:break;
          }
        }
      }
    };

  }  // namespace validation
}  // namespace shared_model

#endif  // IROHA_TRANSACTION_PROTO_VALIDATOR_HPP
