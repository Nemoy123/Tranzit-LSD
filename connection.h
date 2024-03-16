#ifndef CONNECTION_H
#define CONNECTION_H

#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QMessageBox>
#include <QtSql/QSqlError>

static bool createConnection() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "connection1");
    db.setHostName("localhost");
    db.setDatabaseName("tranzit");
    db.setUserName("postgres");
    db.setPassword("Bocman");

    if (!db.open()) {// Если соединение с базой данных не удастся, оно появится
        //critical(QWidget *parent, const QString &title,
        //const QString &text,
        //QMessageBox::StandardButtons buttons = Ok,
        //QMessageBox::StandardButton defaultButton = NoButton)
        QMessageBox::critical(0, "Cannot open database",
                              "Unable to establish a database connection", QMessageBox::Cancel);
        return false;
    }
    return true;
}


#endif // CONNECTION_H


