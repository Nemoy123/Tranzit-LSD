// #include "parsing.h"
#include <vector>
#include <iostream>
#include <clocale>
#include <windows.h>

using namespace std;
using namespace serversql;

std::string ValuesString (const std::vector <std::string>& vect) {
    std::stringstream out;
    out << "(";
    bool begin = true;
    for (const auto& stroke :vect) {
        if (!begin) {
            out << ",";
        }
        if (stroke.empty()) {
            out << "NULL";
        }
        else {
            out << "'" << stroke << "'";
        }
        
        begin = false;
    }
    out << ")";
    return out.str();
}

void StorageOperationWithMakeNB (ServerSQL& server, const std::vector <std::string>& buffer, int num) {
    if (buffer[2].substr(0,4) == "НБ" && buffer[2].size() > 6) {  // если приход на базу
        
            string name_storage = buffer[2];
            //заменить пробелы в названии на "_"
            for (auto& ch : name_storage) { if (ch == 32) { ch = 95;  } }

            // auto store_ptr = std::find (cbegin(server.GetStorages()), cend(server.GetStorages()), name_storage);
            // if (store_ptr == cend(server.GetStorages())) { // если склада нет
            //     server.CreateStorage(name_storage);
            // }
           //добавить операцию на склад
            stringstream out;
            out << R"(INSERT INTO storages (date_of_deal,  operation, rjd_number, arrival_doc, price_tn, storage_name, main_table_id) VALUES )";
            //out << R"(INSERT INTO )" << name_storage << R"( (date_of_deal,  operation, rjd_number, arrival_doc, price_tn) VALUES )";
            std::vector <std::string> new_vect { buffer[1], buffer[2], buffer[3], buffer[10], buffer[11], name_storage, std::to_string(num) };
            out << ValuesString (new_vect);
            server.ExecuteSQLCommand (out.str());
            // //внести номер в основную таблицу
            // pqxx::work tx{server.GetConnection()};
            // out.clear();
            // out.str( std::string () );
            // out << R"( SELECT id FROM storages ORDER BY id DESC LIMIT 1 )";
            // return tx.query_value<int>( out.str() );
    } 
    else if (buffer[5].substr(0,4) == "НБ") {  // списание с базы
        std::string name_storage {};
        if ( buffer[6].substr(0,4) != "НБ" ) {  name_storage = "НБ_" + buffer[6]; }
        else { name_storage = buffer[6]; }
        
       //заменить пробелы в названии на "_"
        for (auto& ch : name_storage) { if (ch == 32) { ch = 95; } }
        // auto store_ptr = std::find (cbegin(server.GetStorages()), cend(server.GetStorages()), name_storage);
        // if (store_ptr == cend(server.GetStorages())) { // если склада нет
        //     server.CreateStorage(name_storage);
        // }
        //добавить операцию на склад
        stringstream out;
        out << R"(INSERT INTO storages (date_of_deal, operation, departure_litres, plotnost, departure_kg, storage_name, main_table_id) VALUES )";

        // out << R"(INSERT INTO )" << name_storage 
        //     << R"( (date_of_deal, operation, departure_litres, plotnost, departure_kg) VALUES )";
        std::vector <std::string> new_vect { buffer[1], buffer[2], buffer[8], buffer[9], buffer[10], name_storage, std::to_string(num) };
        out << ValuesString ( new_vect );
        server.ExecuteSQLCommand ( out.str() );
        // //внести номер в основную таблицу
        // pqxx::work tx{server.GetConnection()};
        // out.clear();
        // out.str( std::string () );
        // out << R"(SELECT id FROM storages ORDER BY id DESC LIMIT 1 )";
        // return tx.query_value<int>( out.str() );
    }
    
    // else {
    //     return 0;
    // }
}

bool is_valid_utf8(const char * string){
    if(!string){return true;}
    const unsigned char * bytes = (const unsigned char *)string;
    unsigned int cp;
    int num;
    while(*bytes != 0x00){
        if((*bytes & 0x80) == 0x00){
            // U+0000 to U+007F 
            cp = (*bytes & 0x7F);
            num = 1;
        }
        else if((*bytes & 0xE0) == 0xC0){
            // U+0080 to U+07FF 
            cp = (*bytes & 0x1F);
            num = 2;
        }
        else if((*bytes & 0xF0) == 0xE0){
            // U+0800 to U+FFFF 
            cp = (*bytes & 0x0F);
            num = 3;
        }
        else if((*bytes & 0xF8) == 0xF0){
            // U+10000 to U+10FFFF 
            cp = (*bytes & 0x07);
            num = 4;
        }
        else{return false;}
        bytes += 1;
        for(int i = 1; i < num; ++i){
            if((*bytes & 0xC0) != 0x80){return false;}
            cp = (cp << 6) | (*bytes & 0x3F);
            bytes += 1;
        }
        if( (cp > 0x10FFFF) ||
            ((cp <= 0x007F) && (num != 1)) || 
            ((cp >= 0xD800) && (cp <= 0xDFFF)) ||
            ((cp >= 0x0080) && (cp <= 0x07FF)  && (num != 2)) ||
            ((cp >= 0x0800) && (cp <= 0xFFFF)  && (num != 3)) ||
            ((cp >= 0x10000)&& (cp <= 0x1FFFFF)&& (num != 4)) ){return false;}
    }
    return true;
}

string cp1251_to_utf8(const char *str){
    string res; 
    int result_u, result_c;
    result_u = MultiByteToWideChar(1251, 0, str, -1, 0, 0);
    if(!result_u ){return 0;}
    wchar_t *ures = new wchar_t[result_u];
    if(!MultiByteToWideChar(1251, 0, str, -1, ures, result_u)){
        delete[] ures;
        return 0;
    }
    result_c = WideCharToMultiByte(65001, 0, ures, -1, 0, 0, 0, 0);
    if(!result_c){
        delete [] ures;
        return 0;
    }
    char *cres = new char[result_c];
    if(!WideCharToMultiByte(65001, 0, ures, -1, cres, result_c, 0, 0)){
        delete[] cres;
        return 0;
    }
    delete[] ures;
    res.append(cres);
    delete[] cres;
    return res;
}


void ChangeWinTOUTF8 (std::vector <std::string>& buffer) {
    for (auto& word : buffer) {
        
        if (!word.empty()) {
            bool test = false;
            for (auto& ch:word) {
                if (!is_valid_utf8(&ch)) {
                    test = true;
                    break;
                }
            }
            if (test) {
                word = cp1251_to_utf8(&word[0]);
            }
        }
        
    }
}


std::string& CleanSpaces (string& word) {
    while (true) {
        if (!word.empty()) {
            if (word[0] == 32) {  // 32 space
                word = word.substr(1);
            } else {
                if (word.back() == 32) {
                    word.pop_back();
                }
                else {
                    break;
                }
            }
        } 
        else {
            break;
        }
    }
    return word;
}


void PrepareDate (std::string& word) {
    if (!word.empty()) {
        string day = word.substr(0, 2);
        string month = word.substr(3, 2);
        string year = word.substr(6, 4);
       // word = year + '-' + month + '-' + day;
        word =  day + '-' + month + '-' + year;
    }
}

void PrepareNumber (std::string& word) {
    

    while (true) {
        if (!word.empty()) {
            //cout << word.back() << endl;
            if (word.back() < 48 || word.back() > 57) { // �� �����
               // cout << word.back() << endl;
                word.pop_back();
            }
            else {
                break;
            }
        }
        else {
            break;
        }
    }
    auto iter = word.find(",");
    if (iter != std::string::npos) {
        word[iter] = '.';
    }
    
    iter = word.find(" ");
    while (iter != std::string::npos) {
        word.erase(iter,1);
        iter = word.find(" ");
    }
    // if (word.empty()) {
    //     word = "NULL";
    // }
}


std::vector <std::string>& PrepareVectorToSQL (std::vector <std::string>& vect) {
    PrepareDate (vect[1]);
    PrepareNumber (vect[4]);
    
    
    for (auto i = 8; i < vect.size(); ++i) {    
        PrepareNumber (vect[i]);
    }
    return vect;
}

bool ParsingSCV ( ServerSQL& server, std::ifstream& file ) {
    if (!file.is_open()) {
         throw std::runtime_error ("FILE NOT OPEN");
    }
    
  std::string line;
  SetConsoleCP(CP_UTF8);
  SetConsoleOutputCP(CP_UTF8);

  while (getline(file, line)) {
    std::vector <std::string> buffer;
    buffer.reserve(25);
    int counter = 0;
    std::string word{};
    for (auto& ch : line) {
        if (ch != ';') {  word += ch; }
        else {
            buffer.push_back( CleanSpaces(word) );
            word.clear();
        }
    }
    PrepareVectorToSQL (buffer);

    // замена кодировки 1251 на UTF8
    ChangeWinTOUTF8 (buffer);
    
    
    //buffer.push_back( num != 0 ? std::to_string(num) : ""s );

    
    // внести запись в основную таблицу
    string begin_part = R"(INSERT INTO deals (manager, date_of_deal, customer, number_1c, number_dop_1c, postavshik, neftebaza, 
        tovar_short_name, litres, plotnost, ves, price_in_tn, price_out_tn, price_out_litres, transp_cost_tn,
        commission, rentab_tn, profit, summ) VALUES )";
    string second_part = ValuesString (buffer);
    string result = begin_part + second_part;
    //std::cout << result << std::endl;
    server.ExecuteSQLCommand (result);
    // //внести номер в основную таблицу
    int num_id = 0;
    {
        pqxx::work tx{server.GetConnection()};
        std::stringstream out;
        out << R"( SELECT id FROM deals ORDER BY id DESC LIMIT 1 )";
        num_id = tx.query_value<int>( out.str() );
    }

    // оформить приход на склад в storages
    StorageOperationWithMakeNB (server,  buffer, num_id);
  }
  
  return true;
}

