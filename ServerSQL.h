#pragma once


#include "pqxx/pqxx"
#include <iostream>
#include <optional>
#include <vector>

// #include <string>


namespace serversql {

class ServerSQL {
    public:
        ServerSQL(std::string& coninf);
        pqxx::connection& GetConnection();
        const std::vector <std::string>& GetStorages () const; 
        void ExecuteSQLCommand (const std::string& command);
        
        void TableConfig ();

        template <class T>
        void CleanTable (T&& name);
     
        template <class F>
        void DeleteTable (F&& name);

        //std::vector < std::vector<std::string> > GetDeals (); // индекс первого вектора номер строки, индекс второго номер столбца
 
    private:
        pqxx::connection con_name_;
        std::vector <std::string> storages_{};
};

//bool ParsingSCV ( ServerSQL& server, std::ifstream& file );


template <class T>
void ServerSQL::CleanTable (T&& name) {
    T name_ = std::forward<T>(name);
    pqxx::work tx{con_name_};
    std::string result = R"(DELETE FROM )";
    
    tx.exec0(result + name_);
    tx.commit();
}

template <class F>
void ServerSQL::DeleteTable (F&& name) {
    F name_ = std::forward<F>(name);
    pqxx::work tx{con_name_};

    std::string result = R"(DROP TABLE IF EXISTS )";
    
    tx.exec0(result + name_);
    tx.commit();
}


} // конец namespace
