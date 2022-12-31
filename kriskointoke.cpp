#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;
CONTRACT kriskointoke : public contract {
     using contract::contract;
    public:
        TABLE balances {
            name acct;
            std::string sym;

            int64_t funds = 0;;
            auto primary_key() const { return acct.value; }
        };

        typedef eosio::multi_index< "balance"_n, balances >  balances_table; 
        [[eosio::on_notify("eosio.token::transfer")]]
        void deposit(name from,
                     name to, 
                     eosio::asset quantity, 
                     std::string memo){
            
            eosio::symbol token_symbol("MYCASH", 0);
            check(to != get_self(),"No transfer yourself");
            check (quantity.amount > 0,"no negative values");
            check(quantity.symbol != token_symbol, "Illegal asset symbol");

            balances_table balanceTbl(get_self(), from.value);
            auto it = balanceTbl.find(to.value);

            if (it != balanceTbl.end()){
                balanceTbl.modify(it, get_self(), [&](auto &row) {
                row.funds += quantity.amount;
                });
            }else{
                balanceTbl.emplace(get_self(), [&](auto &row) {
                row.acct = to;
                row.funds = quantity.amount;
                row.sym = token_symbol.code().to_string();
                });
            }
            
        }

};

