#include "mainwindow.h"
#include "connection.h"
#include "./ui_mainwindow.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QSqlDriver>
#include <QSqlRecord>
#include <QSqlField>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QTableView>
#include <QVariant>
#include <QMap>



void MainWindow::UpdateListStorage() {
    ui->listWidget->clear();
    QString command = "SELECT DISTINCT storage_name FROM storages";
    QSqlQuery change_query = QSqlQuery(db_);
    if (!change_query.exec( command )) {
        qDebug() << change_query.lastError().databaseText();
        qDebug() << change_query.lastError().driverText();
        return;
    }
    else {
        //auto i = 0;
        while (change_query.next()) {
            ui->listWidget->addItem( change_query.value(0).toString() );
           // ++i;
        }

    }
    ui->listWidget->addItem ("_Все склады");
    ui->listWidget->sortItems();

}

QString ValuesString (const QVector <QString>& vect) {
    QString result = {};
    QTextStream out(&result);
    out << "(";
    bool begin = true;
    for (const auto& stroke :vect) {
        if (!begin) {
            out << ",";
        }
        if (stroke.isEmpty() || stroke == "NULL") {
            out << "NULL";
        }
        else {
            out << "'" << stroke << "'";
        }

        begin = false;
    }
    out << ")";
    //QString result = out.readAll();
    // out >> result;
    return result;
}

//template <typename T>
QString MainWindow::AddRowSQLString (const QString& storage, const QMap<QString, QString>& date_){
    //T date_ = std::forward<QMap<QString, QString>>(date);
    QString command = "INSERT INTO ";
    command += storage + " (";

    QString key_stroke{};
    QString value_stroke{};
    bool begin = true;
    for (auto item = date_.cbegin(), end = date_.cend(); item != end; ++item) {
        if (!item.value().isEmpty()) {
            if (!begin) {
                key_stroke += ", ";
                value_stroke += ", ";
            }
            key_stroke += item.key();
            if (item.value() != "") {
                value_stroke += "'"+item.value()+"'";
            } else {value_stroke += "NULL";}
            begin = false;
        }
    }
    key_stroke += ")";
    value_stroke += ")";
    command += key_stroke +" VALUES ("+ value_stroke + " RETURNING id";
    if (auto query = ExecuteSQL(command)) {
        while(query.value().next()) {
            return query.value().value(0).toString();
        }
    }

    return "-1";

}

bool MainWindow::AddRowSQL (const QString& storage, const QMap<QString, QString>& date_) {

    return ( AddRowSQLString (storage, date_) != "-1");
}

bool MainWindow::DeleteFromSQL (const QString& storage, const QString& id) {
    QString command = "DELETE FROM ";
    command += storage + " WHERE main_table_id = '" + id + "'";
    auto query = ExecuteSQL(command);
    if (query->isActive()) { // isActive() вернет true если запрос SQL успешен
        return true;
    }
    return false;
}

MainWindow::MainWindow( QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    createConnection();
    //if(!createConnection()) return 1;// Ситуацию возврата можно заменить в зависимости от разных ситуаций
    // Указываем подключение к базе данных
    db_ = QSqlDatabase::database("connection1");

    UpdateListStorage();
}

MainWindow::~MainWindow()
{
    delete ui;
}

QVariant MainWindow::FindID (int row, int column) {
    QModelIndex index_id = model->index(row, column);
    QVariant data = model->data(index_id);
    qDebug() << data;
    return data;
}

std::optional<QSqlQuery> MainWindow::ExecuteSQL(const QString& command){
    QSqlQuery query = QSqlQuery(db_);
    if (!query.exec( command )) {
        qDebug() << query.lastError().databaseText();
        qDebug() << query.lastError().driverText();
        return {};
    }
    return query;
}




bool MainWindow::StorageAdding(const QString& id_string, QString& new_text) {
    // проверить надо ли добавлять (поиск НБ в клиенте и в поставщике)
    auto query_test = ExecuteSQL(QString{"SELECT customer, postavshik FROM deals WHERE id =" + id_string});
    query_test.value().first();
    if (       query_test.value().value(0).toString().indexOf("НБ") == 0
        || query_test.value().value(1).toString().indexOf("НБ") == 0
        || new_text.indexOf("НБ") == 0
        ) {
        //command_store = "INSERT INTO storages (date_of_deal,  operation, storage_name, tovar_short_name, arrival_doc, departure_litres, plotnost, departure_kg, price_tn, main_table_id) VALUES ";

        // получить нужные данные из deals
        QString command_to_deals = "SELECT date_of_deal, customer, postavshik, neftebaza, tovar_short_name, litres, plotnost, ves, price_in_tn, id, price_out_tn FROM deals WHERE id =";
        command_to_deals += id_string;

        QVector<QString> vect_deals;
        if (auto query = ExecuteSQL(command_to_deals)) {
            while(query.value().next()) {
                for (auto i = 0; i < 11; ++i) {
                    vect_deals.push_back( query.value().value(i).toString() ); // первое value это от optional
                }
            }
        }
        //Удалить старые данные из storages привязанные к ID основной таблицы
        DeleteFromSQL("storages", id_string);

        //Внести новые данные
        QMap<QString, QString> command;

        command["date_of_deal"] = vect_deals.at(0); // дата
        command["tovar_short_name"] = vect_deals.at(4); // заполняем название товара
        command["main_table_id"] = vect_deals.at(9); // заполняем ИД основной таблицы

        if (vect_deals.at(1).indexOf("НБ") == 0) { // приход на базу или перемещение между базами

            command["operation"] = vect_deals.at(1); // перемещаем графу поставщик + нефтебаза
            //new_text.simplified().replace(char{32}, char{95});
            command["storage_name"] = vect_deals.at(1); // заполняем название склада
            command["arrival_doc"] = vect_deals.at(7); // заполняем вес прихода
            command["price_tn"] = vect_deals.at(8); // заполняем цену ЗАКУПКИ

            if (vect_deals.at(2).indexOf("НБ") == 0) {
                // если перемещение между складами списать со склада донора
                QMap<QString, QString> date;
                date["date_of_deal"] = vect_deals.at(0); // дата
                date["operation"] = vect_deals.at(1); // на какой склад отправляем
                date["storage_name"] = vect_deals.at(2) + "_" + vect_deals.at(3); // название склада с кот списываем
                date["tovar_short_name"] = vect_deals.at(4); // название товара
                date["departure_litres"] = vect_deals.at(5); // заполняем литры
                date["plotnost"] = vect_deals.at(6); // заполняем плотность
                date["departure_kg"] = vect_deals.at(7); // заполняем вес отгруженного
                date["price_tn"] = vect_deals.at(8); // заполняем цену
                date["main_table_id"] = vect_deals.at(9); // заполняем ИД основной таблицы

                if (!AddRowSQL("storages", date)) {
                    qDebug() << "ERROR AddStorageDate date";
                }
            }
        } else { // списание со склада клиенту

            command["operation"] = vect_deals.at(1); // куда списали
            command["storage_name"] = "НБ_"+vect_deals.at(3); // заполняем название склада
            command["departure_litres"] = vect_deals.at(5);
            command["plotnost"] = vect_deals.at(6);
            command["departure_kg"] = vect_deals.at(7);
            command["price_tn"] = vect_deals.at(10); // заполняем цену ПРОДАЖИ В ТОННАХ
        }

        if (!AddRowSQL("storages", command)) {
            qDebug() << "ERROR AddStorageDate command";
        }

        UpdateListStorage();

        return true;
    }

    return false;
}

bool MainWindow::UpdateStorageLine(const QVector <QString>& vect_deals, int column, QString& new_text, const QString& id_string) {
    QVector <QString> vect {"date_of_deal", "customer", "number_1c", "postavshik", "neftebaza", "tovar_short_name", "litres", "plotnost", "ves", "price_in_tn",
                          "price_out_tn", "price_out_litres", "transp_cost_tn", "commission", "rentab_tn", "profit", "manager", "id"};
    const QString old_item = vect_deals.at(column);
    QString command_store;
    command_store = "UPDATE storages SET ";
    if (column == 0) { //date_of_deal
        command_store += vect.at(0) + " = '"+ new_text + "' WHERE main_table_id = " + id_string;
    }
    else if (column == 1) {
        // если в старом значении есть НБ, а в новом нет -  либо удалить запись  прихода со склада
        //либо заменить на списание со склада
        if (old_item.indexOf("НБ") == 0) {
            if (new_text.indexOf("НБ") == 0) { // приход с одной базы перекидывается на другую
                command_store += "storage_name";
            }
            else {  // ошибка строки??? удалить приход???
                // есть ли в поставщике НБ (списание со склада) -
                // если нет -просто удалить запись со склада - транзитная сделка
                if (vect_deals.at(3).indexOf("НБ") != 0) {
                    ExecuteSQL("DELETE FROM storages WHERE main_table_id = " + id_string);
                    command_store += "operation";
                }
                else { //если есть списание со склада  -- операция из перемещени с базы на базу превращается в списание с базы
                    // две записи по складу (приход и списание) - удаляем приход
                    ExecuteSQL("DELETE FROM storages WHERE main_table_id = " + id_string + "AND customer =" + vect_deals.at(1));
                    command_store += "operation";
                }
            }
        }
        // если нет НБ ни в старом ни в новом - просто поменять клиента
        else if (old_item.indexOf("НБ") != 0 && new_text.indexOf("НБ") != 0) {
            command_store += "operation";
        }
        // если в старом нет НБ а в новом есть
        else if (old_item.indexOf("НБ") != 0 && new_text.indexOf("НБ") == 0) {

            if (vect_deals.at(4).indexOf("НБ") != 0) {

            }
            else {
                QMessageBox::critical(0, "Ошибка значения",
                                      "Удалите и создайте новую строку, так менять нельзя", QMessageBox::Cancel);
                return false;
            }
        }
        else {
            QMessageBox::critical(0, "Ошибка значения",
                                  "Неизвестная ошибка", QMessageBox::Cancel);
            return false;
        }

        command_store += "'"+ new_text + "' WHERE main_table_id = " + id_string;

    }
    else if (column == 3) {
        // если НБ нет в старом но есть в новом
        // если перемещение со склада на склад - сделать две записи на склад
        if (old_item.indexOf("НБ") != 0 && new_text.indexOf("НБ") == 0) {
            if (vect_deals.at(1).indexOf("НБ") == 0) { // был приход на склад, перемещение стало
                //приход старый обновить operation на НБ_назв_склада
                ExecuteSQL("UPDATE storages SET operation = НБ_" + vect_deals.at(4));
                //добавить списание со второго склада
                QVector <QString> val {vect_deals.at(0), new_text, ("НБ_" + vect_deals.at(4)), };
                ExecuteSQL("INSERT INTO storages (date_of_deal,  operation, storage_name, tovar_short_name, arrival_doc, "
                           "departure_litres, plotnost, departure_kg, price_tn, main_table_id) VALUES " + ValuesString(val));
            }
        }

        command_store += "operation = '" + QString {new_text + " " + vect_deals.at(4) + "' "};
        command_store += "WHERE main_table_id = " + id_string;
    }
    else if (column == 4) {
        if (new_text.indexOf("НБ") != 0 && old_item.indexOf("НБ") != 0) {
            // если не склады - просто поменять operation поставщик+место
            //узнать поствщик из deals SELECT postavshik FROM deals WHERE id = id_string
            command_store += "operation ='" + vect_deals.at(3) + " " + new_text + "' WHERE main_table_id = " + id_string;
        }
        else {
            // операция перемещения с базы на базу ??? две операции должны быть
            //?? пока выдать критикал

            if (vect_deals.at(1).indexOf("НБ") == 0)  {
                QMessageBox::critical(0, "Ошибка значения",
                                      "Надо решить эту проблему", QMessageBox::Cancel);
                return false;
            }
        }
    }
    else if (column == 5) { // имя товара
        command_store += vect.at(5) + " = '"+ new_text + "' WHERE main_table_id = " + id_string;
    }
    else if (column == 6) {
        command_store += "departure_litres = '" + new_text + "' WHERE main_table_id = " + id_string;
    }
    else if (column == 7) {  // plotnost
        // заменить запятую на точку
        command_store += vect.at(7) + " = '"+ new_text + "' WHERE main_table_id = "+ id_string;
    }
    else if (column == 8) {
        command_store += "departure_kg = '" + new_text+"' WHERE main_table_id = " + id_string;
    }
    else if (column == 9) {
        command_store += "price_tn = '"+ new_text + "' WHERE main_table_id = " + id_string;
    }
    else { // изменяется колонка, которая не учитываются в складах
        return false;
    }
    //выполнить command_store
    ExecuteSQL(command_store);
    return true;
}



void MainWindow::on_pushButton_deals_clicked()
{
    ui->buttons_action->setVisible(true);

    model =  new QStandardItemModel(10, 18);
    // установка заголовков таблицы

    model->setHeaderData(0, Qt::Horizontal, "Дата");
    model->setHeaderData(1, Qt::Horizontal, "Клиент");
    model->setHeaderData(2, Qt::Horizontal, "Номер УПД");
    model->setHeaderData(3, Qt::Horizontal, "Поставщик");
    model->setHeaderData(4, Qt::Horizontal, "Нефтебаза");
    model->setHeaderData(5, Qt::Horizontal, "Товар");
    model->setHeaderData(6, Qt::Horizontal, "Объем, л");
    model->setHeaderData(7, Qt::Horizontal, "Плотность");
    model->setHeaderData(8, Qt::Horizontal, "Вес, кг");
    model->setHeaderData(9, Qt::Horizontal, "Цена входа, р\\тн");
    model->setHeaderData(10, Qt::Horizontal, "Цена продажи, р\\тн");
    model->setHeaderData(11, Qt::Horizontal, "Цена продажи, р\\л");
    model->setHeaderData(12, Qt::Horizontal, "Трансп затраты, р\\тн");
    model->setHeaderData(13, Qt::Horizontal, "Комиссии");
    model->setHeaderData(14, Qt::Horizontal, "Прибыль на тонну");
    model->setHeaderData(15, Qt::Horizontal, "Прибыль сумма");
    model->setHeaderData(16, Qt::Horizontal, "Менеджер");
    model->setHeaderData(17, Qt::Horizontal, "ID");

    if (auto query = ExecuteSQL("SELECT date_of_deal, customer, number_1c, postavshik, neftebaza, "
                            "tovar_short_name, litres, plotnost, ves, price_in_tn, price_out_tn, "
                            "price_out_litres, transp_cost_tn, commission, rentab_tn, profit,manager, "
                                "id FROM deals ORDER BY date_of_deal")) {
        int row_count = 0;
        while(query.value().next()){
            for (auto i = 0; i < 18; ++i) {
                model->setItem(row_count, i, new QStandardItem(query.value().value(i).toString()));
            }
            ++row_count;
        }
    }

    QObject::connect(model, &QStandardItemModel::itemChanged,
                     [&](QStandardItem *item) {
                         qDebug() << "Item changed:" << item->index().row() << item->index().column() << item->text();
                        QVector <QString> vect {"date_of_deal", "customer", "number_1c", "postavshik", "neftebaza", "tovar_short_name", "litres", "plotnost", "ves", "price_in_tn",
                                                "price_out_tn", "price_out_litres", "transp_cost_tn", "commission", "rentab_tn", "profit", "manager", "id"};
                        int column = item->index().column();
                        int row =    item->index().row();
                        QString table_name {"deals"};
                        int id_number_column = 17;
                        QString id_string = FindID(row, id_number_column).toString();

                        QString new_text = item->text();
                        if (vect.value(column) == "plotnost") {
                            new_text.replace(',', '.'); // убрать запятую если плотность
                        }

                        QString qout =  {"UPDATE " + table_name + " SET " + vect.value(column) + " = '" + new_text + "' WHERE id = " + id_string};
                        ExecuteSQL(qout);
                        StorageAdding(id_string, new_text);
                        on_pushButton_deals_clicked();
                    });


    ui->tableView->setWordWrap(1);
    ui->tableView->setModel(model); //устанавливает перенос слов
    ui->tableView->resizeColumnsToContents(); // адаптирует размер всех столбцов к содержимому

    //прокрутка вниз влево
    QModelIndex bottomLeft = model->index(model-> rowCount() - 1, 0);
    ui->tableView->scrollTo(bottomLeft);
    connect(ui->tableView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this,
            SLOT(tableSelectionChanged(QItemSelection, QItemSelection)));
    // connect(
    //     table->selectionModel(),
    //     SIGNAL(selectionChanged(const QItemSelection & selected, const QItemSelection & deselected)),
    //     this,
    //     SLOT(slotLoadTransaction(const QItemSelection & selected, const QItemSelection & deselected))
    //     );

    UpdateListStorage();

}

void MainWindow::tableSelectionChanged(QItemSelection, QItemSelection) {
    auto listindexes = ui->tableView->selectionModel()->selectedIndexes();
    index_set_rows.clear();
    for (const auto& index : listindexes) {
        index_set_rows.insert(index.row());
    }
    //std::sort(index_set_rows.begin(), index_set_rows.end());
    qDebug() << "Row ";
    for (auto& row_i : index_set_rows) {
        qDebug() << row_i;
    }
}

// void MainWindow::on_tableView_clicked(const QModelIndex &index)
// {
//     index_buffer_ = index.row();

// }

void MainWindow::on_pushButton_copy_clicked()
{
    if (index_set_rows.empty()) {
        QMessageBox::critical(this, "НЕ СКОПИРОВАНО", "Выделите нужные строки и нажмите Скопировать!");
        return;
    }
    else {

        index_set_rows_copy = index_set_rows;

    }
}


void MainWindow::on_pushButton_paste_clicked()
{
    if (index_set_rows_copy.empty()) {
        QMessageBox::critical(this, "НЕ СКОПИРОВАНО", "Выделите нужные строки и нажмите Скопировать!");
        return;
    }
    //bool begin = true;
    for (const auto& row : index_set_rows_copy) {
        //auto row = index_for_copy_;
        //нашли ID копируемого объекта
        //auto id_copy = FindID (index_for_copy->row(), index_for_copy->column()).toString();
        QMap<QString, QString> date;
        QVector <QString> vect_names {"date_of_deal", "customer", "number_1c", "postavshik", "neftebaza", "tovar_short_name", "litres", "plotnost", "ves", "price_in_tn",
                                    "price_out_tn", "price_out_litres", "transp_cost_tn", "commission", "rentab_tn", "profit", "manager"};


        // копируем всё кроме id
        for (auto column = 0; column < model->columnCount()-1; ++column)
        {
            date [vect_names[column]] = model->item(row, column)->data(Qt::DisplayRole).toString();

        }

        if (*index_set_rows.begin()  != *index_set_rows_copy.begin()) {
            if (model->item(*index_set_rows.begin(), 0) != nullptr) { // если ячейка не пустая
                date["date_of_deal"] = model->item(*index_set_rows.begin(), 0)->data(Qt::DisplayRole).toString();
            }
            else {
                date["date_of_deal"] = GetCurrentDate();
            }
        }
        auto id_num = AddRowSQLString ("deals", date);
        QString temp ={};
        StorageAdding(id_num, temp);
        on_pushButton_deals_clicked();
        //index_buffer_ = -1; // сброс буфера
        //begin = false;
    }
}


void MainWindow::on_pushButton_delete_clicked()
{
    if (index_set_rows.empty()) {
        QMessageBox::critical(this, "НЕ УДАЛЕНО", "Выделите любую ячейку в удаляемой строке и нажмите УДАЛИТЬ!");
        return;
    }
    for (auto& row : index_set_rows) {
        auto id = FindID (row, 17).toString();
        QString command = "DELETE FROM deals WHERE id = ";
        command += "'" + id + "'";
        QSqlQuery change_query = QSqlQuery(db_);
        if (!change_query.exec( command )) {
            qDebug() << change_query.lastError().databaseText();
            qDebug() << change_query.lastError().driverText();
            return;
        }
        //Удалить старые данные из storages привязанные к ID основной таблицы
        DeleteFromSQL("storages", id);
    }
    on_pushButton_deals_clicked();
}

QString MainWindow::GetCurrentDate () {
    QString command = "SELECT CURRENT_DATE";
    QString date;
    if (auto query = ExecuteSQL (command)) {
        if (query.value().first()) {
            qDebug() << query.value().value(0).toString();
            date = query.value().value(0).toString();
        }
    }
    return date;
}

void MainWindow::on_pushButton_new_clicked()
{

    //QString date = GetCurrentDate ();
    ExecuteSQL (QString{"INSERT INTO deals (date_of_deal) VALUES ('"+ GetCurrentDate () + "')"});
    on_pushButton_deals_clicked();
}


void MainWindow::on_action_triggered()
{

    QApplication::quit();

}


void MainWindow::on_action_hovered()
{
    ui->statusbar->showMessage("Выйти из программы");
}


void MainWindow::on_action_2_triggered()
{
    ui->statusbar->showMessage("Настройки нажаты");
}


void MainWindow::on_action_2_hovered()
{
     ui->statusbar->showMessage("Настройки выделены");
}


void MainWindow::on_action_2_changed()
{
    ui->statusbar->showMessage("Настройки выделены changed");
}


void MainWindow::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    auto store = item->text();
    ui->statusbar->showMessage(store);
    ShowStorages(store);
    ui->buttons_action->setVisible(false);
}

void MainWindow::ShowStorages(const QString& store) {
    if (!db_.open()) {// Если соединение с базой данных не удастся, оно появится
        QMessageBox::critical(0, "Cannot open database", "Unable to establish a database connection", QMessageBox::Cancel);
        return;
    }

    QString command;
    if (store != "_Все склады") {
        command = "SELECT date_of_deal, operation, start_balance, arrival_doc, arrival_fact, departure_litres,plotnost,departure_kg,balance_end,nedoliv,price_tn,rjd_number,storage_name, id, main_table_id, tovar_short_name FROM storages WHERE storage_name = '";
        command += store + "' ORDER BY date_of_deal";
    } else {
        command = "SELECT date_of_deal, operation, start_balance, arrival_doc, arrival_fact, departure_litres,plotnost,departure_kg,balance_end,nedoliv,price_tn,rjd_number,storage_name, id, main_table_id, tovar_short_name FROM storages ORDER BY date_of_deal";
    }

    //QStandardItemModel* model=  new QStandardItemModel(50, 18);
    model_storages_ =  new QStandardItemModel(10, 16);
    // установка заголовков таблицы

    model_storages_->setHeaderData(0, Qt::Horizontal, "Дата");
    model_storages_->setHeaderData(1, Qt::Horizontal, "Операция");
    model_storages_->setHeaderData(2, Qt::Horizontal, "Сальдо на начало дня");
    model_storages_->setHeaderData(3, Qt::Horizontal, "Приход по документам");
    model_storages_->setHeaderData(4, Qt::Horizontal, "Приход фактический");
    model_storages_->setHeaderData(5, Qt::Horizontal, "Отгружено, литры");
    model_storages_->setHeaderData(6, Qt::Horizontal, "Плотность");
    model_storages_->setHeaderData(7, Qt::Horizontal, "Отгружено, кг");
    model_storages_->setHeaderData(8, Qt::Horizontal, "Сальдо на конец дня");
    model_storages_->setHeaderData(9, Qt::Horizontal, "Недолив, кг");
    model_storages_->setHeaderData(10, Qt::Horizontal, "Цена, р\\тн");
    model_storages_->setHeaderData(11, Qt::Horizontal, "Номер вагона");
    model_storages_->setHeaderData(12, Qt::Horizontal, "Название Склада");
    model_storages_->setHeaderData(13, Qt::Horizontal, "ID");
    model_storages_->setHeaderData(14, Qt::Horizontal, "Основание (ID основной таблицы)");
    model_storages_->setHeaderData(15, Qt::Horizontal, "Название товара");

    int row_count = 0;

    auto query_storages = ExecuteSQL(command);

    while(query_storages.value().next()){

        for (auto i = 0; i < 16; ++i) {
            model_storages_->setItem(row_count, i, new QStandardItem(query_storages.value().value(i).toString()));

        }
        ++row_count;
    }
    ui->tableView->setWordWrap(1); //устанавливает перенос слов
    ui->tableView->resizeColumnsToContents(); // адаптирует размер всех столбцов к содержимому

    ui->tableView->setModel(model_storages_);

    //прокрутка вниз влево
    QModelIndex bottomLeft = model_storages_->index(model_storages_-> rowCount() - 1, 0);
    ui->tableView->scrollTo(bottomLeft);

}



