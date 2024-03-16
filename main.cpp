#include "mainwindow.h"

#include <QApplication>
#include <QTableView>
#include <QStandardItemModel>


int main(int argc, char *argv[])
{
    // QSqlDatabase db;
    QApplication a(argc, argv);
    MainWindow w;

    // if (!createConnection(db)) {
    //     return 1;
    // }

    w.show();
    return a.exec();
}
