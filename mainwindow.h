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
#include <set>

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

    //void on_tableView_clicked(const QModelIndex &index);

    void on_pushButton_copy_clicked();

    void on_pushButton_paste_clicked();

    void on_pushButton_delete_clicked();

    void on_pushButton_new_clicked();

    void on_action_triggered();

    void on_action_hovered();

    void on_action_2_triggered();

    void on_action_2_hovered();

    void on_action_2_changed();

    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

    void tableSelectionChanged(QItemSelection, QItemSelection);

    void on_pushButton_insert_clicked();

private:
    QVariant FindID (int row, int column);
    void UpdateListStorage();
    void ShowStorages(const QString& store);
    Ui::MainWindow *ui;
    QStandardItemModel* model;
    QStandardItemModel* model_storages_;
    std::optional<QSqlQuery> ExecuteSQL(const QString& command);
    QString  AddRowSQLString (const QString& storage, const QMap<QString, QString>& date_);
    bool AddRowSQL (const QString& storage, const QMap<QString, QString>& date_);
    bool DeleteFromSQL (const QString& storage, const QString& id);
    bool StorageAdding(const QString& id_string, QString& new_text);
    bool UpdateStorageLine(const QVector <QString>& vect_deals, int column, QString& new_text, const QString& id_string);
    QString GetCurrentDate (); // получить дату Сегодня из SQL
    QSqlDatabase db_;
   /* const QModelIndex* index_buffer_ = nullptr;
    const QModelIndex* index_for_copy_ = nullptr;
*/

    //int index_buffer_ = -1;
    //int index_for_copy_ = -1;
    std::set <int> index_set_rows{};
    std::set <int> index_set_rows_copy{};

};
#endif // MAINWINDOW_H
