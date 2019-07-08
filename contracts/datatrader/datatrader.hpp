#include <eosiolib/eosio.hpp>
using namespace eosio;

class [[eosio::contract]] datatrader : public contract {
  public:
      //using contract::contract;

      const uint64_t DATA_STATUS_ON_SALE = 0x00;
      const uint64_t DATA_STATUS_ADDING = 0x01;
      const uint64_t DATA_STATUS_REMOVED = 0x02;

      datatrader(name receiver, name code, datastream<const char*> ds )
        : contract(receiver, code, ds),
        _data(receiver, code.value),
        _datatype(receiver, code.value),
        _buydata(receiver, code.value),
        _idfs(receiver, code.value),
        _idfscluster(receiver, code.value) {}

      [[eosio::action]] void hi( name user );
      [[eosio::action]] void adddatabegin(
        name provider,
        std::string datatype_name,
        uint64_t price,
        vector<std::string> detail_fields,
        uint64_t data_size,
        uint64_t data_available_period,
        uint64_t data_segment_quantity,
        std::string data_hash_original,
        vector<segment> segments
      );
      [[eosio::action]] void adddataend(
        name provider,
        uint64_t data_id
        std::vector<std::string>
      );
      [[eosio::action]] void adddatatype(
        name user,
        std::string datatype_name,
        uint64_t detail_fields_num,
        std::vector<std::string> detail_fields
      );
      [[eosio::action]] void buydata(
        name user,
        uint64_t data_id
      );
      [[eosio::action]] void removedata(
        name user,
        uint64_t data_id
      );
      [[eosio::action]] void addidfs(
        name idfs_account,
        uint64_t capacity,
        uint64_t cluster_id,
        string idfs_public_key,
        string ipaddr,
        uint64_t port
      );
      [[eosio::action]] void addcluster(
        name idfs_account,
        string cluster_key
      );
      
  private:
      struct segment {
        uint64_t no;
        uint64_t size;
        std::string hash;
        std::string hash_ipfs;
        name idfs_cluster_id;
      };
      
      struct [[eosio::table]] data {
        uint64_t data_id;
        std::string datatype_name;
        name provider;
        time_t datetime;
        uint64_t price;
        uint64_t status;
        vector<std::string> detail_fields;
        uint64_t available_period;
        std::string data_hash_original;
        vector<segment> segments;
        
        uint64_t primary_key() const { return data_id; } 
      };
      
      struct [[eosio::table]] datatype {
        uint64_t datatype_id;
        std::string datatype_name;
        name definer;
        uint64_t detail_fields_num;
        vector<std::string> detail_fields;

        uint64_t primary_key() const { return datatype_id; }
      };

      struct [[eosio::table]] buydata {
        uint64_t buy_id;
        name buyer;
        uint64_t data_id;
        uint64_t datetime;

        uint64_t primary_key() const { return buy_id; }	
      };
      
      struct [[eosio::table]] idfs {
        uint64_t idfs_id;
        name account;
        std::string idfs_public_key
        uint64_t capacity;
        time_t since;
        uint64_t cluster_id;
        std::string ipaddr;
        uint64_t post;
      };
      
      struct [[eosio::table]] idfscluster {
        uint64_t cluster_id;
        uint64_t usage;
        uint64_t capacity;
      };

      typedef eosio::multi_index<"data"_n, data> data_index;
      typedef eosio::multi_index<"datatype"_n, datatype> datatype_index;
      typedef eosio::multi_index<"buydata"_n, buydata> buydata_index;
      typedef eosio::multi_index<"idfs"_n, idfs> idfs_index;
      typedef eosio::multi_index<"idfscluster"_n, idfscluster> idfscluster_index;

      datatype_index    _datatype;
      data_index        _data;
      buydata_index     _buydata;
      idfs_index        _idfs;
      idfscluster_index _idfscluster;

      data_index::const_iterator get_data_by_id(uint64_t data_id);
      bool check_if_buy(name user, uint64_t data_id);
      void match_idfs_cluster(vector<segment> segments);
};

