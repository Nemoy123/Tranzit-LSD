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

    void ChangeCurrentTableFont (QFont font);

signals:
    void signal_importcsv(QFile& file);
    void signal_set_server(const QMap <QString, QString>& server_set);
    void signal_check_store();
    void signal_font_change (QString font_name, QString font_size);
    void signal_font_default();


private slots:
    void on_listWidget_itemClicked(QListWidgetItem *item);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_Button_check_store_clicked();

    void on_pushButton_8_clicked();

    void on_pushButton_7_clicked();

    void on_pushButton_4_clicked();


private:
    Ui::SettingWindow *ui;
    void FontSizeSetting ();
    QFont current_table;

};

#endif // SETTINGWINDOW_H
