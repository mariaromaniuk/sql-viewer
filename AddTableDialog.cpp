#include "addtabledialog.h"

#include <QHBoxLayout>
#include <QLineEdit>

AddTableDialog::AddTableDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Add Table"));

    tableNameEdit = new QLineEdit;
    tableNameEdit->setPlaceholderText(tr("Enter table name"));

    rowsLayout = new QVBoxLayout;

    addRowButton = new QPushButton(tr("Add Row"));
    connect(addRowButton, &QPushButton::clicked, this, &AddTableDialog::addRow);

            // Allowed column types
    allowedColumnTypes << "INTEGER" << "REAL" << "TEXT";

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(tableNameEdit);
    mainLayout->addLayout(rowsLayout);
    mainLayout->addWidget(addRowButton);

    QPushButton *submitButton = new QPushButton(tr("Submit"));
    connect(submitButton, &QPushButton::clicked, this, &AddTableDialog::submitTable);
    mainLayout->addWidget(submitButton);

    setLayout(mainLayout);
}

AddTableDialog::~AddTableDialog()
{
}

QString AddTableDialog::getTableName() const
{
    return tableNameEdit->text();
}

QVector<QPair<QString, QString>> AddTableDialog::getColumnInfo() const
{
    return columnInfo;
}

void AddTableDialog::addRow()
{
    QHBoxLayout *rowLayout = new QHBoxLayout;

    QLineEdit *nameEdit = new QLineEdit;
    nameEdit->setPlaceholderText(tr("Column Name"));

    QComboBox *typeComboBox = new QComboBox;
    typeComboBox->addItems(allowedColumnTypes);

    rowLayout->addWidget(nameEdit);
    rowLayout->addWidget(typeComboBox);

    QWidget *rowWidget = new QWidget;
    rowWidget->setLayout(rowLayout);

    rowsLayout->addWidget(rowWidget);

    columnInfo.append(qMakePair(QString(), QString()));
}

void AddTableDialog::submitTable()
{
    columnInfo.clear();
    for (int i = 0; i < rowsLayout->count(); ++i) {
        QWidget *rowWidget = rowsLayout->itemAt(i)->widget();
        if (rowWidget) {
            QHBoxLayout *rowLayout = qobject_cast<QHBoxLayout*>(rowWidget->layout());
            if (rowLayout) {
                QLineEdit *nameEdit = qobject_cast<QLineEdit*>(rowLayout->itemAt(0)->widget());
                QComboBox *typeComboBox = qobject_cast<QComboBox*>(rowLayout->itemAt(1)->widget());
                if (nameEdit && typeComboBox) {
                    columnInfo.append(qMakePair(nameEdit->text(), typeComboBox->currentText()));
                }
            }
        }
    }

    accept();
}
