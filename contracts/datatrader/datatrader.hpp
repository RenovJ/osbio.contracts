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
      const name TOKEN_CONTRACT = "osbio.token"_n;
      const std::string DATA_REWARD_MEMO = "Data reward";
      const std::string KEEPER_REWARD_MEMO = "Keeper reward";
      const uint64_t MEGA_BYTE = 1024 * 1024;
      const uint64_t MAX_KEEPER_NUMBER_OF_CLUSTER = 5;
      const uint64_t DAY_SECONDS = 60 * 60 * 24;
      
      struct fragment {
        uint64_t fragment_no;
        uint64_t size;
        std::string encrypt_key;
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
        _idfscluster(receiver, code.value),
        _keeperreward(receiver, code.value),
        _keeperclaim(receiver, code.value) {}

      [[eosio::action]] void hi( name user );
      [[eosio::action]] void adddatabegin(
        name provider,
        std::string datatype_name,
        asset price,
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
        std::string buyer_key
      );
      [[eosio::action]] void removedata(
        name user,
        uint64_t data_id
      );
      [[eosio::action]] void addidfs(
        name idfs_account,
        uint64_t capacity,
        uint64_t cluster_id,
        std::string idfs_public_key,
        std::string ipaddr,
        uint64_t port
      );
      [[eosio::action]] void addcluster(
        name idfs_account,
        std::string cluster_key_hash
      );
      [[eosio::action]] void claimkreward(
        name idfs_account,
        uint64_t reward_id
      );
      
      
  private:
      struct [[eosio::table]] data {
        uint64_t data_id;
        std::string datatype_name;
        name provider;
        uint64_t timestamp;
        asset price;
        uint64_t status;
        std::vector<std::string> detail_fields;
        uint64_t period;
        std::string data_hash_original;
        uint64_t size;
        uint64_t total_storage_fee;
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
        uint64_t timestamp;
        std::string buyer_key;

        uint64_t primary_key() const { return buy_id; }	
      };
      
      struct [[eosio::table]] idfs {
        uint64_t idfs_id;
        name account;
        std::string idfs_public_key;
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
        std::string cluster_key_hash;
        uint64_t usage;
        uint64_t capacity;
        uint64_t fee_ratio;
        std::vector<uint64_t> idfs_list;
        
        uint64_t primary_key() const { return cluster_id; }
      };
      
      struct [[eosio::table]] keeperreward {
        uint64_t reward_id;
        uint64_t data_id;
        uint64_t fragment_no;
        uint64_t cluster_id;
        name idfs_account;
        asset reward_total;
        asset reward_claimed;
        
        uint64_t primary_key() const { return reward_id; }
      };
      
      struct [[eosio::table]] keeperclaim {
        uint64_t claim_id;
        uint64_t reward_id;
        asset quantity;
        uint64_t timestamp;
        
        uint64_t primary_key() const { return claim_id; }
      };

      typedef eosio::multi_index<"data"_n, data> data_index;
      typedef eosio::multi_index<"datatype"_n, datatype> datatype_index;
      typedef eosio::multi_index<"buyhistory"_n, buyhistory> buyhistory_index;
      typedef eosio::multi_index<"idfs"_n, idfs,
      indexed_by<"getcluster"_n, const_mem_fun<idfs, uint64_t, &idfs::secondary_key>>> idfs_index;
      typedef eosio::multi_index<"idfscluster"_n, idfscluster> idfscluster_index;
      typedef eosio::multi_index<"keeperreward"_n, keeperreward> keeperreward_index;
      typedef eosio::multi_index<"keeperclaim"_n, keeperclaim> keeperclaim_index;

      data_index::const_iterator get_data_by_id(uint64_t data_id);
      idfscluster_index::const_iterator get_idfs_cluster_by_id(uint64_t cluster_id);
      idfs_index::const_iterator get_idfs_by_id(uint64_t keeper_id);
      keeperreward_index::const_iterator get_reward_by_id(uint64_t reward_id);
      bool check_if_buy(name user, uint64_t data_id);
      std::vector<fragment> match_idfs_cluster(std::vector<fragment> fragments);

      datatype_index      _datatype;
      data_index          _data;
      buyhistory_index    _buyhistory;
      idfs_index          _idfs;
      idfscluster_index   _idfscluster;
      keeperreward_index  _keeperreward;
      keeperclaim_index   _keeperclaim;
};

