#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/print.hpp>

using namespace eosio;
using namespace std;

/**
 * SecureTransfer
 *
 * - A smartcontract which allows user can control token transfer limitation(Token transfer limitation for each transaction or day) 
 *
 * 'updatelimit' method controls a limit amount of token transfer per day(add additional owner permission for better security)
 * > The default setting value is 10 EOS per transaction / 100 EOS per day
 *
 * - EOSHub suggests the following security policies to protect user's EOS account:
 * > 1. Owner key and Active key should be different.
 * > 2. Onwer key should be stored in Apple secure enclave or Hardware wallet. 
 * > 3. Most resources need to be staking to prevent illegal token transfer.
 * > 4. Please visit EOSHub security web page to get more information.
 */
 
class [[eosio::contract]] SecureTransfer: public contract {

public:
  using contract::contract;
  SecureTransfer(name receiver, name code,  datastream<const char*> ds):contract(receiver, code, ds) {}

private:
  const symbol CORE_SYMBOL = symbol(symbol_code("EOS"),4);
  const uint64_t LIMIT_ONCE = 100000; // 10.0000 EOS
  const uint64_t LIMIT_DAILY = 1000000; // 100.0000 EOS

  struct limit {
    uint64_t    id;
    uint64_t		date;
    uint64_t    limitOnce;
    uint64_t    limitDaily;
    uint64_t    amountDaily;
    uint64_t		primary_key() const { return id; }
  };

  typedef eosio::multi_index< name("limit"), limit> limit_index;


  struct account {
    asset    balance;
    uint64_t primary_key() const { return balance.symbol.code().raw(); }
  };

  struct st_transfer {
    name  from;
    name  to;
    asset  quantity;
    string   memo;

    EOSLIB_SERIALIZE( st_transfer, (from)(to)(quantity)(memo) )
  };

public:
  //Transfer hooking
  void transfer(uint64_t sender, uint64_t receiver);

  //update limitation amount of tokens for each transaction/day 
  [[eosio::action]]
  void updatelimit(uint64_t onetime, uint64_t daily);

private:
  limit getLimitToday();
  uint64_t getDate();
  void addAmount(uint64_t amount);
};
