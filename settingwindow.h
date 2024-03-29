#ifndef SETTINGWINDOW_H
#define SETTINGWINDOW_H

#include <QDialog>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QFile>

namespace Ui {
class SettingWindow;
}

class SettingWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SettingWindow(QWidget *parent = nullptr);
    ~SettingWindow();

signals:
    void signal_importcsv(QFile& file);
    void signal_set_server(const QMap <QString, QString>& server_set);
    void signal_check_store();

private slots:
    void on_listWidget_itemClicked(QListWidgetItem *item);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_Button_check_store_clicked();

private:
    Ui::SettingWindow *ui;



};

#endif // SETTINGWINDOW_H
