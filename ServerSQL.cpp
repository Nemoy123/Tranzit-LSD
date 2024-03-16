#include "ServerSQL.h"
#include <fstream>
#include <utility>
#include <iostream>
#include <vector>
#include <clocale>
#include <windows.h>


using namespace std;

namespace serversql {
    

void ServerSQL::ExecuteSQLCommand (const std::string& command) {
    pqxx::work tx{GetConnection()};
    tx.exec0(command);
    tx.commit(); 
}

const std::vector <std::string>& ServerSQL::GetStorages () const {
    return storages_;
}

pqxx::connection& ServerSQL::GetConnection() {
    return con_name_;
}

ServerSQL::ServerSQL(std::string& coninf)
                    : con_name_(std::move(coninf))
        {
            std::cout << "Connected to " << con_name_.dbname() << std::endl;
            
            //обновить storage из базы sql
            pqxx::work tx{con_name_};
            for (auto [table_name] : tx.query<std::string>("SELECT tablename FROM pg_catalog.pg_tables WHERE schemaname = 'public'"))
            {
                //std::cout << "Таблица " << table_name << std::endl;
                storages_.push_back(table_name);
            }
        }


void ServerSQL::TableConfig () {
    pqxx::work tx{con_name_};
    std::string result = R"(
        CREATE TABLE IF NOT EXISTS deals (
            id BIGSERIAL NOT NULL PRIMARY KEY,
            manager VARCHAR(150),
            date_of_deal DATE,
            customer VARCHAR(150),  
            number_1c NUMERIC (25, 0),
            number_dop_1c NUMERIC (10, 0),
            postavshik VARCHAR(150), 
            neftebaza  VARCHAR(150), 
            tovar_short_name VARCHAR(250),
            litres NUMERIC (25, 2),
            plotnost NUMERIC (5, 4),
            ves NUMERIC (25, 0),
            price_in_tn NUMERIC (25, 0),
            price_out_tn NUMERIC (25, 0),
            price_out_litres NUMERIC (25, 2),
            transp_cost_tn NUMERIC (25, 2),
            commission NUMERIC (25, 2),
            rentab_tn NUMERIC (25, 2),
            profit NUMERIC (25, 2),
            summ NUMERIC (25, 2)
        )
    )";

    
    std::string making_storages_table = R"(
        CREATE TABLE IF NOT EXISTS storages 
        (
            id BIGSERIAL NOT NULL PRIMARY KEY,
            date_of_deal DATE NOT NULL,
            operation VARCHAR(150),
            start_balance NUMERIC (25, 2),
            arrival_doc NUMERIC (25, 2),
            arrival_fact NUMERIC (25, 2),
            departure_litres NUMERIC (25, 2),
            plotnost NUMERIC (5, 4),
            departure_kg NUMERIC (25, 2),
            balance_end NUMERIC (25, 2),
            nedoliv NUMERIC (25, 2),
            price_tn NUMERIC (25, 2),
            rjd_number NUMERIC (25, 0),
            storage_name VARCHAR(150),
            main_table_id BIGINT UNIQUE,
            FOREIGN KEY (main_table_id) REFERENCES deals (id) ON DELETE CASCADE
        )

    )";
    
   //storage_id BIGINT REFERENCES ??????????????? название_склада (id)
    tx.exec0(result);
    tx.exec0(making_storages_table);
    tx.commit();

}
std::vector < std::vector<std::string> > GetDeals() {
    
}

} // конец namespace