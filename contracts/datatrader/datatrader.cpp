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
    uint64_t data_available_period,
    std::string data_hash_original,
    std::vector<segment> segments
) {
    // check provider
    require_auth(provider);
    
    // check datatype_name
    auto iterator = _datatype.begin();
    do {
      if ((*iterator).datatype_name == datatype_name)
        break;
    } while (++iterator != _datatype.end());
    eosio_assert(iterator != _datatype.end(), "The datatype is invalid");
    
    // Matching idfs cluster for each data segment
    match_idfs_cluster(segments);
    
    // emplace data
    uint64_t size = std::distance(_data.cbegin(), _data.cend());
    _data.emplace(_self, [&](auto& row) {
        row.data_id = size;
        row.datatype_name = datatype_name;
        row.provider = provider;
        row.datetime = current_time_point().sec_since_epoch();
        row.price = price;
        row.status = DATA_STATUS_ADDING;
        row.detail_fields = detail_fields;
        row.available_period = data_available_period;
        row.data_hash_original = data_hash_original;
        row.segments = segments; 
        });
}

void datatrader::adddataend(
    name provider,
    uint64_t data_id,
    std::vector<segment> segments
) {
    require_auth(provider);
    auto itData = get_data_by_id(data_id);
    for (int i = 0; i < segments.size(); i++) {
      for (auto s : (*itData).segments) {
        if (s.seg_no == segments.at(i).seg_no) {
          segments.at(i).size = (*itData).segments.at(i).size;
          segments.at(i).hash = (*itData).segments.at(i).hash;
          segments.at(i).idfs_cluster_id = (*itData).segments.at(i).idfs_cluster_id;
          
          // modify usage of idfs cluster
          auto itCluster = get_idfs_cluster_by_id((*itData).segments.at(i).idfs_cluster_id);
          _idfscluster.modify(itCluster, _self, [&](auto& row) {
            row.usage += (*itData).segments.at(i).size;
          });
        }
      }
    }
    
    _data.modify(itData, _self, [&](auto& row) {
      row.status = DATA_STATUS_ON_SALE;
      row.segments = segments;
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
    do {
        eosio_assert((*iterator).datatype_name != datatype_name, "The datatype is already exist");
    } while(++iterator != _datatype.end());
    
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
    uint64_t data_id
) {
   require_auth(user);
   auto it_data = get_data_by_id(data_id);
   data d = *it_data;
   
   eosio_assert(d.status == DATA_STATUS_ON_SALE, "The data is not on sale");

   uint64_t size = std::distance(_buyhistory.cbegin(), _buyhistory.cend());
   auto it = _buyhistory.begin();
   do {
     eosio_assert(
       (*it).buyer != user || (*it).data_id != data_id,
       "The user has already purchased the data"
     );
   } while(++it != _buyhistory.end());

   _buyhistory.emplace(_self, [&](auto& row) {
      row.buy_id = size;
      row.buyer = user;
      row.data_id = data_id;
      row.datetime = current_time_point().sec_since_epoch();
   });

   action(
     permission_level{user, "active"_n},
  	 "eosio.token"_n, "transfer"_n,
  	 std::make_tuple(user, d.provider, asset(d.price, symbol(TOKEN_SYMBOL, TOKEN_DECIMAL)),
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
    std::string idfs_public_key,
    std::string ipaddr,
    uint64_t port
) {
    require_auth(idfs_account);
    
    // TODO: setting cluster capacity
    // auto itCluster = get_idfs_cluster_by_id(cluster_id);
    
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
    do {
      eosio_assert((*iterator).cluster_key != cluster_key, "The cluster key is already exist");
    } while (++iterator != _idfscluster.end());
    
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
 * @TODO: Exception for a case of smaller number of clusters than the number of segments.
**/
void datatrader::match_idfs_cluster(std::vector<segment> segments) {
  auto it_cluster = _idfscluster.begin();
  auto min_usage_cluster = _idfscluster.end();
  for (uint64_t i = 0; i < segments.size(); i++) {
    min_usage_cluster = _idfscluster.end();
    do {
      if (min_usage_cluster == _idfscluster.end()) {
        if ((*it_cluster).capacity - (*it_cluster).usage < segments.at(i).size) {
          eosio_assert(segments.at(i).size > 0, "A segment size must be bigger than 0");
          min_usage_cluster = it_cluster;
        } else {
          continue;
        }
      }
      
      if (min_usage_cluster->usage > (*it_cluster).usage) {
        // check matched already for previous segment
        uint64_t j;
        for (j = 0; j < i; j++) {
          if (segments.at(j).idfs_cluster_id == (*it_cluster).cluster_id)
            break;
        }
        if (j == i) {
          min_usage_cluster = it_cluster;
        }
      }
    } while (++it_cluster != _idfscluster.end());
  
    segments.at(i).idfs_cluster_id = (*it_cluster).cluster_id;
  }
}

EOSIO_DISPATCH( datatrader, (hi)(adddatabegin)(adddataend)(adddatatype)(buydata)(removedata)(addidfs)(addcluster) )
