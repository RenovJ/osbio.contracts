#include "datatrader.hpp"
#include <eosiolib/asset.hpp>

using namespace eosio;

void datatrader::hi( name user ) {
    print("Hello, ", user);
}

void datatrader::adddatabegin(
    name provider,
    std::string datatype_name,
    uint64_t price,
    vector<std::string> detail_fields,
    uint64_t data_available_period,
    std::string data_hash_original,
    vector<segment> segments
) {
    // check provider
    require_auth(provider);
    
    // check datatype_name
    datatype_index types(_code, _code.value);
    auto iterator = types.begin();
    do {
      if ( (*iterator).datatype_name == datatype_name )
        break;
    } while (++iterator != types.end());
    eosio_assert(iterator != types.end(), "The datatype is invalid");
    
    // Matching idfs cluster for each data segment
    match_idfs_cluster(segments);
    
    // emplace data
    data_index dataset(_code, _code.value);
    auto it = dataset.begin();
    uint64_t size=0;
    for(auto& item : dataset ) {
      size++;
    }
    dataset.emplace(_self, [&](auto& row) {
        row.data_id = size;
        row.datatype_name = datatype_name;
        row.provider = provider;
        row.datetime = now();
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
    uint64_t data_id
    std::vector<std::string>
) {

}

void datatrader::adddatatype(
    name user,
    std::string datatypename,
    uint64_t detail_fields_num,
    std::vector<std::string> detail_fields
  ) {
    require_auth(user);
    datatype_index types(_code, _code.value);
    uint64_t size = std::distance(types.cbegin(), types.cend());
    auto iterator = types.begin();
    while(iterator != types.end()) {
        if ((*iterator).datatypename == datatypename)
            break;
        iterator++;
    }
    
    if( iterator == types.end() )
    {
        types.emplace(_self, [&](auto& row) {
            row.datatypeid = size;
            row.datatypename = datatypename;
            row.definer = user;
            row.fieldnum = fieldnum;
            row.field1name = field1;
            row.field2name = field2;
            row.field3name = field3;
            row.field4name = field4;
            row.field5name = field5;
            });
        
        print( "A new data type is added by ", user );
    } else {
        print( "The data type has already been registered by ", user);	
    }
}

void datatrader::buydata(
    name user,
    uint64_t data_id
) {
   require_auth(user);
   auto iterator = get_data_by_id(dataid);
   data d = *iterator;
   if (d.status != DATA_STATUS_ON_SALE) {
      print("The data is not on sale");
      return;
   }

   buy_history_index history(_code, _code.value);
   uint64_t size = std::distance(history.cbegin(), history.cend());
   auto it_his = history.begin();
   while(it_his != history.end()) {
        if ((*it_his).buyer == user &&
            (*it_his).dataid == dataid)
            break;
        it_his++;
   }
   // check if buyer has already purchased the data
   // ...

   history.emplace(_self, [&](auto& row) {
      row.history_id = size;
      row.buyer = user;
      row.dataid = dataid;
      row.datetime = now();
   });

   action(
     permission_level{user, "active"_n},
	 "osb.token"_n, "transfer"_n,
	 std::make_tuple(user, d.provider, asset(d.price, symbol("FDT", 4)), std::string("Data reward"))
   ).send();
}

void datatrader::removedata(
    name user,
    uint64_t data_id
) {
    require_auth(user);
    auto iterator = get_data_by_id(dataid);
    data d = *iterator;
    if (d.provider != user) {
        print("Only provider can remove data");
        return;
    }
    if (d.status == DATA_STATUS_REMOVED) {
        print("The data has already been removed");
        return;
    }
    data_index dataset(_code, _code.value);
    iterator = dataset.find(dataid);
    dataset.modify(iterator, user, [&](auto& row) {
        row.status = DATA_STATUS_REMOVED;
    });
}

void datatrader::addidfs(
    name idfs_account,
    uint64_t capacity,
    uint64_t cluster_id,
    string idfs_public_key,
    string ipaddr,
    uint64_t port
) {

}

void datatrader::addcluster(
    name idfs_account,
    string cluster_key
) {

}

datatrader::data_index::const_iterator datatrader::get_data_by_id(uint64_t dataid) {
    data_index dataset(_code, _code.value);
    auto iterator = dataset.find(dataid);
    eosio_assert(iterator != dataset.end(), "Dataid is invalid");
    return iterator;
}

bool datatrader::check_if_buy(name user, uint64_t dataid) {
  buy_history_index history(_code, _code.value);
  auto iterator = history.begin();
  if (iterator == history.end())
      return false;

  do {
        if ((*iterator).buyer == user &&
            (*iterator).dataid == dataid) {
            return true;
        }
  } while (++iterator != history.end());
  return false;
}

/**
 * Matching idfs clusters first whose usage is the lowest.
 * @TODO: Exception for a case of smaller number of clusters than the number of segments.
**/
void datatrader::match_idfs_cluster(vector<segment> segments) {
  auto it_cluster = _clusters.begin()
  auto min_usage_cluster = _clusters.end();
  for (uint64_t i = 0; i < segments.size(); i++) {
    min_usage_cluster = _clusters.end();
    do {
      if (min_usage_cluster == _clusters.end()) {
        if ((*it_cluster).capacity - (*it_cluster).usage < segments.at(i).size) {
          min_usage_cluster = it_cluster;
        } else {
          continue;
        }
      }
      
      if (min_usage_cluster.usage > (*it_cluster).usage && ) {
        // check matched already for previous segment
        for (uint64_t j = 0; j < i; j++) {
          if (segments.at(j).idfs_cluster_id == (*it_cluster).cluster_id)
            break;
        }
        if (j == i) {
          min_usage_cluster = *it_cluster
        }
      }
    } while (++it_cluster != _clusters.end());
  
    segments.at(i).idfs_cluster_id = (*it_cluster).cluster_id;
  }
}

EOSIO_DISPATCH( datatrader, (hi)(adddatabegin)(adddataend)(adddatatype)(buydata)(removedata)(addidfs)(addcluster) )
