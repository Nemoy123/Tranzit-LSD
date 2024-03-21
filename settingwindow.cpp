#include "settingwindow.h"
#include "ui_settingwindow.h"
#include <QFileDialog>
#include <fstream>

SettingWindow::SettingWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingWindow)
{
    ui->setupUi(this);

    //ui->Widget_import->hide();
    ui->listWidget->addItem("Импорт данных");
    ui->listWidget->addItem("Текст");
    ui->listWidget->addItem("База данных");

}

SettingWindow::~SettingWindow()
{
    delete ui;
}

void SettingWindow::on_listWidget_itemClicked(QListWidgetItem *item)
{
    ui->Widget_import->show();
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
    std::ifstream file (ui->textEdit->toPlainText().toLocal8Bit(), std::ios::in);
    if (!file.is_open()) {
        qDebug() << ui->textEdit->toPlainText().toLocal8Bit();
        QMessageBox::critical(0, "Cannot open file", "Проверьте путь к файлу", QMessageBox::Cancel);

        return;
    }
    qDebug() << "Успех";
    // вставить коннект на сигнал функции в главном окне
    //ParsingCSV(file);
    emit signal_importcsv(file);

}

