#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

using namespace eosio;
const static uint32_t MINUTE = 60;

CONTRACT kriskointoke : public contract {

     TABLE tokesettings {
        std::string  token_symbol ;
        uint32_t withdraw_timespan ;//in minutes
    } default_settings;

    using singleton_settings = eosio::singleton<"tokesettings"_n,tokesettings> ;

    public:
        using contract::contract;

        kriskointoke(
            eosio::name receiver,
            eosio::name code,
            datastream < const char *> ds
        ):
        contract(receiver,code,ds),
        settings_instance(receiver,receiver.value)
        {}

        singleton_settings settings_instance;
        
        TABLE balances {
            name acct;
            std::string sym;
            int64_t funds = 0;
            uint32_t withdraw_due_time;
            auto primary_key() const { return acct.value; }
        };

        typedef eosio::multi_index< "balance"_n, balances >  balances_table; 
        [[eosio::on_notify("eosio.token::transfer")]]
        void deposit(name from,
                     name to, 
                     eosio::asset quantity, 
                     std::string memo){

            eosio::symbol token_symbol(get_token_symbol(),0);
            check(to != get_self(),"No transfer yourself");
            check (quantity.amount > 0,"no negative values");
            check(quantity.symbol.code().to_string() == token_symbol.code().to_string(), "Illegal asset symbol");

            balances_table balanceTbl(get_self(), get_self().value);
            auto it = balanceTbl.find(to.value);
            eosio::time_point_sec tps = eosio::current_time_point();
        
            if (it != balanceTbl.end()){
                balanceTbl.modify(it, get_self(), [&](auto &row) {
                row.funds += quantity.amount;
                row.withdraw_due_time  = tps.sec_since_epoch() + (get_timespan() *  MINUTE) ;
                });
            }else{
                balanceTbl.emplace(get_self(), [&](auto &row) {
                row.acct = to;
                row.funds = quantity.amount;
                row.sym = token_symbol.code().to_string();
                row.withdraw_due_time  = tps.sec_since_epoch() + (get_timespan() * MINUTE );
                });
            }
            
        }

        

        ACTION withdraw(name from,
                     name to, 
                     eosio::asset quantity, 
                     std::string memo){

            eosio::symbol token_symbol(get_token_symbol(),0);
            check(quantity.symbol.code().to_string() == token_symbol.code().to_string(), "Illegal asset symbol");

            balances_table balanceTbl(get_self(), get_self().value);
            auto it = balanceTbl.find(to.value);
            //check (it != balanceTbl.end(),"Couldn't find balance for this account");
            
            eosio::time_point_sec tps = eosio::current_time_point();
            check (  it->withdraw_due_time <= tps.sec_since_epoch() ,"too soon for withdraw");
            action(
                permission_level {get_self(),"active"_n},
                "eosio.token"_n,
                "transfer"_n,
                std::make_tuple(from,to,quantity,std::string("send u back"))
            ).send();
            
            
        }

        ACTION setconfig(std::string  symbol,uint8_t timespan){
            auto config = settings_instance.get_or_create(get_self(),default_settings);
            config.token_symbol = symbol;
            config.withdraw_timespan = timespan;
            settings_instance.set(config,get_self());
           
        }

        uint32_t get_timespan (){
            if( settings_instance.exists()){
                return settings_instance.get().withdraw_timespan;
            }else{
                return 0;
            }    
        } 

        std::string get_token_symbol (){
            if( settings_instance.exists()){
                return settings_instance.get().token_symbol;
            }else{
                return "NOSYM";
            }    
        } 
        

};

