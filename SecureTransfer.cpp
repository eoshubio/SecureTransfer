#include "SecureTransfer.hpp"

//Transfer hooking
void SecureTransfer::transfer(uint64_t sender, uint64_t receiver) {

  auto transfer_data = unpack_action_data<st_transfer>();

  //Checking the transaction whether occurred by a rightful account owner
  if (transfer_data.from.value != _self.value) {
      //Incoming transaction.
      return;
  }

  uint64_t amountValue = transfer_data.quantity.amount;
  auto limitToday = getLimitToday();
  //Checking withdrawal limitation of a transaction
  eosio_assert(limitToday.limitOnce >= amountValue, "You have exceeded the one-time withdrawal limit.");
  //Checking withdrawal limitation of a day
  eosio_assert(limitToday.limitDaily >= amountValue + limitToday.amountDaily, "You have exceeded the one day withdrawal limit.");

  addAmount(amountValue);
}

[[eosio::action]]
void SecureTransfer::updatelimit(uint64_t onetime, uint64_t daily) {
  //Checking permission of owner
  require_auth(permission_level{_self, "owner"_n});
  limit_index dailyLimit(_self, _self.value);
  auto itr = dailyLimit.begin();
  if (itr == dailyLimit.end()) {
    dailyLimit.emplace(_self, [&](auto& v){
      v.id = 0;
      v.date = 0;
      v.limitOnce = onetime;
      v.limitDaily = daily;
      v.amountDaily = 0;
    });
  } else {
    auto limitValue = dailyLimit.get(0);
    auto itr = dailyLimit.find(0);
    dailyLimit.modify(itr, _self, [&](auto& v){
      v.limitOnce = onetime;
      v.limitDaily = daily;
    });
  }
}

SecureTransfer::limit SecureTransfer::getLimitToday() {
  uint64_t date = getDate();
  limit_index dailyLimit(_self, _self.value);
  auto itr = dailyLimit.begin();
  if (itr == dailyLimit.end()) {
    dailyLimit.emplace(_self, [&](auto& v){
      v.id = 0;
      v.date = 0;
      v.limitOnce = LIMIT_ONCE;
      v.limitDaily = LIMIT_DAILY;
      v.amountDaily = 0;
    });
    return limit{ 0, date, LIMIT_ONCE, LIMIT_DAILY, 0 };
  } else {
    //Checking the date.
    auto limitValue = dailyLimit.get(0);
    if (limitValue.date < date) {
      //Update daily limit
      auto itr = dailyLimit.find(0);
      dailyLimit.modify(itr, _self, [&](auto& v){
        v.date = date;
        v.amountDaily = 0;
      });
    }
    print(limitValue.date);
    return limitValue;
  }
}

uint64_t SecureTransfer::getDate() {
  return now()/86400;
}

void SecureTransfer::addAmount(uint64_t amount) {
  limit_index dailyLimit(_self, _self.value);
  auto limitValue = dailyLimit.get(0);
  //Set daily limit
  auto itr = dailyLimit.find(0);
  dailyLimit.modify(itr, _self, [&](auto& v){
    v.amountDaily = limitValue.amountDaily + amount;
  });

}

//MARK: apply
#define EOSIO_DISPATCH_EX( TYPE, MEMBERS ) \
extern "C" { \
  void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
    if( code == receiver ) { \
      if (action != name("transfer").value) { \
        switch( action ) { \
          EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
        } \
        /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
      } \
    } \
    else if (code == name("eosio.token").value && action == name("transfer").value ) { \
      execute_action(name(receiver), name(code), &SecureTransfer::transfer ); \
    } \
  } \
} \

EOSIO_DISPATCH_EX( SecureTransfer, (transfer)(updatelimit))
