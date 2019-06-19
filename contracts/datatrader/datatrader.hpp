#include <eosiolib/eosio.hpp>
using namespace eosio;


class [[eosio::contract]] datatrader : public contract {
  public:
      //using contract::contract;

      const uint64_t DATA_STATUS_ON_SALE = 0x00;
      const uint64_t DATA_STATUS_REMOVED = 0x01;

      datatrader(name receiver, name code, datastream<const char*> ds )
        : contract(receiver, code, ds),
        _datas(receiver, code.value),
        _datatypes(receiver, code.value),
        _buyhistories(receiver, code.value) {}

      [[eosio::action]] void hi( name user );
      [[eosio::action]] void adddata( name provider,
        std::string datatypename,
        uint64_t price,
        std::string field1,
        std::string field2,
        std::string field3,
        std::string field4,
        std::string field5,
        std::string idfshash
        );
      [[eosio::action]] void adddatatype( name user,
        std::string datatypename,
        uint64_t fieldnum,
        std::string field1,
        std::string field2,
        std::string field3,
        std::string field4,
        std::string field5
        );
      [[eosio::action]] void buydata( name user, uint64_t dataid );
      [[eosio::action]] void removedata(name user, uint64_t dataid);
      [[eosio::action]] void datalist(std::string datatypename, name user);
      [[eosio::action]] void datatypelist();
      [[eosio::action]] void getdatatype(std::string datatypename);
      [[eosio::action]] void checkifbuy(name user, uint64_t dataid);

  private:
      struct [[eosio::table]] datatype {
        uint64_t datatypeid;
        std::string datatypename;
        name definer;
        uint64_t fieldnum;
        std::string field1name;
        std::string field2name;
        std::string field3name;
        std::string field4name;
        std::string field5name;

        uint64_t primary_key() const { return datatypeid; }
      };

      struct [[eosio::table]] data {
        uint64_t dataid;
        std::string datatypename;
        name provider;
        time_t datetime;
        uint64_t price;
        uint64_t status;
        std::string field1value;
        std::string field2value;
        std::string field3value;
        std::string field4value;
        std::string field5value;
        std::string idfshash;
        
        uint64_t primary_key() const { return dataid; }	
      };

      struct [[eosio::table]] buyhistory {
        uint64_t history_id;
        name buyer;
        uint64_t dataid;
        uint64_t datetime;

        uint64_t primary_key() const { return history_id; }	
      };

      typedef eosio::multi_index<"datatypes"_n, datatype> datatype_index;
      typedef eosio::multi_index<"datas"_n, data> data_index;
      typedef eosio::multi_index<"buyhistories"_n, buyhistory> buy_history_index;

      datatype_index    _datatypes;
      data_index        _datas;
      buy_history_index _buyhistories;

      data_index::const_iterator get_data_by_id(uint64_t dataid);
      void print_data_info(const data it);
      void print_datatype_info(const datatype it);
      bool check_if_buy(name user, uint64_t dataid);
};

