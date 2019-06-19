#include "datatrader.hpp"
#include <eosiolib/asset.hpp>

using namespace eosio;

void datatrader::hi( name user ) {
    print("Hello, ", user);
}

void datatrader::adddata( name provider,
  std::string datatypename,
  uint64_t price,
  std::string field1,
  std::string field2,
  std::string field3,
  std::string field4,
  std::string field5,
  std::string idfshash
  ) {
    require_auth(provider);
    // check datatypename
    data_index dataset(_code, _code.value);
    auto it = dataset.begin();
    uint64_t size=0;
    for(auto& item : dataset ) {
      size++;
    }
    dataset.emplace(_self, [&](auto& row) {
        row.dataid = size;
        row.datatypename = datatypename;
        row.provider = provider;
        row.datetime = now();
        row.price = price;
        row.status = DATA_STATUS_ON_SALE;
        row.field1value = field1;
        row.field2value = field2;
        row.field3value = field3;
        row.field4value = field4;
        row.field5value = field5;
        row.idfshash = idfshash;
        });
    print( "A new data is added by ", provider );
}

[[eosio::action]]
void datatrader::adddatatype( name user,
  std::string datatypename,
  uint64_t fieldnum,
  std::string field1,
  std::string field2,
  std::string field3,
  std::string field4,
  std::string field5
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

[[eosio::action]]
void datatrader::buydata( name user, uint64_t dataid ) {
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

datatrader::data_index::const_iterator datatrader::get_data_by_id(uint64_t dataid) {
    data_index dataset(_code, _code.value);
    auto iterator = dataset.find(dataid);
    eosio_assert(iterator != dataset.end(), "Dataid is invalid");
    return iterator;
}

void datatrader::removedata(name user, uint64_t dataid) {
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

void datatrader::datalist(std::string datatypename, name user) {
    data_index dataset(_code, _code.value);
    auto it = dataset.find(0);
    if (it == dataset.end()) {
        print("No data added");
        return;
    }

    int count = 0;
    std::string result;
    print("{ ");
    print('"');
    print("data");
    print('"');
    print(": [");
//    print("{\n  \"data\": [");
    for(auto& item : dataset) {
        if (datatypename == "" || datatypename == item.datatypename) {
            if (count != 0)
                //print(",\n  ");
                print(", ");
            //print("{");
            print("{");
            print_data_info(item);
            if (check_if_buy(user, item.dataid) == true) {
                print(", ");
                print('"');
                print("purchased");
                print('"');
                print(": ");
                print('"');
                print("true");
                print('"');
            }
            else {
                print(", ");
                print('"');
                print("purchased");
                print('"');
                print(": ");
                print('"');
                print("false");
                print('"');

            }
            print("  }");
            count++;
        }
    }
    print("]}");
}

void datatrader::print_data_info(const data it) {
    std::string status;
    if (it.status == DATA_STATUS_ON_SALE) {
        status = "on_sale";
    } else if (it.status == DATA_STATUS_REMOVED) {
        status = "removed";
    }

    print('"');
    print("dataid");
    print('"');
    print(": ", it.dataid);
    print(", ");
    print('"');
    print("provider");
    print('"');
    print(": ");
    print('"');
    print(it.provider);
    print('"');
    print(", ");
    print('"');
    print("datatypename");
    print('"');
    print(": ");
    print('"');
    print(it.datatypename);
    print('"');
    print(", ");
    print('"');
    print("datetime");
    print('"');
    print(": ");
    print('"');
    print((uint32_t)it.datetime);
    print('"');
    print(", ");
    print('"');
    print("price");
    print('"');
    print(": ", it.price);
    print(", ");
    print('"');
    print("status");
    print('"');
    print(": ");
    print('"');
    print(status.c_str());
    print('"');
    print(",");
    print('"');
    print("field1");
    print('"');
    print(": ");
    print('"');
    print(it.field1value);
    print('"');
    print(", ");
    print('"');
    print("field2");
    print('"');
    print(": ");
    print('"');
    print(it.field2value);
    print('"');
    print(", ");
    print('"');
    print("field3");
    print('"');
    print(": ");
    print('"');
    print(it.field3value);
    print('"');
    print(", ");
    print('"');
    print("field4");
    print('"');
    print(": ");
    print('"');
    print(it.field4value);
    print('"');
    print(", ");
    print('"');
    print("field5");
    print('"');
    print(": ");
    print('"');
    print(it.field5value);
    print('"');
    print(", ");
    print('"');
    print("idfshash");
    print('"');
    print(": ");
    print('"');
    print(it.idfshash);
    print('"');
}

void datatrader::datatypelist() {
    datatype_index types(_code, _code.value);
    auto iterator = types.begin();
    if (iterator == types.end()) {
        print("No datatype added");
        return;
    }

    print("{\n  \"datatypename\": [");
    do {
        if (iterator != types.begin())
            print(",\n");
        print_datatype_info( *iterator );
    } while (++iterator != types.end());
    print("]\n}");
}

[[eosio::action]]
void datatrader::getdatatype(std::string datatypename) {
    datatype_index types(_code, _code.value);
    auto iterator = types.begin();
    if (iterator == types.end()) {
        print("The datatype is not found - ", datatypename.c_str());
        return;
    }

    do {
        if ((*iterator).datatypename == datatypename)
            print_datatype_info( *iterator );
    } while (++iterator != types.end());
}

void datatrader::print_datatype_info(const datatype it) {
    print("{\n    \"datatypeid\": ");
    print(it.datatypeid);
    print(",\n    \"datatypename\": \"", it.datatypename);
    print("\",\n    \"definer\": \"", it.definer);
    print("\",\n    \"fieldnum\": ", it.fieldnum);
    print(",\n    \"field1\": \"", it.field1name+"\",");
    print("\n    \"field2\": \"", it.field2name+"\",");
    print("\n    \"field3\": \"", it.field3name+"\",");
    print("\n    \"field4\": \"", it.field4name+"\",");
    print("\n    \"field5\": \"", it.field5name+"\"\n  }");
}

void datatrader::checkifbuy(name user, uint64_t dataid) {
    if (check_if_buy(user, dataid) == true)
        print("purchased");
    else
        print("not_purchased");
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
EOSIO_DISPATCH( datatrader, (hi)(adddata)(adddatatype)(buydata)(removedata)(datalist)(datatypelist)(getdatatype)(checkifbuy) )
