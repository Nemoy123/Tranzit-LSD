#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QMessageBox>
#include <QtSql/QSqlError>
#include <QStandardItemModel>
#include <QListWidgetItem>
#include <optional>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_deals_clicked();

    void on_tableView_clicked(const QModelIndex &index);

    void on_pushButton_copy_clicked();

    void on_pushButton_insert_clicked();

    void on_pushButton_delete_clicked();

    void on_pushButton_new_clicked();

    void on_action_triggered();

    void on_action_hovered();

    void on_action_2_triggered();

    void on_action_2_hovered();

    void on_action_2_changed();

    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

private:
    QVariant FindID (int row, int column);
    void UpdateListStorage();
    void ShowStorages(const QString& store);
    Ui::MainWindow *ui;
    QStandardItemModel* model;
    QStandardItemModel* model_storages_;
    std::optional<QSqlQuery> ExecuteSQL(const QString& command);
    bool AddRowSQL (const QString& storage, const QMap<QString, QString>& date_);
    bool DeleteFromSQL (const QString& storage, const QString& id);
    bool StorageAdding(const QString& id_string, QString& new_text);
    bool UpdateStorageLine(const QVector <QString>& vect_deals, int column, QString& new_text, const QString& id_string);
    QSqlDatabase db_;
   /* const QModelIndex* index_buffer_ = nullptr;
    const QModelIndex* index_for_copy_ = nullptr;
*/
    int index_buffer_ = -1;
    int index_for_copy_ = -1;

};
#endif // MAINWINDOW_H
