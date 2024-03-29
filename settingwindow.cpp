#include "settingwindow.h"
#include "ui_settingwindow.h"
#include <QFileDialog>
//#include <fstream>


SettingWindow::SettingWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingWindow)
{
    ui->setupUi(this);

    ui->Widget_import->hide();
    ui->listWidget->addItem("База данных");
    ui->listWidget->addItem("Импорт данных");
    //ui->listWidget->addItem("Текст");
    ui->listWidget->addItem("Проверка БД");
    ui->BaseWidget_1->hide();
    ui->BaseWidget_2->hide();
    ui->Widget_check_store->hide();
    ui->Widget_import->hide();

}

SettingWindow::~SettingWindow()
{
    delete ui;
}

void SettingWindow::on_listWidget_itemClicked(QListWidgetItem *item)
{
    auto store = item->text();
    if (store == "Импорт данных") {
        ui->BaseWidget_1->hide();
        ui->BaseWidget_2->hide();
        ui->Widget_check_store->hide();
        ui->Widget_import->show();
    }
    else if (store == "База данных") {
        ui->Widget_import->hide();
        ui->Widget_check_store->hide();
        ui->BaseWidget_1->show();
        ui->BaseWidget_2->show();
    }
    else if (store == "Проверка БД") {
        ui->Widget_import->hide();
        ui->Widget_check_store->show();
        ui->BaseWidget_1->hide();
        ui->BaseWidget_2->hide();
    }

}


void SettingWindow::on_pushButton_clicked()
{
    auto fileName = QFileDialog::getOpenFileName(this,
                            tr("Open file"), "/", tr("CSV File (*.csv)"));
    ui->textEdit->setText(fileName);
}


void SettingWindow::on_pushButton_2_clicked()
{
    //проверить правильность пути
    QFile file (ui->textEdit->toPlainText());

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << ui->textEdit->toPlainText();
        QMessageBox::critical(0, "Cannot open file", "Проверьте путь к файлу", QMessageBox::Cancel);

        return;
    }
    qDebug() << "Успех";
    // вставить коннект на сигнал функции в главном окне
    //ParsingCSV(file);
    emit signal_importcsv(file);

}


void SettingWindow::on_pushButton_3_clicked() // применить настройки сервера
{
    QMap <QString, QString> server_set;
    server_set["base_name_"] = ui->textEdit_server_name->toPlainText();
    server_set["server_"] = ui->textEdit_ip_server->toPlainText();
    server_set["port_"] = ui->textEdit_port_server->toPlainText();
    server_set["login_"] = ui->textEdit_login_server->toPlainText();
    server_set["pass_"] = ui->textEdit_pass_server->toPlainText();
    qDebug() << "Отправляем сигнал настройки сервера";
    // вставить коннект на сигнал функции в главном окне

    emit signal_set_server(server_set);
}


void SettingWindow::on_Button_check_store_clicked()
{
    // даем сигналя для запуска слота проверки базы склада
    emit signal_check_store ();
}

