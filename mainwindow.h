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
#include "settingwindow.h"
#include <QFile>
#include <QStyledItemDelegate>
#include <QComboBox>
#include <QItemDelegate>

// Делегат для комбобокса внутри tableview
class ComboBoxDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    ComboBoxDelegate(QObject *parent = 0, std::vector < std::vector<QString> >* vect = nullptr, std::map <QString, QString>* filter_deals = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;
    void paint (QPainter* painter, const QStyleOptionViewItem& option,
                const QModelIndex& index) const;
    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option, const QModelIndex &index) const;
private:
    std::vector < std::vector<QString> >* vect_;
    std::map <QString, QString>* filter_deals_;
};
//конец делегата



class Encdec {
    int key = 13;

    // File name to be encrypt
    const QString* file_name_;
    char c;

public:
    Encdec(const QString* name): file_name_(name){}
    void encrypt(const QString& in);
    QString decrypt();
    const QString* GetName() const {return file_name_;}
};

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

public slots:

    void ParsingCSV(QFile& file);

private slots:
    void on_pushButton_deals_clicked();

    //void on_tableView_clicked(const QModelIndex &index);

    void on_pushButton_copy_clicked();

    void on_pushButton_delete_clicked();

    void on_pushButton_new_clicked();

    void on_action_triggered();

    void on_action_hovered();

    void on_action_2_triggered();

    void on_action_2_hovered();

    void on_action_2_changed();

    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

    void tableSelectionChanged(QItemSelection, QItemSelection);

    void on_settings_triggered();

    void on_exit_triggered();

    void on_pushButton_paste_clicked();

    void CheckStorages();
    void ChangeTableFont(QString font_name, QString font_size);
    void CheckCurrentFont();
    void HorizontSizeChange(int logicalIndex, int oldSize, int newSize);
    void HorizontSizeChangeHeader(int logicalIndex, int oldSize, int newSize);

    void on_pushButton_enter_filtr_clicked();

    void on_pushButton_filtr_default_clicked();

signals:
    void signal_return_font (QFont font);

private:
    bool LoadConfig ();
    bool createConnection();

    QSqlDatabase db_;
    QString server_ = "localhost";
    int port_ = 5432;
    QString base_name_ = "tranzit";
    QString login_ = "postgres";
    QString pass_ = "Serva_984";
    QString setting_file_ = "settings.lsd";
    Encdec cl_enc;
    void ChangeSettingServer(const QMap<QString, QString>& map_set);
    void SaveSettings ();

    QVariant FindID (int row, int column);
    void UpdateListStorage();
    void ShowStorages(const QString& store);
    Ui::MainWindow *ui;
    QStandardItemModel* model;
    QStandardItemModel* model_header_;
    QStandardItemModel* model_storages_;
    std::optional<QSqlQuery> ExecuteSQL(const QString& command);
    QString  AddRowSQLString (const QString& storage, QMap<QString, QString>& date_);
    bool AddRowSQL (const QString& storage, QMap<QString, QString>& date_);
    bool DeleteFromSQL (const QString& storage, const QString& main_table_id);
    bool StorageAdding(const QString& id_string, QString& new_text);
    QString GetCurrentDate (); // получить дату Сегодня из SQL

    double StartingSaldo (const QString& date_of_deal, const QString& tovar_short_name, const QString& storage_name);
    double StartingSaldo (const QString& storage_id);
    void ChangeFutureStartSaldo (const QString& id);
    void ChangeFutureStartSaldo (const QString& id, const QString& date_of_deal,const QString& storage_name, const QString& tovar_short_name);
    void UpdateSQLString (const QString& storage, const QMap<QString, QString>& date);
    QString FindPrevIdFromStorage (const QString& storage_id);
    QString FindNextOrPrevIdFromStorage (const QString& storage_id, const QString& operation);
    double AveragePriceIn (const QString& id_storage);
    double AveragePriceIn (const QString& date_of_deal,const QString& storage_name,const QString& tovar_short_name,const QString& start_balance, const QString& id_storage);
    std::set <int> index_set_rows{};
    std::set <int> index_set_rows_copy{};
    SettingWindow* set_window;
    void UpdateAverageForLater (const QString& id);
    bool first_launch = true;
    int index_row_change_item = -1;
    // void UpdateModelDeals ();
    QMap<QString, QString> CheckDealsParam (QMap<QString, QString>& date);
    QMap<QString, QString> CheckDealsParam (const QString& id_deals);

    QFont current_table_view_font = QFont("Segoe UI", 10);

    std::map <QString, QString> filter_deals{};
    std::vector < std::vector<QString> > vect_deals_filters{};
    //std::vector<QString> last_filter_choice_{};
    QString storages_filter_goods {};
    QString last_storage_query_ {};
    QString last_storage_filter_ {};
    QString FindNextStorageIdFromIdDeals (const QString& id_deals);
    QString FindDateFromIdDeals (const QString& id_deals);
};
#endif // MAINWINDOW_H
