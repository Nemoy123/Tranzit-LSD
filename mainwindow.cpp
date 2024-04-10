#include "mainwindow.h"
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
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QFont>
#include <QScrollBar>

ComboBoxDelegate::ComboBoxDelegate(QObject *parent, std::vector < std::vector<QString> >* vect,  std::map <QString, QString>* filter_deals )
    : QItemDelegate(parent)
    , vect_(vect)
    , filter_deals_(filter_deals)
    {

    }

QWidget* ComboBoxDelegate::createEditor(QWidget* parent,
                                        const QStyleOptionViewItem &/* option */,
                                        const QModelIndex & index ) const
{
    QComboBox *editor = new QComboBox(parent);

    editor->addItem("Все");
    if (vect_ != nullptr) {
        for (const auto& item: (*vect_).at(index.column())) {
                   editor->addItem(item);
        }
    } else {
        qDebug() << "Error combobox delegate";
    }
    //editor->setCurrentIndex(editor->findText("Все"));

    QComboBox *cBox = static_cast<QComboBox*>(editor);
    cBox->setCurrentIndex(cBox->findText("Все"));
    //ui->comboBox_filter->setCurrentIndex(ui->comboBox_filter->findText(last_storage_filter_));

    editor->setEditable(false);
    return editor;
}

void ComboBoxDelegate::setEditorData(QWidget *editor,
                                     const QModelIndex &index) const
{
    QString value = index.model()->data(index, Qt::EditRole).toString();

    QComboBox *cBox = static_cast<QComboBox*>(editor);
    cBox->setCurrentIndex(cBox->findText(value));
}

void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    QComboBox *cBox = static_cast<QComboBox*>(editor);
    QString value = cBox->currentText();
    if (!value.isEmpty()) {
        model->setData(index, value, Qt::EditRole);

    }
}
void ComboBoxDelegate::paint (QPainter* painter, const QStyleOptionViewItem& option,
                                const QModelIndex& index) const
{


    QStyleOptionComboBox box;
    box.state = option.state;

    box.rect = option.rect;

    QVector <QString> vect {"date_of_deal", "customer", "number_1c", "postavshik", "neftebaza", "tovar_short_name", "litres", "plotnost", "ves", "price_in_tn",
                          "price_out_tn", "price_out_litres", "transp_cost_tn", "commission", "rentab_tn", "profit", "manager", "id"};
    int column = index.column();

    if (filter_deals_->find(vect.at(column)) != (*filter_deals_).end()) {
        box.currentText = (*filter_deals_).at( vect.at(column) );
    } else {
        box.currentText = "Все";
    }

    QApplication::style()->drawComplexControl(QStyle::CC_ComboBox, &box, painter, 0);
    QApplication::style()->drawControl(QStyle::CE_ComboBoxLabel, &box, painter, 0);
    return;

   // QStyledItemDelegate::paint(painter, option, index);
}
void ComboBoxDelegate::updateEditorGeometry(QWidget *editor,
                                            const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

void Encdec::encrypt(const QString& in) {
    QString res{};
    for (const QChar& ch : in) {
        res += QChar(ch.unicode() + key);;
    }

    QFile file(*Encdec::GetName());

    if (file.open(QIODevice::ReadWrite)) {
        file.resize(0); // очистить файл
        QDataStream stream(&file);
        stream << res;
        file.close();
    }
}

// Definition of decryption function
QString Encdec::decrypt()
{
    QFile file(*Encdec::GetName());
    QString res{};
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream stream(&file);
        stream >> res;
        file.close();
    }
    QString out {};
    for (const QChar& ch : res) {
        out += QChar(ch.unicode() - key);
    }
    return out;
}


bool MainWindow::LoadConfig () {
    // проверяем наличие файла и что это именно файл а не ссылка
    QFileInfo check_file(setting_file_);
    // check if file exists and if yes: Is it really a file and no directory?
    if (check_file.exists() && check_file.isFile()) {
        qDebug() << "Есть файл";

        QFile file(setting_file_);
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream streamstring(&file);

            QString text = cl_enc.decrypt();

            QStringList list = text.split("?");
            QString font_name{};
            QString font_size{};

            int counter = 0;
            for (const auto& item : list) {
                if (counter == 0) { server_ = item;}
                else if (counter == 1) { port_ = item.toInt();}
                else if (counter == 2) { base_name_ = item;}
                else if (counter == 3) { login_ = item;}
                else if (counter == 4) { pass_ = item;}
                else if (counter == 5) { font_name = item;}
                else if (counter == 6) { font_size = item;}

                ++counter;
            }
            if (!font_name.isEmpty() && !font_size.isEmpty()) {
                current_table_view_font = QFont (font_name, font_size.toInt());
            }

            file.close();
            return true;
        }
    }
    qDebug() << "Нет файла или файл не открылся";
    return false;
}

bool MainWindow::createConnection() {
    if (!LoadConfig ()) {
        QString buffer{};
        QTextStream stream(&buffer);
        stream << server_<<"?"<<port_<<"?"<<base_name_<<"?"<<login_<<"?"<<pass_<<"?";
        cl_enc.encrypt(stream.readAll());
    }
    db_ = QSqlDatabase::addDatabase("QPSQL", "connection1");
    db_.setHostName(server_);
    db_.setPort(port_);
    db_.setDatabaseName(base_name_);
    db_.setUserName(login_);
    db_.setPassword(pass_);

    if (!db_.open()) {// Если соединение с базой данных не удастся, оно появится
        //critical(QWidget *parent, const QString &title,
        //const QString &text,
        //QMessageBox::StandardButtons buttons = Ok,
        //QMessageBox::StandardButton defaultButton = NoButton)
        QMessageBox::critical(0, "Cannot open database",
                              "Unable to establish a database connection: " + db_.lastError().text(), QMessageBox::Cancel);
        return false;
    }
    return true;
}

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

QMap<QString, QString> MainWindow::CheckDealsParam (const QString& id_deals) {
    const QVector <QString> vect {"date_of_deal", "customer", "number_1c", "postavshik", "neftebaza", "tovar_short_name", "litres", "plotnost", "ves", "price_in_tn",
                          "price_out_tn", "price_out_litres", "transp_cost_tn", "commission", "rentab_tn", "profit", "manager"};
    QMap<QString, QString> date;
    date["id"] = id_deals;
    QString command = "SELECT date_of_deal, customer, number_1c, postavshik, neftebaza, tovar_short_name, litres, plotnost, ves, price_in_tn, "
                      "price_out_tn, price_out_litres, transp_cost_tn, commission, rentab_tn, profit, manager "
                      "FROM deals WHERE id = '" + id_deals + "'";

    if (auto query = ExecuteSQL(command)) {
        if(query.value().next()) {
            for (int i = 0; i < vect.size(); ++i) {
                date[vect.at(i)] = query.value().value(i).toString();
            }
        }
    }
    return CheckDealsParam (date);


}

QString MainWindow::FindNextStorageIdFromIdDeals(const QString &id_deals)
{
    QString command = "SELECT id FROM storages "
                      "WHERE main_table_id = '" + id_deals + "' ORDER BY date_of_deal ASC, id ASC LIMIT 1";

    QString check_id;
    if (auto query = ExecuteSQL(command)) {
        if (query.value().next()) {
            check_id = query.value().value(0).toString();
        }
    }
    if (check_id.isEmpty()) {
       return "";
    }



    QString result_id = FindNextOrPrevIdFromStorage (check_id, "next");
    if (result_id == "-1") {
        return "";
    }
    // проверяем что у result_id не совпадает main_table_id
    command = "SELECT main_table_id FROM storages WHERE id = '" + result_id + "'";
    if (auto query = ExecuteSQL(command)) {
        if (query.value().next()) {
            if ( query.value().value(0).toString() != id_deals) {
                return result_id;
            }
            else {
                result_id = FindNextOrPrevIdFromStorage (result_id, "next");
                command = "SELECT main_table_id FROM storages WHERE id = '" + result_id + "'";
                if (auto query = ExecuteSQL(command)) {
                    if ( query.value().value(0).toString() != id_deals) {
                        return result_id;
                    }
                }
            }
        }
    }
    return "";


}

QString MainWindow::FindDateFromIdDeals(const QString &id_deals)
{
    QString command = "SELECT date_of_deal FROM deals WHERE id = '"+ id_deals +"'";
    if (auto query = ExecuteSQL(command)) {
        if(query.value().next()) {
            return query.value().value(0).toString();
        }
    } else {
        qDebug() << "Ошибка";
        return "-1";
    }
    return "-1";
}




QMap<QString, QString> MainWindow::CheckDealsParam (QMap<QString, QString>& date) {

    if (date.contains("price_out_litres") && date.contains("plotnost")) {
        double price_lt =0;
        price_lt = date.value("price_out_litres").toDouble(); // если цена за литр не равна 0
        if (price_lt != 0 && date.value("plotnost").toDouble() != 0) {
            double price_tn = price_lt / date.value("plotnost").toDouble() * 1000;
            price_tn = round(price_tn*100)/100;
            date["price_out_tn"] = QString::number(price_tn);
        }
    }
    if (date.contains("litres") && date.contains("plotnost") && date.contains("ves")) {
        double lit = date.value("litres").toDouble();
        double plot = date.value("plotnost").toDouble();
        double ves = date.value("ves").toDouble();
        if (lit == 0 && plot != 0 && ves != 0) {
            lit = ves / plot;
            date["litres"] = QString::number(lit,'f',2);
        }
        else if (lit != 0 && plot == 0 && ves != 0) {
            plot = ves / lit;
            date["plotnost"] = QString::number(plot,'f',3);
        }
        else if (lit != 0 && plot != 0 && ves == 0) {
            ves = lit * plot;
            date["ves"] = QString::number(qRound(ves));
        }
        else if (lit != 0 && plot != 0 && ves != 0) {
            if (ves != lit * plot) {
                ves = lit * plot;
                date["ves"] = QString::number(qRound(ves));
            }
        }

    }


    // проверить если приход на базу далее не выполнять
    if (date.contains("id") && !date.contains("customer")) {
        QString command = "SELECT customer FROM deals WHERE id = '" + date.value("id") + "'";
        if (auto query = ExecuteSQL(command)) {
            if(query.value().next()) {
                if (query.value().value(0).toString().indexOf("НБ") == 0) {
                    date["rentab_tn"] = "0";
                    date["profit"] = "0";
                    return date;
                }
            }
        }
    }
    else if (date.contains("customer")) {
        if (date.value("customer").indexOf("НБ") == 0) {
            date["rentab_tn"] = "0";
            date["profit"] = "0";
            return date;
        }
    }


    double mass = 0;
    mass = date.value("ves").toDouble(); // вес
    if (mass != 0) { // проверить деление на ноль
        double price_tn_prod = date.value("price_out_tn").toDouble(); // цена продажи тонна
        double price_tn_vhod = date.value("price_in_tn").toDouble(); // цена покупки тонна

        double commission = date.value("comission").toDouble(); // комиссионные всего, не за тонну
        double transport = date.value("transp_cost_tn").toDouble(); // транспорт за рейс
        double rentab = (price_tn_prod - price_tn_vhod) - (transport / (mass/1000)) - commission;
        rentab = round(rentab*100)/100;
        double summ_rentab = rentab * (mass/1000);
        date["rentab_tn"] = QString::number(rentab);
        date["profit"] = QString::number(summ_rentab);
    }
    return date;
}

QString MainWindow::AddRowSQLString (const QString& storage, QMap<QString, QString>& date_){
    if (storage == "deals") {
        date_ = CheckDealsParam (date_);
    }
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

bool MainWindow::AddRowSQL (const QString& storage, QMap<QString, QString>& date_) {

    return ( AddRowSQLString (storage, date_) != "-1");
}

bool MainWindow::DeleteFromSQL (const QString& storage, const QString& main_table_id) {
    QString command = "DELETE FROM ";
    command += storage + " WHERE main_table_id = '" + main_table_id + "'";
    auto query = ExecuteSQL(command);
    if (query->isActive()) { // isActive() вернет true если запрос SQL успешен
        return true;
    }
    return false;
}

MainWindow::MainWindow( QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , cl_enc(&setting_file_)
{


    ui->setupUi(this);

    createConnection();
    //if(!createConnection()) return 1;// Ситуацию возврата можно заменить в зависимости от разных ситуаций
    // Указываем подключение к базе данных
    db_ = QSqlDatabase::database("connection1");

    UpdateListStorage();
    on_pushButton_deals_clicked();
}

MainWindow::~MainWindow()
{
    delete ui;
}

QVariant MainWindow::FindID (int row, int column) {
    QModelIndex index_id = model->index(row, column);
    QVariant data = model->data(index_id);
    //qDebug() << data;
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

void MainWindow::UpdateSQLString (const QString& storage, const QMap<QString, QString>& date){

    QString command = "UPDATE " + storage + " SET ";
    bool begin = true;
    for (auto i = date.cbegin(), end = date.cend(); i != end; ++i){
        if (!begin && i.key() != "id") {
            command += ", ";

        }

        if (i.key() != "id") {
            begin = false;
            command += i.key();
            command += " = '";
            command += i.value();
            command += "'";
            //command += i.key() + " = '" + i.value() + "'";
        }

    }
    command += " WHERE id = '" + date.value("id") + "'";

    ExecuteSQL(command);
}

void MainWindow::ChangeFutureStartSaldo (const QString& id) {
    QString command = "SELECT date_of_deal, storage_name, tovar_short_name FROM storages WHERE id = '" + id + "'";
    QMap <QString, QString> result;
    if (auto query = ExecuteSQL(command)) {
        while(query.value().next()) {
            result.insert( "date_of_deal", query.value().value(0).toString() ); // первое value это от optional
            result.insert( "storage_name", query.value().value(1).toString() );
            result.insert( "tovar_short_name", query.value().value(2).toString() );
        }
    }
    ChangeFutureStartSaldo(id, result.value("date_of_deal"), result.value("storage_name"), result.value("tovar_short_name"));

}

void MainWindow::ChangeFutureStartSaldo (const QString& id, const QString& date_of_deal,const QString& storage_name, const QString& tovar_short_name) {

    QString command = "SELECT id, arrival_doc, departure_kg FROM storages "
              "WHERE tovar_short_name = '" + tovar_short_name + "' AND storage_name = '" + storage_name
              + "' AND date_of_deal > '" + date_of_deal + "' AND id != '" + id + "' " +
              "OR tovar_short_name = '" + tovar_short_name +
              "' AND storage_name = '" + storage_name +
              "' AND date_of_deal = '" + date_of_deal + "' AND id > '" + id + "' "
              "ORDER BY date_of_deal, id";
    if (auto query = ExecuteSQL(command)) {
        while(query.value().next()) {
            QMap<QString, QString> stroke;
            stroke["id"] = query.value().value(0).toString();
            double st_b = StartingSaldo(stroke["id"]) ;
            stroke["start_balance"] = QString::number( st_b );
            stroke["arrival_doc"] = query.value().value(1).toString();
            stroke["departure_kg"] = query.value().value(2).toString();
            stroke["balance_end"] = QString::number(st_b + stroke["arrival_doc"].toDouble() - stroke["departure_kg"].toDouble());
            UpdateSQLString ("storages", stroke);
        }
    }
}

double MainWindow::StartingSaldo (const QString& date_of_deal, const QString& tovar_short_name, const QString& storage_name) {
    QString command = "SELECT date_of_deal, id, balance_end FROM storages "
                      "WHERE tovar_short_name = '" + tovar_short_name + "' AND storage_name = '" + storage_name + "' AND date_of_deal <= '";
    command += date_of_deal + "' ORDER BY date_of_deal DESC, id DESC LIMIT 1";
    // первая строки и есть искомый пассажир, так как новой строки еще нет в базе
    QVector <QString> vect;
    if (auto query = ExecuteSQL(command)) {
        while(query.value().next()) {
            for (auto i = 0; i < 3; ++i) {
                vect.push_back( query.value().value(i).toString() ); // первое value это от optional
            }
        }
    }
    if (vect.size() > 0) {
        double res = vect.at(2).toDouble();
        return res;
    }
    return 0;

}
QString MainWindow::FindNextOrPrevIdFromStorage (const QString& storage_id, const QString& operation) {
    if (operation != "next" && operation != "prev") {
        return "-1";
    }
    if (storage_id.isEmpty()) {
        return "";
    }
    QString command = "SELECT date_of_deal, tovar_short_name, storage_name FROM storages "
                      "WHERE id = '" + storage_id + "'";
    QMap<QString, QString> date;
    if (auto query = ExecuteSQL(command)) {
        while(query.value().next()) {
            date["date_of_deal"] = query.value().value(0).toString();
            date["tovar_short_name"] = query.value().value(1).toString();
            date["storage_name"] = query.value().value(2).toString();
        }
    }

    command = "SELECT id  FROM storages "
              "WHERE id != '" + storage_id + "' AND tovar_short_name = '" + date["tovar_short_name"]
                  + "' AND storage_name = '" + date["storage_name"] + "' AND date_of_deal";
    if (operation == "next") {
        command += " > ";
    }
    if (operation == "prev") {
        command += " < ";
    }
    command += "'" + date["date_of_deal"] + "' "
              "OR tovar_short_name = '" + date["tovar_short_name"] + "' AND storage_name = '" + date["storage_name"] +
               "' AND date_of_deal = '" + date["date_of_deal"] + "'" +" AND id";
    if (operation == "next") {
        command += " > ";
        command += "'" + storage_id + "' ";
        command += "ORDER BY date_of_deal ASC, id ASC LIMIT 1";

    }
    if (operation == "prev") {
        command += " < ";
        command += "'" + storage_id + "' ";
        command += "ORDER BY date_of_deal DESC, id DESC LIMIT 1";
    }

    QString result;
    if (auto query = ExecuteSQL(command)) {
        while(query.value().next()) {
            result = query.value().value(0).toString(); // первое value это от optional
            return result;
        }
    }
    return "-1";
}

QString MainWindow::FindPrevIdFromStorage (const QString& storage_id) {
    QString command = "SELECT date_of_deal, tovar_short_name, storage_name FROM storages "
                      "WHERE id = '" + storage_id + "'";
    QMap<QString, QString> date;
    if (auto query = ExecuteSQL(command)) {
        while(query.value().next()) {
            date["date_of_deal"] = query.value().value(0).toString();
            date["tovar_short_name"] = query.value().value(1).toString();
            date["storage_name"] = query.value().value(2).toString();
        }
    }
    command = "SELECT id  FROM storages "
              "WHERE id != '" + storage_id + "' AND tovar_short_name = '" + date["tovar_short_name"]
              + "' AND storage_name = '" + date["storage_name"] + "' AND date_of_deal < '" + date["date_of_deal"] + "' "
              "OR tovar_short_name = '" + date["tovar_short_name"] + "' AND storage_name = '" + date["storage_name"] +
              "' AND date_of_deal = '" + date["date_of_deal"] + "'" +" AND id < '" + storage_id + "' ";
    command += "ORDER BY date_of_deal DESC, id DESC LIMIT 1";
    QString result;
    if (auto query = ExecuteSQL(command)) {
        while(query.value().next()) {
            result = query.value().value(0).toString(); // первое value это от optional
            return result;
        }
    }
    return "-1";
}

double MainWindow::StartingSaldo (const QString& storage_id) {
    QString command = "SELECT date_of_deal, tovar_short_name, storage_name FROM storages "
                      "WHERE id = '" + storage_id + "'";
    QMap<QString, QString> date;
    if (auto query = ExecuteSQL(command)) {
        while(query.value().next()) {
            date["date_of_deal"] = query.value().value(0).toString();
            date["tovar_short_name"] = query.value().value(1).toString();
            date["storage_name"] = query.value().value(2).toString();
        }
    }
    command = "SELECT balance_end  FROM storages "
              "WHERE id != '" + storage_id + "' AND tovar_short_name = '" + date["tovar_short_name"]
              + "' AND storage_name = '" + date["storage_name"] + "' AND date_of_deal < '" + date["date_of_deal"] + "' "
              "OR tovar_short_name = '" + date["tovar_short_name"] + "' AND storage_name = '" + date["storage_name"] +
              "' AND date_of_deal = '" + date["date_of_deal"] + "'" +" AND id < '" + storage_id + "' ";
    command += "ORDER BY date_of_deal DESC, id DESC LIMIT 1";
    // первая строки и есть искомый пассажир, так как новой строки еще нет в базе
    QString result;
    if (auto query = ExecuteSQL(command)) {
        while(query.value().next()) {

            result = query.value().value(0).toString(); // первое value это от optional
            return result.toDouble();

        }
    }
    else { // если нет выше строк
        return 0;
    }

    return 0;

}

void CheckLitresPlotnost(QString& litres, QString& plotnost, QString& mass){
    double litr = litres.toDouble();
    double plot = plotnost.toDouble();
    double mas = mass.toDouble();
    if (litr != 0 && plot == 0 && mas != 0) {
        plot = mas/litr;
        plotnost = QString::number(plot);
    }
    else if (litr != 0 && plot != 0 && mas == 0) {
        mas = litr * plot;
        mass = QString::number(mas);
    }
    else if (litr == 0 && plot != 0 && mas != 0) {
        litr = mas / plot;
        litres = QString::number(litr);
    }

}

void MainWindow::UpdateAverageForLater (const QString& id) {
    QString command = "SELECT date_of_deal, storage_name, tovar_short_name FROM storages WHERE id = '" + id + "'";
    QMap <QString, QString> result;
    if (auto query = ExecuteSQL(command)) {
        while(query.value().next()) {
            result.insert( "date_of_deal", query.value().value(0).toString() ); // первое value это от optional
            result.insert( "storage_name", query.value().value(1).toString() );
            result.insert( "tovar_short_name", query.value().value(2).toString() );
        }
    }
    command = "SELECT id, main_table_id FROM storages "
                      "WHERE departure_kg > '0' AND tovar_short_name = '" + result["tovar_short_name"] + "' AND storage_name = '" + result["storage_name"]
                      + "' AND date_of_deal > '" + result["date_of_deal"] + "' "
                      "OR departure_kg > '0' AND tovar_short_name = '" + result["tovar_short_name"] +
                      "' AND storage_name = '" + result["storage_name"] +
                      "' AND date_of_deal = '" + result["date_of_deal"] + "' AND id > '" + id + "' "
                     "ORDER BY date_of_deal ASC, id ASC";
    std::vector<QString> id_for_change {};
    std::vector<QString> main_id_for_change {};
    if (auto query = ExecuteSQL(command)) {
        while(query.value().next()) {
            id_for_change.push_back(query.value().value(0).toString());
            main_id_for_change.push_back(query.value().value(1).toString());
        }
    }
    for (int i = 0; i < id_for_change.size(); ++i) {
        QString pr = QString::number( AveragePriceIn( id_for_change.at(i) ) );
        QMap<QString, QString> date{};
        date["id"] = main_id_for_change.at(i);
        date["price_in_tn"] = pr;
        UpdateSQLString("deals", date);
    }


}

bool MainWindow::StorageAdding(const QString& id_string, QString& new_text) {
    // проверить надо ли добавлять (поиск НБ в клиенте и в поставщике)
    auto query_test = ExecuteSQL(QString{"SELECT customer, postavshik FROM deals WHERE id =" + id_string});
    query_test.value().first();

    //Удалить старые данные из storages привязанные к ID основной таблицы (изменить расчет след сальдо)
    auto next_store_id_operation = FindNextStorageIdFromIdDeals(id_string);
    DeleteFromSQL("storages", id_string);
    if (!next_store_id_operation.isEmpty()) {
        double saldo = StartingSaldo (next_store_id_operation);
        QMap<QString, QString> upd_saldo;
        upd_saldo["id"] = next_store_id_operation;
        upd_saldo["start_balance"] = QString::number(saldo);
        auto query_test = ExecuteSQL(QString{"SELECT arrival_doc, departure_kg, nedoliv FROM storages WHERE id ='" + next_store_id_operation+ "'"});
        QVector<QString> vect;
        if (query_test.value().next()) {
            for (auto i = 0; i < 3; ++i) {
                vect.push_back( query_test.value().value(i).toString() ); // первое value это от optional
            }
        } else {
            vect[0] = "0";
            vect[1] = "0";
            vect[2] = "0";
        }
        upd_saldo["balance_end"] = QString::number(saldo + vect.at(0).toDouble() - vect.at(1).toDouble() - vect.at(2).toDouble());
        UpdateSQLString ("storages", upd_saldo);
        ChangeFutureStartSaldo(next_store_id_operation);
    }

    if (       query_test.value().value(0).toString().indexOf("НБ") == 0
        || query_test.value().value(1).toString().indexOf("НБ") == 0
        || new_text.indexOf("НБ") == 0
        ) {
        // получить нужные данные из deals
        QString command_to_deals = "SELECT date_of_deal, customer, postavshik, neftebaza, tovar_short_name, litres, plotnost, ves, price_in_tn, id, price_out_tn, price_out_litres FROM deals WHERE id =";
        command_to_deals += id_string;

        QVector<QString> vect_deals;
        if (auto query = ExecuteSQL(command_to_deals)) {
            while(query.value().next()) {
                for (auto i = 0; i < 12; ++i) {
                    vect_deals.push_back( query.value().value(i).toString() ); // первое value это от optional
                }
            }
        }


        //Внести новые данные
        QMap<QString, QString> command;

        command["date_of_deal"] = vect_deals.at(0); // дата
        command["tovar_short_name"] = vect_deals.at(4); // заполняем название товара
        command["main_table_id"] = vect_deals.at(9); // заполняем ИД основной таблицы
        QString id_new{};
        if (vect_deals.at(1).indexOf("НБ") == 0) { // приход на базу или перемещение между базами

            command["operation"] = vect_deals.at(2) +" "+ vect_deals.at(3); // перемещаем графу поставщик + нефтебаза
            //new_text.simplified().replace(char{32}, char{95});
            command["storage_name"] = vect_deals.at(1).simplified(); // заполняем название склада
            command["arrival_doc"] = vect_deals.at(7); // заполняем вес прихода
            command["price_tn"] = vect_deals.at(8); // заполняем цену ЗАКУПКИ
            // заполняем начальное сальдо
            double st_bal = StartingSaldo(command["date_of_deal"], command["tovar_short_name"], command["storage_name"]);
            command["start_balance"] = QString::number(st_bal);

            command["balance_end"] = QString::number(command["start_balance"].toDouble() + vect_deals.at(7).toDouble()); // заполняем конечное сальдо

            if (vect_deals.at(2).indexOf("НБ") == 0) {
                // если перемещение между складами списать со склада донора
                QMap<QString, QString> date;
                date["date_of_deal"] = vect_deals.at(0); // дата
                date["operation"] = vect_deals.at(1); // на какой склад отправляем
                date["storage_name"] = vect_deals.at(2).simplified() + "_" + vect_deals.at(3).simplified(); // название склада с кот списываем
                date["tovar_short_name"] = vect_deals.at(4); // название товара
                date["departure_litres"] = vect_deals.at(5); // заполняем литры
                date["plotnost"] = vect_deals.at(6); // заполняем плотность
                date["departure_kg"] = vect_deals.at(7); // заполняем вес отгруженного
                date["price_tn"] = vect_deals.at(8); // заполняем цену
                date["main_table_id"] = vect_deals.at(9); // заполняем ИД основной таблицы
                // заполняем начальное сальдо
                double st_bal = StartingSaldo(date["date_of_deal"], date["tovar_short_name"], date["storage_name"]);
                date["start_balance"] = QString::number(st_bal);

                date["balance_end"] = QString::number(date["start_balance"].toDouble() - date["departure_kg"].toDouble()); // заполняем конечное сальдо
                if (!AddRowSQL("storages", date)) {
                    qDebug() << "ERROR AddStorageDate date";
                }
            }
            id_new = AddRowSQLString ("storages", command);
            //UpdateAverageForLater (id_new); // обновить среднюю цену для всех отгрузок что позже


        } else { // списание со склада клиенту

            command["operation"] = vect_deals.at(1); // куда списали
            command["storage_name"] = "НБ_"+vect_deals.at(3).simplified(); // заполняем название склада
            command["departure_litres"] = vect_deals.at(5);
            command["plotnost"] = vect_deals.at(6);
            command["departure_kg"] = vect_deals.at(7);

            double st_bal = StartingSaldo(command["date_of_deal"], command["tovar_short_name"], command["storage_name"]);
            command["start_balance"] = QString::number(st_bal);
            command["balance_end"] = QString::number(command["start_balance"].toDouble() - command["departure_kg"].toDouble()); // заполняем конечное сальдо
            command["price_tn"] = vect_deals.at(10);; // заполняем цену ПРОДАЖИ В ТОННАХ

            id_new = AddRowSQLString ("storages", command);

            command.clear();
            command["id"] = id_string;
            command["price_in_tn"] = QString::number(AveragePriceIn(id_new));;
            //command.remove("price_tn");
            UpdateSQLString ("deals", command);
            //UpdateAverageForLater (id_new); // обновить среднюю цену для всех отгрузок что позже
        }
        UpdateAverageForLater (id_new); // обновить среднюю цену для всех отгрузок что позже
        if (id_new == "-1") {
            qDebug() << "ERROR AddStorageDate command";
        }
        ChangeFutureStartSaldo(id_new);
        UpdateListStorage();

        return true;
    }

    return false;
}

bool FilterEmptyChecking (const std::map <QString, QString>& filter_deals) {
    if (filter_deals.empty()) {return true;}
    for (const auto& [key, value] : filter_deals) {
        if (value != "Все" && !value.isEmpty()) {
            return false;
        }
    }
    return true;
}

void MainWindow::on_pushButton_deals_clicked()
{
    ui->filter_widget->hide();
    ui->buttons_action->setVisible(true);
    ui->comboBox_filter->addItem("Все");
    ui->tableView_header->show();

    model_header_ = new QStandardItemModel(1, 18);
    // установка заголовков таблицы
    model_header_->setHeaderData(0, Qt::Horizontal, "Дата");
    model_header_->setHeaderData(1, Qt::Horizontal, "Клиент");
    model_header_->setHeaderData(2, Qt::Horizontal, "Номер УПД");
    model_header_->setHeaderData(3, Qt::Horizontal, "Поставщик");
    model_header_->setHeaderData(4, Qt::Horizontal, "Нефтебаза");
    model_header_->setHeaderData(5, Qt::Horizontal, "Товар");
    model_header_->setHeaderData(6, Qt::Horizontal, "Объем, л");
    model_header_->setHeaderData(7, Qt::Horizontal, "Плотность");
    model_header_->setHeaderData(8, Qt::Horizontal, "Вес,\n кг");
    model_header_->setHeaderData(9, Qt::Horizontal, "Цена входа,\n р\\тн");
    model_header_->setHeaderData(10, Qt::Horizontal, "Цена продажи,\n р\\тн");
    model_header_->setHeaderData(11, Qt::Horizontal, "Цена продажи,\n р\\л");
    model_header_->setHeaderData(12, Qt::Horizontal, "Трансп затраты,\n за рейс");
    model_header_->setHeaderData(13, Qt::Horizontal, "Комиссии");
    model_header_->setHeaderData(14, Qt::Horizontal, "Прибыль\n на тонну");
    model_header_->setHeaderData(15, Qt::Horizontal, "Прибыль\n сумма");
    model_header_->setHeaderData(16, Qt::Horizontal, "Менеджер");
    model_header_->setHeaderData(17, Qt::Horizontal, "ID");


    model =  new QStandardItemModel(10, 18);



    QString command{};
    // добавить проверку фильтра

    if ( FilterEmptyChecking(filter_deals) ) {
        command = "SELECT to_char(date_of_deal, 'DD-MM-YYYY'), customer, number_1c, postavshik, neftebaza, "
                                "tovar_short_name, litres, plotnost, ves, price_in_tn, price_out_tn, "
                                "price_out_litres, transp_cost_tn, commission, rentab_tn, profit,manager, "
                           "id FROM deals ORDER BY date_of_deal, id";
    } else {
        command = "SELECT to_char(date_of_deal, 'DD-MM-YYYY'), customer, number_1c, postavshik, neftebaza, "
                          "tovar_short_name, litres, plotnost, ves, price_in_tn, price_out_tn, "
                          "price_out_litres, transp_cost_tn, commission, rentab_tn, profit,manager, "
                          "id FROM deals WHERE ";
        bool begin = true;
        for (const auto& [name, val] : filter_deals) {
            if (!begin) {command += "AND ";}
            command += name + " = '" + val + "' ";
            begin = false;
        }
        command += "ORDER BY date_of_deal ASC, id ASC";

    }
    if (std::optional<QSqlQuery> query = ExecuteSQL(command)) {
            int row_count = 0; // начинаем с 1 строки. 0 строка под комбобоксы
            while(query.value().next()){
                for (auto i = 0; i < 18; ++i) {
                    // QModelIndex ind = model->index(row_count, i);
                    // model->setData(ind, query.value().value(i).toString());
                    model->setItem(row_count, i, new QStandardItem(query.value().value(i).toString()));
                }
                ++row_count;
            }
    }
    if ( FilterEmptyChecking(filter_deals) ) {
        //std::vector < std::vector<QString> > vect_deals_filters{};
        vect_deals_filters.reserve(18);

        for (int i = 0; i < 18; ++i) {
            std::set <QString> set_date{};
            for (int y = 0; y < (model->rowCount()-1); ++y) {
                QString temp_stroke = model->item(y, i)->text();
                set_date.insert(temp_stroke);
            }
            std::vector <QString> temp (set_date.begin(), set_date.end());
            vect_deals_filters.push_back(std::move(temp));
        }
    }


    QObject::connect(model, &QStandardItemModel::itemChanged,
                     [&](QStandardItem *item) {
                         qDebug() << "Item changed:" << item->index().row() << item->index().column() << item->text();
                        QVector <QString> vect {"date_of_deal", "customer", "number_1c", "postavshik", "neftebaza", "tovar_short_name", "litres", "plotnost", "ves", "price_in_tn",
                                                "price_out_tn", "price_out_litres", "transp_cost_tn", "commission", "rentab_tn", "profit", "manager", "id"};
                        int column = item->index().column();
                        int row =    item->index().row();
                        int id_number_column = 17;
                        const QString id_string = FindID(row, id_number_column).toString();
                        QString new_text = item->text().trimmed(); // убираем пробелы в начала и конце
                        if (vect.value(column) == "plotnost" || vect.value(column) == "price_out_litres") {
                            new_text.replace(',', '.'); // убрать запятую если плотность или цена в литрах
                        }
                        QMap <QString, QString> date;
                        date ["id"] = id_string;
                        date [vect.value(column)] = new_text;
                        UpdateSQLString ("deals", date);
                        date = CheckDealsParam(id_string);
                        UpdateSQLString ("deals", date);
                        StorageAdding(id_string, new_text);
                        index_row_change_item = item->index().row();
                        on_pushButton_deals_clicked();

                    });
    QObject::connect(model_header_, &QStandardItemModel::itemChanged,
                     [&](QStandardItem *item) {
                        QVector <QString> vect {"date_of_deal", "customer", "number_1c", "postavshik", "neftebaza", "tovar_short_name", "litres", "plotnost", "ves", "price_in_tn",
                                                "price_out_tn", "price_out_litres", "transp_cost_tn", "commission", "rentab_tn", "profit", "manager", "id"};
                        int column = item->index().column();
                        //int row =    item->index().row();
                        if (item->text() != "Все") {
                            filter_deals[vect.at(column)] = item->text();
                        } else {
                            filter_deals.erase(vect.at(column));
                        }
                        on_pushButton_deals_clicked();
    });


    QObject::connect(ui->tableView->horizontalScrollBar(), &QScrollBar::sliderMoved, ui->tableView_header->horizontalScrollBar(), &QScrollBar::setValue);

    ui->tableView->setWordWrap(1); //устанавливает перенос слов
    ui->tableView->setModel(model);
    ui->tableView->setFont(current_table_view_font); // меняем шрифт

    //ui->tableView->setSortingEnabled(true);
    ui->tableView->verticalHeader()->setVisible(false); // отключить номерацию строк
    ui->tableView_header->verticalHeader()->setVisible(false); // отключить номерацию строк
    ui->tableView_header->setModel(model_header_);

    ui->tableView_header->horizontalHeader()->setFont(current_table_view_font);
    ui->tableView_header->setFont(current_table_view_font);
    ui->tableView_header->setItemDelegateForRow(0, new ComboBoxDelegate(model_header_, &vect_deals_filters, &filter_deals));


    ui->tableView->resizeColumnsToContents(); // адаптирует размер всех столбцов к содержимому
    std::vector<int> vect_main;

    for (int i = 0; i < 18; ++i) {
        vect_main.push_back(ui->tableView->columnWidth(i));
    }
    ui->tableView_header->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents); // адаптирует размер заголовка к содержимому
    std::vector<int> vect_head;
    for (int i = 0; i < 18; ++i) {
        vect_head.push_back(ui->tableView_header->columnWidth(i));

    }

    QObject::connect(ui->tableView->horizontalHeader(), &QHeaderView::sectionResized, this, &MainWindow::HorizontSizeChange);
    QObject::connect(ui->tableView_header->horizontalHeader(), &QHeaderView::sectionResized, this, &MainWindow::HorizontSizeChangeHeader);

    for (int i = 0; i < 18; ++i) {
        ui->tableView_header->setColumnWidth(i, std::max(vect_head.at(i), vect_main.at(i)));
        ui->tableView->setColumnWidth(i, std::max(vect_head.at(i), vect_main.at(i)));
    }


    if (first_launch || index_row_change_item <= 0) {
        //прокрутка вниз влево
        QModelIndex bottomLeft = model->index(model-> rowCount() - 1, 0);
        ui->tableView->scrollTo(bottomLeft);
        first_launch = false;
    }
    else {
        if (index_row_change_item > 0) {
            QModelIndex position = model->index(index_row_change_item, 0);
            ui->tableView->scrollTo(position);
            index_row_change_item = -1; //сброс индекса
        }
    }
    connect(ui->tableView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this,
            SLOT(tableSelectionChanged(QItemSelection, QItemSelection)));

    UpdateListStorage();

}


void MainWindow::tableSelectionChanged(QItemSelection, QItemSelection) {
    auto listindexes = ui->tableView->selectionModel()->selectedIndexes();
    index_set_rows.clear();
    double sum = 0;
    double sred = 0;
    int count = 0;
    for (const auto& index : listindexes) {
        index_set_rows.insert(index.row());
        bool test = false;
        index.data(Qt::DisplayRole).toDouble(&test);
        if (test) {
            sum +=  index.data(Qt::DisplayRole).toDouble();
            ++count;
            sred = sum/count;
        }
    }
    ui->statusbar->showMessage(" Сумма: " + QString::number(sum) + " Количество элементов: " + QString::number(count) + " Среднее: " + QString::number(sred));
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
        auto main_table_id = FindID (row, 17).toString();
        //найти предыдущий id ;
        QString command = "SELECT id FROM storages WHERE main_table_id = '" + main_table_id + "'";

        QVector<QString> vect_id; // нашли id склада привязанные к id сделок
        if (auto query = ExecuteSQL(command)) {
            while(query.value().next()) {
                vect_id.push_back( query.value().value(0).toString() ); // первое value это от optional
            }
        }
        // найти более раннее id для обновления сальдо ??? более позднее id ???
        QVector<QString> vect_prev_id;
        for (const auto& id_deleted: vect_id) {
            QString temp = FindNextOrPrevIdFromStorage(id_deleted, "prev");
            if (temp == "-1") {
                temp = FindNextOrPrevIdFromStorage(id_deleted, "next");
                // стартовый баланс начать с нуля
                QMap<QString, QString> date;
                date.insert("id", temp);
                date.insert("start_balance", "0");
                UpdateSQLString ("storages", date);

               // qDebug() << "Error on_pushButton_delete_clicked";
            }
            vect_prev_id.push_back( temp );
        }

        // удаляем
        command = "DELETE FROM deals WHERE id = ";
        command += "'" + main_table_id + "'";
        QSqlQuery change_query = QSqlQuery(db_);
        if (!change_query.exec( command )) {
            qDebug() << change_query.lastError().databaseText();
            qDebug() << change_query.lastError().driverText();
            return;
        }

        //Удалить старые данные из storages привязанные к ID основной таблицы
        DeleteFromSQL("storages", main_table_id);

        // обновляем сальдо у поздних

        for (const auto& id_upd : vect_prev_id) {
            if (id_upd != "-1") {
                ChangeFutureStartSaldo (id_upd);
            }
        }


    }

    on_pushButton_deals_clicked();
    //UpdateModelDeals();
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
    //UpdateModelDeals();
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
    last_storage_query_ = store;
    ui->filter_widget->show();
    ui->tableView_header->hide();
    ui->tableView->verticalHeader()->setVisible(true); // включить номерацию строк
    ui->tableView->horizontalHeader()->setFont(current_table_view_font);
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
        ui->comboBox_filter->clear();
        ui->comboBox_filter->addItem("Все");
        std::set <QString> goods_name_set{};
        std::set <QString> combobox_filter_set{};

        auto query_goods_name = ExecuteSQL(goods_name_command);
        while(query_goods_name.value().next()){
            combobox_filter_set.insert(query_goods_name.value().value(0).toString());
            ui->comboBox_filter->addItem(query_goods_name.value().value(0).toString());
        }
        if (!last_storage_filter_.isEmpty()) {
            ui->comboBox_filter->setCurrentIndex(ui->comboBox_filter->findText(last_storage_filter_));
        }
        else {
            ui->comboBox_filter->setCurrentIndex(ui->comboBox_filter->findText("Все"));
        }
        if (!storages_filter_goods.isEmpty()) {
           goods_name_set.insert(storages_filter_goods);
        } else {
            goods_name_set = combobox_filter_set;
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


            model_storages_->setItem(row_count++, 0, new QStandardItem("Товар: " + product)); // название склада


                command = "SELECT to_char(date_of_deal, 'DD-MM-YYYY'), operation, start_balance, arrival_doc, arrival_fact, departure_litres,plotnost,"
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
    ui->tableView->setFont(current_table_view_font); // меняем шрифт
    ui->tableView->setWordWrap(1); //устанавливает перенос слов
    ui->tableView->resizeColumnsToContents(); // адаптирует размер всех столбцов к содержимому
    //ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers); // запрет редактирования

    //прокрутка вниз влево
    QModelIndex bottomLeft = model_storages_->index(model_storages_-> rowCount() - 1, 0);
    ui->tableView->scrollTo(bottomLeft);

}

void MainWindow::SaveSettings () {
    QString buffer{};
    QTextStream stream(&buffer);
    stream << server_<<"?"<<port_<<"?"<<base_name_<<"?"<<login_<<"?"<<pass_<<"?"<<
        current_table_view_font.family()<<"?"<<current_table_view_font.pointSize();

     cl_enc.encrypt(stream.readAll());
}

void MainWindow::ChangeSettingServer(const QMap<QString, QString>& map_set) {
    if (db_.isOpen()) {
        db_.close();
        qDebug() << "Old DB closed";
    }
    QString buffer{};
    QTextStream stream(&buffer);
    stream << map_set["server_"]<<"?"<<map_set["port_"]<<"?"<<map_set["base_name_"]<<"?"
           <<map_set["login_"]<<"?"<<map_set["pass_"]<<"?";
    cl_enc.encrypt(stream.readAll());

    createConnection();
    qDebug() << "Connection new opened?";
}


void MainWindow::on_settings_triggered()
{

    set_window = new SettingWindow;
    set_window->setModal(true);
    if (!(connect ( set_window, &SettingWindow::signal_importcsv, this, &MainWindow::ParsingCSV ) ) ) {
        qDebug() << "connect false";
    }
    if (!(connect ( set_window, &SettingWindow::signal_set_server, this, &MainWindow::ChangeSettingServer ) ) ) {
        qDebug() << "connect setting false";
    }
    if (!(connect ( set_window, &SettingWindow::signal_check_store, this, &MainWindow::CheckStorages ) ) ) {
        qDebug() << "connect check_store false";
    }
    if (!(connect ( set_window, &SettingWindow::signal_font_change, this, &MainWindow::ChangeTableFont ) ) ) {
        qDebug() << "connect font change false";
    }
    if (!(connect ( set_window, &SettingWindow::signal_font_default, this, &MainWindow::CheckCurrentFont ) ) ) {
        qDebug() << "connect font default false";
    }
    if (!(connect ( this, &MainWindow::signal_return_font, set_window, &SettingWindow::ChangeCurrentTableFont ) ) ) {
        qDebug() << "connect current font return false";
    }

    set_window->exec();


   // auto current_font_table = ui->tableView->property("font").value<QFont>();


}


void MainWindow::on_exit_triggered()
{
    close();
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

                QString id =  model->item(*index_set_rows.begin(), 17)->data(Qt::DisplayRole).toString();
                date["date_of_deal"] = FindDateFromIdDeals(id);
            }
            else {
                date["date_of_deal"] = GetCurrentDate();
            }
        } else {
            QString id =  model->item(*index_set_rows.begin(), 17)->data(Qt::DisplayRole).toString();
            // находим дату из SQL
            date["date_of_deal"] = FindDateFromIdDeals(id);


        }


        auto id_num = AddRowSQLString ("deals", date);

        QString temp ={};
        StorageAdding(id_num, temp);
        on_pushButton_deals_clicked();

    }
}



double MainWindow::AveragePriceIn (const QString& date_of_deal,const QString& storage_name,
                                   const QString& tovar_short_name,const QString& start_balance, const QString& id_storage) {

    // если в предыдущей строке Списание - взять средний  цену оттуда. елс иприход - рассчитывать
    QString command = "SELECT arrival_doc, departure_kg, main_table_id  FROM storages WHERE tovar_short_name = '" + tovar_short_name
                      + "' AND storage_name = '" + storage_name + "' AND date_of_deal < '" + date_of_deal + "' "
                      "OR tovar_short_name = '" + tovar_short_name + "' AND storage_name = '" + storage_name +
                      "' AND date_of_deal = '" + date_of_deal + "'" +" AND id < '" + id_storage + "' ";
     command += "ORDER BY date_of_deal DESC, id DESC LIMIT 1";
    if (auto query = ExecuteSQL(command)) {
        if(query.value().next()) {
            if (query.value().value(0).toDouble() == 0 &&  query.value().value(1).toDouble() > 0) {
                QString main_id = query.value().value(2).toString();

                command = "SELECT price_in_tn FROM deals WHERE id = '" + main_id + "'";
                if (auto query_deals = ExecuteSQL(command)) {
                    if(query_deals.value().next()) {
                        double res = query_deals.value().value(0).toDouble();
                        return res;
                    }
                }
            }
        }
    }

    // если в предыдущей строке Приход:
    bool last_mass_bool = false;
   // double last_mass = 0;
    double price_last_mass = 0;
    QString main_id_for_last_mass {};
    QString id_last_mass{};
    QString date_last_mass{};
    double balance_end_last_mass = 0;
    //проверить есть ли списание перед Приходом/Приходами - взять оттуда (конечный вес * ср цену)
     command = "SELECT  main_table_id, id, date_of_deal, balance_end  FROM storages WHERE departure_kg > '0' AND tovar_short_name = '" + tovar_short_name
                       + "' AND storage_name = '" + storage_name + "' AND date_of_deal < '" + date_of_deal + "' "
                       "OR departure_kg > '0' AND tovar_short_name = '" + tovar_short_name + "' AND storage_name = '" + storage_name +
                       "' AND date_of_deal = '" + date_of_deal + "'" +" AND id < '" + id_storage + "' "
                       "ORDER BY date_of_deal DESC, id DESC LIMIT 1";
    if (auto query = ExecuteSQL(command)) {
        if(query.value().next()) {
          //  last_mass = query.value().value(0).toDouble();
            main_id_for_last_mass = query.value().value(0).toString();
            id_last_mass = query.value().value(1).toString();
            date_last_mass = query.value().value(2).toString();
            balance_end_last_mass = query.value().value(3).toDouble();
            last_mass_bool = true;
            command = "SELECT price_in_tn FROM deals WHERE id = '" + main_id_for_last_mass + "'";
            if (auto query_deals = ExecuteSQL(command)) {
                if(query_deals.value().next()) {
                    price_last_mass = query_deals.value().value(0).toDouble();
                }
            }
        }
    }


    // посчитать все приходы после последнего списания
    double total_mass = 0;
    double total_cost = 0;

    QString command_paste = " (arrival_doc > '0' AND tovar_short_name = '" + tovar_short_name
                            + "' AND storage_name = '" + storage_name + "') ";

    if (last_mass_bool) {



        command = "SELECT arrival_doc, price_tn, id  FROM storages WHERE "
                  + command_paste +
                  "AND ( ( ( date_of_deal > '" + date_last_mass + "') OR (date_of_deal = '" + date_last_mass + "' AND id > '" + id_last_mass + "') ) "
                        "AND ( (date_of_deal < '" + date_of_deal + "') OR (date_of_deal = '" + date_of_deal + "' AND id < '" + id_storage + "') ) ) "
                        "ORDER BY date_of_deal ASC, id ASC";

        //setlocale(LC_ALL, "");
        //qDebug() << command;
        total_mass += balance_end_last_mass / 1000;
        total_cost += balance_end_last_mass / 1000 * price_last_mass;
    }
    else { // если списаний ранее не было, посчитать все приходы
        command = "SELECT arrival_doc, price_tn  FROM storages WHERE arrival_doc > '0' AND tovar_short_name = '" + tovar_short_name
                  + "' AND storage_name = '" + storage_name + "' AND date_of_deal < '" + date_of_deal + "' "
                   "OR arrival_doc > '0' AND tovar_short_name = '" + tovar_short_name + "' AND storage_name = '" + storage_name +
                  "' AND date_of_deal = '" + date_of_deal + "'" +" AND id < '" + id_storage + "'";
    }
    std::vector <std::pair <double, double>> store_in;
    if (auto query = ExecuteSQL(command)) {
         while(query.value().next()) {
             store_in.push_back( {query.value().value(0).toString().toDouble(), query.value().value(1).toString().toDouble()} );
             qDebug() << query.value().value(2).toString();
         }
    }
    for (const auto& [in_mass, price]: store_in) {
        total_mass += in_mass/1000;
        total_cost += in_mass / 1000 * price;
    }
    if (total_mass != 0) {
        double result = total_cost / total_mass;
        return result;
    }

    else return 0;

}

double MainWindow::AveragePriceIn (const QString& id_storage) {

    QString command = "SELECT date_of_deal, storage_name, tovar_short_name, start_balance FROM storages WHERE id = '" + id_storage + "'";
    QMap <QString, QString> date;
    if (auto query = ExecuteSQL(command)) {
        while(query.value().next()) {
            date.insert( "date_of_deal", query.value().value(0).toString() ); // первое value это от optional
            date.insert( "storage_name", query.value().value(1).toString() );
            date.insert( "tovar_short_name", query.value().value(2).toString() );
            date.insert( "start_balance", query.value().value(3).toString() );
        }
    }
    return AveragePriceIn (date.value("date_of_deal"), date.value("storage_name"),date.value("tovar_short_name"),date.value("start_balance"), id_storage);
}

bool Parsing_line(QString&& line, std::vector<QMap<QString, QString>>& vect) {

    try
    {
        line.replace(',', '.');
        QStringList list = line.split(";");
        QMap<QString, QString> date {};
        std::vector<QString> names {"date_of_deal", "customer", "number_1c", "postavshik", "neftebaza",
                                   "tovar_short_name", "litres", "plotnost", "ves", "price_in_tn",
                                   "price_out_tn", "price_out_litres", "transp_cost_tn", "commission"};
        for (int i{}; i < list.length(); i++) {
            date[names[i]] = list[i];
        }
        vect.push_back(date);

    } catch (...) {
        return false;
    }
    return true;
}

void MainWindow::ParsingCSV (QFile& file) {
    qDebug() << "ParsingCSV";

    QMap<QString, QString> date;
    QString stroke;
    std::vector<QMap<QString, QString>> vect;

    QTextStream in(&file);
    int counter = 1;
    while (!in.atEnd()) {
        QString line = in.readLine();

        if (!Parsing_line(std::move(line), vect)) {
            qDebug() << "Ошибка в строке " + QString::number(counter);
            QMessageBox::critical(0, "Error in file", "Ошибка в строке " + QString::number(counter), QMessageBox::Cancel);
            break;
        }

        ++counter;
    }
    counter = 1;
    for (auto& item :vect) {
        auto id = AddRowSQLString("deals", item);
        QString val{};
        StorageAdding(id, val);
        qDebug() <<"Строка "+ QString::number(counter);
        ++counter;
    }
    qDebug() << "ParsingCSV end GOOD";

    //AddRowSQL (const QString& storage, const QMap<QString, QString>& date_)
}


void MainWindow::CheckStorages() {
    // удалить все записи из storages
    QString command = "DELETE FROM storages";
    ExecuteSQL(command);

    // запустить для  записей в deals (где есть движение по складу) создание storage записи

    command = "SELECT id FROM deals WHERE customer LIKE 'НБ%' OR postavshik LIKE 'НБ%' ORDER BY date_of_deal ASC, id ASC";
    QVector<QString> vect_deals;
    QMap<QString, QString> dant {};
    if (auto query = ExecuteSQL(command)) {
        while(query.value().next()) {
            vect_deals.push_back( query.value().value(0).toString() ); // первое value это от optional
        }
    }

    command = "SELECT id FROM deals ORDER BY date_of_deal ASC, id ASC";
    if (auto query = ExecuteSQL(command)) {
        while(query.value().next()) {
            auto date = CheckDealsParam(query.value().value(0).toString());
            UpdateSQLString ("deals", date);
        }
    }
    QString empty_stroke{};
    for (const auto& id_deals :  vect_deals) {
        StorageAdding(id_deals, empty_stroke);
    }

    //проверить deals


}

void MainWindow::ChangeTableFont(QString font_name, QString font_size)
{
    QFont font(font_name, font_size.toInt()); //, QFont::Bold
    //ui->tableView->setFont(font);
    //ui->tableView->setProperty("font", font);
    current_table_view_font = font;
    ui->tableView->setFont(current_table_view_font);
    SaveSettings ();
}

void MainWindow::CheckCurrentFont()
{
    //QFont current_font_table = ui->tableView->property("font").value<QFont>();
    //SettingWindow::ChangeCurrentTableFont(current_table_view_font);
    emit signal_return_font(current_table_view_font);
}

void MainWindow::HorizontSizeChange(int logicalIndex, int oldSize, int newSize)
{

        //int width_col = ui->tableView->columnWidth(logicalIndex);
        ui->tableView_header->setColumnWidth(logicalIndex, newSize);

}
void MainWindow::HorizontSizeChangeHeader(int logicalIndex, int oldSize, int newSize)
{

    //int width_col = ui->tableView->columnWidth(logicalIndex);
    ui->tableView->setColumnWidth(logicalIndex, newSize);

}


void MainWindow::on_pushButton_enter_filtr_clicked()
{
    last_storage_filter_ = ui->comboBox_filter->currentText();

    if (ui->comboBox_filter->currentText() == "Все") {
        on_pushButton_filtr_default_clicked();
        return;
    }

    storages_filter_goods = ui->comboBox_filter->currentText();
    ShowStorages(last_storage_query_);

}


void MainWindow::on_pushButton_filtr_default_clicked()
{
    last_storage_filter_ = "Все";
    storages_filter_goods.clear();
    ShowStorages(last_storage_query_);
}

