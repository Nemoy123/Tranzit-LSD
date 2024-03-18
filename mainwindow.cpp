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
#include "settingwindow.h"


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
            command["start_balance"] = "0"; // заполняем начальное сальдо \\\\\\\\\\потом переделать расчет\\\\\\\\\\\\\\
            //QString sum = QString::number(command["start_balance"].toDouble() + vect_deals.at(7).toDouble());
            command["balance_end"] = QString::number(command["start_balance"].toDouble() + vect_deals.at(7).toDouble()); // заполняем конечное сальдо

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
                date["start_balance"] = "0"; // заполняем начальное сальдо \\\\\\\\\\потом переделать расчет\\\\\\\\\\\\\\
                //QString sum = QString::number(date["start_balance"].toDouble() - date["departure_kg"].toDouble());
                date["balance_end"] = QString::number(date["start_balance"].toDouble() - date["departure_kg"].toDouble()); // заполняем конечное сальдо
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
            command["start_balance"] = "0"; // заполняем начальное сальдо \\\\\\\\\\потом переделать расчет\\\\\\\\\\\\\\
            //QString sum = command["start_balance"].toInt() - command["departure_kg"].toInt();
            QString sum = QString::number(command["start_balance"].toDouble() - command["departure_kg"].toDouble());
            command["balance_end"] = QString::number(command["start_balance"].toDouble() - command["departure_kg"].toDouble()); // заполняем конечное сальдо
        }

        if (!AddRowSQL("storages", command)) {
            qDebug() << "ERROR AddStorageDate command";
        }

        UpdateListStorage();

        return true;
    }

    return false;
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
                                "id FROM deals ORDER BY date_of_deal, id")) {
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

    UpdateListStorage();

}

void MainWindow::tableSelectionChanged(QItemSelection, QItemSelection) {
    auto listindexes = ui->tableView->selectionModel()->selectedIndexes();
    index_set_rows.clear();
    for (const auto& index : listindexes) {
        index_set_rows.insert(index.row());
    }
}



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
            //qDebug() << query.value().value(0).toString();
            date = query.value().value(0).toString();
        }
    }
    return date;
}

void MainWindow::on_pushButton_new_clicked()
{
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


    // вывести строку с названием товара
    // вывести таблицу 1
    // вывести пустую строку
     // сделать выборку SQL по наименованию товара
    QString goods_name_command;
    QString command_storages;
    QString command;
    std::set <QString> all_storages;
    if (store != "_Все склады") {
        goods_name_command = "SELECT DISTINCT tovar_short_name FROM storages WHERE storage_name = '" + store + "'";
        all_storages.insert(store);
    } else {
        command_storages = "SELECT DISTINCT storage_name FROM storages";
        goods_name_command = "SELECT DISTINCT tovar_short_name FROM storages";

        // сделать std::set складов

            auto storages_query = ExecuteSQL(command_storages);
            while(storages_query.value().next()){
                all_storages.insert(storages_query.value().value(0).toString());
            }

    }
    // сделать std::set наименований товара
    std::set <QString> goods_name_set;
    auto query_goods_name = ExecuteSQL(goods_name_command);
    while(query_goods_name.value().next()){
        goods_name_set.insert(query_goods_name.value().value(0).toString());
    }



    model_storages_ =  new QStandardItemModel(10, 16);
    // установка заголовков таблицы

    model_storages_->setHeaderData(0, Qt::Horizontal, "Дата");
    model_storages_->setHeaderData(1, Qt::Horizontal, "Операция");
    model_storages_->setHeaderData(2, Qt::Horizontal, "Сальдо нач");
    model_storages_->setHeaderData(3, Qt::Horizontal, "Приход док");
    model_storages_->setHeaderData(4, Qt::Horizontal, "Приход факт");
    model_storages_->setHeaderData(5, Qt::Horizontal, "Объем,л");
    model_storages_->setHeaderData(6, Qt::Horizontal, "Плотность");
    model_storages_->setHeaderData(7, Qt::Horizontal, "Масса,кг");
    model_storages_->setHeaderData(8, Qt::Horizontal, "Сальдо новое");
    model_storages_->setHeaderData(9, Qt::Horizontal, "Недолив, кг");
    model_storages_->setHeaderData(10, Qt::Horizontal, "Цена, р\\тн");
    model_storages_->setHeaderData(11, Qt::Horizontal, "Номер вагона");
    model_storages_->setHeaderData(12, Qt::Horizontal, "Склад");
    model_storages_->setHeaderData(13, Qt::Horizontal, "ID");
    model_storages_->setHeaderData(14, Qt::Horizontal, "ID Сделки");
    model_storages_->setHeaderData(15, Qt::Horizontal, "Товара");

    int row_count = 0;

    for (const auto& storage_one : all_storages) { // перебор складов

        if (row_count != 0) ++row_count; // пропуск строчки
        model_storages_->setItem(row_count++, 0, new QStandardItem("Склад: " + storage_one)); // название склада
        ++row_count; // пропуск строчки

        for (const auto& product : goods_name_set) { // перебор товаров

            //++row_count; // пропуск строчки
            model_storages_->setItem(row_count++, 0, new QStandardItem("Товар: " + product)); // название склада


                command = "SELECT date_of_deal, operation, start_balance, arrival_doc, arrival_fact, departure_litres,plotnost,"
                          "departure_kg,balance_end,nedoliv,price_tn,rjd_number,storage_name, id, "
                          "main_table_id, tovar_short_name FROM storages WHERE storage_name = '";
                command += storage_one + "' AND tovar_short_name = '" + product + "' ORDER BY date_of_deal, id";


            auto query_storages = ExecuteSQL(command);

            while(query_storages.value().next()){

                for (auto i = 0; i < 16; ++i) {
                    model_storages_->setItem(row_count, i, new QStandardItem(query_storages.value().value(i).toString()));

                }
                ++row_count;
            }
        }

        ++row_count; // пропуск строчки
    }



    ui->tableView->setModel(model_storages_);
    ui->tableView->setWordWrap(1); //устанавливает перенос слов
    ui->tableView->resizeColumnsToContents(); // адаптирует размер всех столбцов к содержимому
    //ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers); // запрет редактирования

    //прокрутка вниз влево
    QModelIndex bottomLeft = model_storages_->index(model_storages_-> rowCount() - 1, 0);
    ui->tableView->scrollTo(bottomLeft);

}




void MainWindow::on_settings_triggered()
{
    SettingWindow setting;
    setting.setModal(true);
    setting.exec();
}


void MainWindow::on_exit_triggered()
{

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

    }
}

