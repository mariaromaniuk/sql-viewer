#ifndef ADDTABLEDIALOG_H
#define ADDTABLEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVector>
#include <QComboBox>

class AddTableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddTableDialog(QWidget *parent = nullptr);
    ~AddTableDialog();

    QString getTableName() const;
    QVector<QPair<QString, QString>> getColumnInfo() const;

signals:
    void submitClicked(const QString &tableName, const QVector<QPair<QString, QString>> &columnInfo);

private slots:
    void addRow();
    void submitTable();

private:
    QLineEdit *tableNameEdit;
    QVBoxLayout *rowsLayout;
    QPushButton *addRowButton;
    QVector<QPair<QString, QString>> columnInfo;
    QStringList allowedColumnTypes;
};

#endif
