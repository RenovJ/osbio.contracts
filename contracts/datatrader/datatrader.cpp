#include "datatrader.hpp"

using namespace eosio;
using namespace eosio::internal_use_do_not_use;

void datatrader::hi( name user ) {
    print("Hello, ", user);
}

void datatrader::adddatabegin(
    name provider,
    std::string datatype_name,
    uint64_t price,
    std::vector<std::string> detail_fields,
    uint64_t period,
    std::string data_hash_original,
    uint64_t size,
    std::vector<fragment> fragments
) {
    // check provider
    require_auth(provider);
    
    // check datatype_name
    auto iterator = _datatype.begin();
    eosio_assert(_datatype.begin() != _datatype.end(), "There is no data type");
    do {
      if ((*iterator).datatype_name == datatype_name)
        break;
    } while (++iterator != _datatype.end());
    eosio_assert(iterator != _datatype.end(), "The datatype is invalid");
    
    // Matching idfs cluster for each data fragment
    fragments = match_idfs_cluster(fragments);
    
    // emplace data
    uint64_t dataListLength = std::distance(_data.cbegin(), _data.cend());
    _data.emplace(_self, [&](auto& row) {
        row.data_id = dataListLength + 1;
        row.datatype_name = datatype_name;
        row.provider = provider;
        row.datetime = current_time_point().sec_since_epoch();
        row.price = price;
        row.status = DATA_STATUS_ADDING;
        row.detail_fields = detail_fields;
        row.period = period;
        row.data_hash_original = data_hash_original;
        row.size = size;
        row.fragments = fragments; 
        });
}

void datatrader::adddataend(
    name provider,
    uint64_t data_id,
    std::vector<fragment> fragments
) {
    require_auth(provider);
    auto itData = get_data_by_id(data_id);
    for (int i = 0; i < fragments.size(); i++) {
      for (auto f : (*itData).fragments) {
        if (f.fragment_no == fragments.at(i).fragment_no) {
          fragments.at(i).size = (*itData).fragments.at(i).size;
          fragments.at(i).hash_original = (*itData).fragments.at(i).hash_original;
          fragments.at(i).hash_encrypted = (*itData).fragments.at(i).hash_encrypted;
          fragments.at(i).idfs_cluster_id = (*itData).fragments.at(i).idfs_cluster_id;
          
          // modify usage of idfs cluster
          auto itCluster = get_idfs_cluster_by_id((*itData).fragments.at(i).idfs_cluster_id);
          _idfscluster.modify(itCluster, _self, [&](auto& row) {
            row.usage += (*itData).fragments.at(i).size;
          });
        }
      }
    }
    
    _data.modify(itData, _self, [&](auto& row) {
      row.status = DATA_STATUS_ON_SALE;
      row.fragments = fragments;
    });
}

void datatrader::adddatatype(
    name user,
    std::string datatype_name,
    uint64_t detail_fields_num,
    std::vector<std::string> detail_fields
  ) {
    require_auth(user);
    uint64_t size = std::distance(_datatype.cbegin(), _datatype.cend());
    auto iterator = _datatype.begin();
    if (iterator != _datatype.end()) {
      do {
          eosio_assert((*iterator).datatype_name != datatype_name, "The datatype is already exist");
      } while(++iterator != _datatype.end());
    }
    
    _datatype.emplace(_self, [&](auto& row) {
        row.datatype_id = size;
        row.datatype_name = datatype_name;
        row.definer = user;
        row.detail_fields_num = detail_fields_num;
        row.detail_fields = detail_fields;
        });
}

void datatrader::buydata(
    name user,
    uint64_t data_id,
    eosio::public_key buyer_key
) {
   require_auth(user);
   auto it_data = get_data_by_id(data_id);
   data d = *it_data;
   
   eosio_assert(d.status == DATA_STATUS_ON_SALE, "The data is not on sale");

   uint64_t size = std::distance(_buyhistory.cbegin(), _buyhistory.cend());
   auto it = _buyhistory.begin();
   if (_buyhistory.begin() != _buyhistory.end()) {
     do {
       eosio_assert(
         (*it).buyer != user || (*it).data_id != data_id,
         "The user has already purchased the data"
       );
     } while(++it != _buyhistory.end());
   }

   _buyhistory.emplace(_self, [&](auto& row) {
      row.buy_id = size;
      row.buyer = user;
      row.data_id = data_id;
      row.datetime = current_time_point().sec_since_epoch();
      row.buyer_key = buyer_key;
   });

   action(
     permission_level{user, "active"_n},
  	 TOKEN_CONTRACT, "transfer"_n,
  	 std::make_tuple(user, d.provider, asset(d.price * pow(10, TOKEN_DECIMAL), symbol(TOKEN_SYMBOL, TOKEN_DECIMAL)),
  	 std::string(DATA_REWARD_MEMO))
   ).send();
}

void datatrader::removedata(
    name user,
    uint64_t data_id
) {
    require_auth(user);
    auto iterator = get_data_by_id(data_id);
    data d = *iterator;
    eosio_assert(d.provider == user, "Only provider can remove data");
    eosio_assert(d.status != DATA_STATUS_REMOVED, "The data has already been removed");
    iterator = _data.find(data_id);
    _data.modify(iterator, _self, [&](auto& row) {
        row.status = DATA_STATUS_REMOVED;
    });
}

void datatrader::addidfs(
    name idfs_account,
    uint64_t capacity,
    uint64_t cluster_id,
    eosio::public_key idfs_public_key,
    std::string ipaddr,
    uint64_t port
) {
    require_auth(idfs_account);
    
    auto itCluster = get_idfs_cluster_by_id(cluster_id);
    if ((*itCluster).capacity == 0 || capacity < (*itCluster).capacity) {
      _idfscluster.modify(itCluster, _self, [&](auto& row) {
        row.capacity = capacity;
      });
    }
    
    uint64_t size = std::distance(_idfs.cbegin(), _idfs.cend());
    _idfs.emplace(_self, [&](auto& row) {
      row.idfs_id = size;
      row.account = idfs_account;
      row.idfs_public_key = idfs_public_key;
      row.capacity = capacity;
      row.since = current_time_point().sec_since_epoch();
      row.cluster_id = cluster_id;
      row.ipaddr = ipaddr;
      row.port = port;
    });
}

void datatrader::addcluster(
    name idfs_account,
    std::string cluster_key
) {
    require_auth(idfs_account);
    
    // check if the cluster key is already exist
    auto iterator = _idfscluster.begin();
    if (iterator != _idfscluster.end()) {
      do {
        eosio_assert((*iterator).cluster_key != cluster_key, "The cluster key is already exist");
      } while (++iterator != _idfscluster.end());
    }
    
    uint64_t size = std::distance(_idfscluster.cbegin(), _idfscluster.cend());
    _idfscluster.emplace(_self, [&](auto& row) {
      row.cluster_id = size;
      row.cluster_key = cluster_key;
    });
}

datatrader::data_index::const_iterator datatrader::get_data_by_id(uint64_t data_id) {
    auto iterator = _data.find(data_id);
    eosio_assert(iterator != _data.end(), "Data id is invalid");
    return iterator;
}

datatrader::idfscluster_index::const_iterator datatrader::get_idfs_cluster_by_id(uint64_t cluster_id) {
    auto iterator = _idfscluster.find(cluster_id);
    eosio_assert(iterator != _idfscluster.end(), "The idfs cluster id is invalid");
    return iterator;
}

bool datatrader::check_if_buy(name user, uint64_t data_id) {
  auto iterator = _buyhistory.begin();
  if (iterator == _buyhistory.end())
      return false;

  do {
    if ((*iterator).buyer == user &&
        (*iterator).data_id == data_id) {
        return true;
    }
  } while (++iterator != _buyhistory.end());
  return false;
}

/**
 * Matching idfs clusters first whose usage is the lowest.
 * @TODO: Exception for a case of smaller number of clusters than the number of fragments.
**/
std::vector<datatrader::fragment> datatrader::match_idfs_cluster(std::vector<fragment> fragments) {
  eosio_assert(_idfscluster.begin() != _idfscluster.end(), "No idfs cluster");
  for (uint64_t i = 0; i < fragments.size(); i++) {
    eosio_assert(fragments.at(i).size > 0, "A fragment size must be bigger than 0");
    auto min_usage_cluster = _idfscluster.end();
    auto it_cluster = _idfscluster.begin();
    do {
      if (min_usage_cluster == _idfscluster.end()) {
        if ((*it_cluster).capacity - (*it_cluster).usage >= fragments.at(i).size) {
          min_usage_cluster = it_cluster;
        }
        continue;
      }
      if (min_usage_cluster->usage >= (*it_cluster).usage) {
        // check matched already for previous fragment
        uint64_t j;
        for (j = 0; j < i; j++) {
          if (fragments.at(j).idfs_cluster_id == (*it_cluster).cluster_id) {
            break;
          }
        }
        if (j == i) {
          min_usage_cluster = it_cluster;
        }
      }
    } while (++it_cluster != _idfscluster.end());
    eosio_assert((*min_usage_cluster).capacity - (*min_usage_cluster).usage >= fragments.at(i).size,
      "There is no idfs cluster to match");
    fragments.at(i).idfs_cluster_id = (*min_usage_cluster).cluster_id;
  }
  
  return fragments;
}

EOSIO_DISPATCH( datatrader, (hi)(adddatabegin)(adddataend)(adddatatype)(buydata)(removedata)(addidfs)(addcluster) )
