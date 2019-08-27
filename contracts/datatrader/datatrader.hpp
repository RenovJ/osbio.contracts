#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <math.h>
using namespace eosio;

class [[eosio::contract]] datatrader : public contract {
  public:
      //using contract::contract;

      const uint64_t DATA_STATUS_ON_SALE = 0x00;
      const uint64_t DATA_STATUS_ADDING = 0x01;
      const uint64_t DATA_STATUS_REMOVED = 0x02;
      const uint64_t DATA_STATUS_OUTDATED = 0x03;
      const std::string TOKEN_SYMBOL = "OSB";
      const uint64_t TOKEN_DECIMAL = 0;
      const name TOKEN_CONTRACT = "osb.token"_n;
      const std::string DATA_REWARD_MEMO = "Data reward";
      
      struct fragment {
        uint64_t fragment_no;
        uint64_t size;
        eosio::public_key encrypt_key;
        std::string hash_original;
        std::string hash_encrypted;
        uint64_t idfs_cluster_id;
        std::string cid;
      };
      
      datatrader(name receiver, name code, datastream<const char*> ds )
        : contract(receiver, code, ds),
        _data(receiver, code.value),
        _datatype(receiver, code.value),
        _buyhistory(receiver, code.value),
        _idfs(receiver, code.value),
        _idfscluster(receiver, code.value) {}

      [[eosio::action]] void hi( name user );
      [[eosio::action]] void adddatabegin(
        name provider,
        std::string datatype_name,
        uint64_t price,
        std::vector<std::string> detail_fields,
        uint64_t period,
        std::string data_hash_original,
        uint64_t size,
        std::vector<fragment> fragments
      );
      [[eosio::action]] void adddataend(
        name provider,
        uint64_t data_id,
        std::vector<fragment> fragments
      );
      [[eosio::action]] void adddatatype(
        name user,
        std::string datatype_name,
        uint64_t detail_fields_num,
        std::vector<std::string> detail_fields
      );
      [[eosio::action]] void buydata(
        name user,
        uint64_t data_id,
        eosio::public_key buyer_key
      );
      [[eosio::action]] void removedata(
        name user,
        uint64_t data_id
      );
      [[eosio::action]] void addidfs(
        name idfs_account,
        uint64_t capacity,
        uint64_t cluster_id,
        eosio::public_key idfs_public_key,
        std::string ipaddr,
        uint64_t port
      );
      [[eosio::action]] void addcluster(
        name idfs_account,
        std::string cluster_key
      );
      
  private:
      struct [[eosio::table]] data {
        uint64_t data_id;
        std::string datatype_name;
        name provider;
        time_t datetime;
        uint64_t price;
        uint64_t status;
        std::vector<std::string> detail_fields;
        uint64_t period;
        std::string data_hash_original;
        uint64_t size;
        std::vector<fragment> fragments;
        
        uint64_t primary_key() const { return data_id; } 
      };
      
      struct [[eosio::table]] datatype {
        uint64_t datatype_id;
        std::string datatype_name;
        name definer;
        uint64_t detail_fields_num;
        std::vector<std::string> detail_fields;

        uint64_t primary_key() const { return datatype_id; }
      };

      struct [[eosio::table]] buyhistory {
        uint64_t buy_id;
        name buyer;
        uint64_t data_id;
        uint64_t datetime;
        eosio::public_key buyer_key;

        uint64_t primary_key() const { return buy_id; }	
      };
      
      struct [[eosio::table]] idfs {
        uint64_t idfs_id;
        name account;
        eosio::public_key idfs_public_key;
        uint64_t capacity;
        time_t since;
        uint64_t cluster_id;
        std::string ipaddr;
        uint64_t port;
        
        uint64_t primary_key() const { return idfs_id; }
        uint64_t secondary_key() const { return cluster_id; }
      };
      
      struct [[eosio::table]] idfscluster {
        uint64_t cluster_id;
        std::string cluster_key;
        uint64_t usage;
        uint64_t capacity;
        
        uint64_t primary_key() const { return cluster_id; }
      };

      typedef eosio::multi_index<"data"_n, data> data_index;
      typedef eosio::multi_index<"datatype"_n, datatype> datatype_index;
      typedef eosio::multi_index<"buyhistory"_n, buyhistory> buyhistory_index;
      typedef eosio::multi_index<"idfs"_n, idfs,
      indexed_by<"getcluster"_n, const_mem_fun<idfs, uint64_t, &idfs::secondary_key>>> idfs_index;
      typedef eosio::multi_index<"idfscluster"_n, idfscluster> idfscluster_index;

      datatype_index    _datatype;
      data_index        _data;
      buyhistory_index  _buyhistory;
      idfs_index        _idfs;
      idfscluster_index _idfscluster;

      data_index::const_iterator get_data_by_id(uint64_t data_id);
      idfscluster_index::const_iterator get_idfs_cluster_by_id(uint64_t cluster_id);
      bool check_if_buy(name user, uint64_t data_id);
      std::vector<fragment> match_idfs_cluster(std::vector<fragment> fragments);
};

