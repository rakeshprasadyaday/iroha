/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <iomanip>
#include <sstream>
#include <type_traits>

#include <gtest/gtest.h>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/size.hpp>
#include "datetime/time.hpp"
#include "framework/integration_framework/integration_test_framework.hpp"
#include "integration/acceptance/acceptance_fixture.hpp"
#include "interfaces/common_objects/types.hpp"
#include "interfaces/permissions.hpp"
#include "module/shared_model/builders/protobuf/test_query_builder.hpp"
#include "module/shared_model/builders/protobuf/test_transaction_builder.hpp"

using namespace integration_framework;
using namespace shared_model;
using namespace common_constants;
using interface::permissions::Role;

static constexpr interface::types::TransactionsNumberType kTxsNumber(20);
static constexpr interface::types::PrecisionType kAssetPrecision(2);

template <typename T>
static std::string assetAmount(T mantissa,
                               interface::types::PrecisionType precision) {
  std::stringstream ss;
  ss << std::setprecision(precision) << mantissa;
  return ss.str();
}

static const interface::TransactionsPageResponse &getTransactionsPageResponse(
    const shared_model::proto::QueryResponse &query_response) {
  EXPECT_EQ(query_response.get().type(),
            boost::typeindex::type_id_with_cvr<
                const interface::TransactionsPageResponse &>())
      << "Wrong query response!";
  return boost::get<const interface::TransactionsPageResponse &>(
      query_response.get());
}

static auto getTransactionsPageResponseGetter(
    std::unique_ptr<proto::TransactionsPageResponse> &destination) {
  return [&destination](auto query_response) {
    destination = std::make_unique<proto::TransactionsPageResponse>(
        static_cast<const proto::TransactionsPageResponse &>(
            getTransactionsPageResponse(query_response)));
  };
}

template <typename QueryTxPaginationTest>
class QueryTxPaginationCommonFixture : public AcceptanceFixture {
 protected:
  using Impl = QueryTxPaginationTest;

  void SetUp() override {
    itf_ = std::make_unique<IntegrationTestFramework>(1);
    itf_->setInitialState(common_constants::kAdminKeypair)
        .sendTx(makeUserWithPerms(Impl::getUserPermissions()))
        .sendTx(complete(
            baseTx(kAdminId)
                .createAsset(kAssetName, kDomain, kAssetPrecision)
                .addAssetQuantity(kAssetId,
                                  assetAmount(kTxsNumber, kAssetPrecision)),
            kAdminKeypair));

    const auto initial_txs = Impl::makeInitialTransactions();
    tx_hashes_.reserve(initial_txs.size());
    for (auto &tx : initial_txs) {
      tx_hashes_.emplace_back(tx.hash());
      itf_->sendTx(tx);
    }
  }

  std::unique_ptr<proto::TransactionsPageResponse> queryPage(
      interface::types::TransactionsNumberType page_size,
      const boost::optional<interface::types::HashType> &first_hash =
          boost::none) const {
    std::unique_ptr<proto::TransactionsPageResponse> tx_page_response;
    itf_->sendQuery(Impl::makeQuery(page_size, first_hash),
                    getTransactionsPageResponseGetter(tx_page_response));
    CheckTransactionsPageResponse(*tx_page_response, page_size, first_hash);
    return tx_page_response;
  }

  void CheckTransactionsPageResponse(
      const interface::TransactionsPageResponse &tx_page_response,
      interface::types::TransactionsNumberType page_size,
      const boost::optional<interface::types::HashType> &first_hash) const {
    EXPECT_EQ(tx_page_response.allTransactionsSize(), tx_hashes_.size())
        << "Wrong \"total transactions\" number.";
    auto resp_tx_hashes = tx_page_response.transactions()
        | boost::adaptors::transformed(
                              [](const auto &tx) { return tx.hash(); });
    const auto page_start = first_hash
        ? std::find(tx_hashes_.cbegin(), tx_hashes_.cend(), *first_hash)
        : tx_hashes_.cbegin();
    if (page_start == tx_hashes_.cend()) {
      // TODO handle the case of missing tx by requested first hash
      return;
    }
    const auto expected_txs_amount =
        std::min<size_t>(page_size, tx_hashes_.cend() - page_start);
    const auto response_txs_amount = boost::size(resp_tx_hashes);
    EXPECT_EQ(response_txs_amount, expected_txs_amount)
        << "Wrong number of transactions returned.";
    auto expected_hash = page_start;
    auto response_hash = resp_tx_hashes.begin();
    const auto page_end =
        page_start + std::min(response_txs_amount, expected_txs_amount);
    while (expected_hash != page_end) {
      EXPECT_EQ(*expected_hash++, *response_hash++)
          << "Wrong transaction returned.";
    }
    EXPECT_EQ(tx_page_response.nextTxHash() == boost::none,
              page_end == tx_hashes_.cend())
        << "Wrong next transaction hash value.";
  }

  std::unique_ptr<IntegrationTestFramework> itf_;
  std::vector<interface::types::HashType> tx_hashes_;
};

struct GetAccountTxPaginationFixture {
  static std::initializer_list<Role> getUserPermissions() {
    return {Role::kSetDetail, Role::kGetMyAccTxs};
  }

  static auto makeInitialTransactions() {
    std::vector<shared_model::proto::Transaction> transactions;
    transactions.reserve(kTxsNumber);
    for (auto i = decltype(kTxsNumber){0}; i < kTxsNumber; ++i) {
      transactions.emplace_back(
          TestUnsignedTransactionBuilder()
              .creatorAccountId(kAdminId)
              .createdTime(iroha::time::now(std::chrono::milliseconds(i)))
              .setAccountDetail(kUserId,
                                "key_" + std::to_string(i),
                                "val_" + std::to_string(i))
              .build()
              .signAndAddSignature(kUserKeypair)
              .finish());
    }
    return transactions;
  }

  static auto makeQuery(interface::types::TransactionsNumberType page_size,
                        const boost::optional<interface::types::HashType>
                            &first_hash = boost::none) {
    return TestUnsignedQueryBuilder()
        .creatorAccountId(kUserId)
        .createdTime(iroha::time::now())
        .getAccountTransactions(kUserId, page_size, first_hash)
        .build()
        .signAndAddSignature(kUserKeypair)
        .finish();
  }
};

struct GetAccountAssetTxPaginationFixture {
  static std::initializer_list<Role> getUserPermissions() {
    return {Role::kReceive, Role::kGetMyAccAstTxs};
  }

  static auto makeInitialTransactions() {
    std::vector<shared_model::proto::Transaction> transactions;
    transactions.reserve(kTxsNumber);
    for (auto i = decltype(kTxsNumber){0}; i < kTxsNumber; ++i) {
      transactions.emplace_back(
          TestUnsignedTransactionBuilder()
              .creatorAccountId(kAdminId)
              .createdTime(iroha::time::now(std::chrono::milliseconds(i)))
              .transferAsset(kAdminId,
                             kUserId,
                             kAssetId,
                             "tx #" + std::to_string(i),
                             assetAmount(i, kAssetPrecision))
              .build()
              .signAndAddSignature(kAdminKeypair)
              .finish());
    }
    return transactions;
  }

  static auto makeQuery(interface::types::TransactionsNumberType page_size,
                        const boost::optional<interface::types::HashType>
                            &first_hash = boost::none) {
    return TestUnsignedQueryBuilder()
        .creatorAccountId(kUserId)
        .createdTime(iroha::time::now())
        .getAccountAssetTransactions(kUserId, kAssetId, page_size, first_hash)
        .build()
        .signAndAddSignature(kUserKeypair)
        .finish();
  }
};

using QueryTxPaginationTestingTypes =
    ::testing::Types<GetAccountTxPaginationFixture,
                     GetAccountAssetTxPaginationFixture>;
TYPED_TEST_CASE(QueryTxPaginationCommonFixture, QueryTxPaginationTestingTypes);

/**
 * @given a query that matches more transactions than the page size
 * @when the first transaction hash is not set
 * @then the requested amount of first transactions is returned @and the next
 * transaction hash is set
 */
TYPED_TEST(QueryTxPaginationCommonFixture, FirstTxsWhenFirstHashNotSet) {
  this->queryPage(kTxsNumber / 2);
}

/**
 * @given a query that requests some transactions
 * @when more transactions per page are requested than there are available
 * @then the available amount of first transactions is returned @and the next
 * transaction hash is not set
 */
TYPED_TEST(QueryTxPaginationCommonFixture, PageSizeBiggerThanAllTxsAmount) {
  this->queryPage(kTxsNumber + 1);
}

/**
 * @given a query that requests some transactions
 * @when transactions are consequtively queried with page size less then total
 * available transactions amount, @and page size not being a divizor of total
 * transactions amount
 * @then all the transactions are returned, with correct last hash value
 */
TYPED_TEST(QueryTxPaginationCommonFixture, GetAllTxsWithSmallPageNotDivisor) {
  constexpr size_t page_size = 3;
  static_assert(kTxsNumber % page_size != 0,
                "Page size must not be divizor of total transactions amount in "
                "this test.");
  boost::optional<interface::types::HashType> next_hash;
  size_t received_txs_amount = 0;
  for (size_t page_start = 0; page_start < kTxsNumber;
       page_start += page_size) {
    const auto tx_page_response = this->queryPage(page_size, next_hash);
    next_hash = tx_page_response->nextTxHash();
    received_txs_amount += tx_page_response->transactions().size();
  }
  EXPECT_EQ(next_hash, boost::none) << "Next transaction hash must be not set "
                                       "when last transactions are returned";
  EXPECT_EQ(received_txs_amount, this->tx_hashes_.size())
      << "Wrong amount of all received transactions.";
}

/**
 * @given a query that requests some transactions
 * @when transactions are consequtively queried with page size less then total
 * available transactions amount, @and page size being a divizor of total
 * transactions amount
 * @then all the transactions are returned, with correct last hash value
 */
TYPED_TEST(QueryTxPaginationCommonFixture, GetAllTxsWithSmallPageDivisor) {
  constexpr size_t page_size = 4;
  static_assert(
      kTxsNumber % page_size == 0,
      "Page size must be divizor of total transactions amount in this test.");
  boost::optional<interface::types::HashType> next_hash;
  size_t received_txs_amount = 0;
  for (size_t page_start = 0; page_start < kTxsNumber;
       page_start += page_size) {
    const auto tx_page_response = this->queryPage(page_size, next_hash);
    next_hash = tx_page_response->nextTxHash();
    received_txs_amount += tx_page_response->transactions().size();
  }
  EXPECT_EQ(next_hash, boost::none) << "Next transaction hash must be not set "
                                       "when last transactions are returned";
  EXPECT_EQ(received_txs_amount, this->tx_hashes_.size())
      << "Wrong amount of all received transactions.";
}
