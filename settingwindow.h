#ifndef SETTINGWINDOW_H
#define SETTINGWINDOW_H

#include <QDialog>
#include <QListWidgetItem>
#include <QMessageBox>


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
    void signal_importcsv(std::ifstream& file);

private slots:
    void on_listWidget_itemClicked(QListWidgetItem *item);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::SettingWindow *ui;
    //void ParsingCSV(const std::ifstream& file);


};

#endif // SETTINGWINDOW_H
