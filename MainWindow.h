#pragma once
#include <QMainWindow>
#include <QListWidget>
#include <QTableView>
#include <QTableWidget>
#include "AddTableDialog.h"
#include "DatabaseConfig.h"

namespace Ui
{
class MainWindow;
}  // namespace Ui

class QSqlTableModel;
class QSqlDatabase;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(DatabaseConfig databaseConfig,
                        QWidget* parent = nullptr);
    ~MainWindow() override;
    MainWindow& operator=(const MainWindow& other) = delete;
    MainWindow(const MainWindow& other) = delete;
    MainWindow& operator=(MainWindow&& other) = delete;
    MainWindow(MainWindow&& other) = delete;
    bool validateType(const QString& value, const QString& type);

private:
    void closeCurrentDatabase();
    QSqlTableModel* createNewModel(QSqlDatabase& database) const;
    void prepareView(QSqlDatabase& database);
    void openDatabaseFile(const QString& databaseFilePath);

    Ui::MainWindow* ui;
    DatabaseConfig databaseConfig_;
    AddTableDialog addTableDialog;
    QListWidget* tableListWidget;
    QTableWidget *tableWidget;
    QString selectedTableName; // Member variable to store selected table name
    QVector<QComboBox*> typeComboBoxes;

private slots:
    void createNewDb();
    void addTable();
    void deleteTable();
    void viewTables();
    void updateTable();
    void openExistingDb();
    void rowSelectionChanged(const QModelIndex& current,
                             const QModelIndex& previous);
    void createTable(const QString &tableName, const QVector<QPair<QString, QString>> &columnInfo);
    void deleteRow();
    void addRow();
    void showTableData(QListWidgetItem* item);
    void deleteTableFromWidget(QListWidgetItem *item);
    void deleteRow(QDialog& dialog, QTableView* tableView, const QString& tableName);
    void addRow(QDialog& dialog, QTableView* tableView, const QString& tableName);
    void alterTable(const QString& tableName);
    void addColumn(QDialog& dialog, QTableView* tableView, const QString& tableName);
    void deleteColumn(QDialog& dialog, QTableView* tableView, const QString& tableName);
};
