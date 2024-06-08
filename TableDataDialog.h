// TableDataDialog.h
#pragma once

#include <QDialog>
#include <QSqlTableModel>

class TableDataDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TableDataDialog(const QString& tableName, QWidget* parent = nullptr);
    ~TableDataDialog();

private slots:
    void alterTable();
    void refreshData();


private:
    QSqlTableModel* tableModel;
    QString tableName;
    QPushButton* alterTableButton;
};
