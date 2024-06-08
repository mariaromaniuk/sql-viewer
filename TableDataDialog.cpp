#include "TableDataDialog.h"
#include <QTableView>
#include <QVBoxLayout>
#include <QPushButton>
#include <qdialogbuttonbox.h>
#include <qformlayout.h>
#include <QComboBox>
#include <QLineEdit>
#include <QSqlQuery>
#include <QSqlError>

TableDataDialog::TableDataDialog(const QString& tableName, QWidget* parent)
    : QDialog(parent)
{
    tableModel = new QSqlTableModel(this);
    tableModel->setTable(tableName);
    tableModel->select();

    QTableView* tableView = new QTableView(this);
    tableView->setModel(tableModel);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(tableView);

    alterTableButton = new QPushButton("Alter Table", this);
    connect(alterTableButton, &QPushButton::clicked, this, &TableDataDialog::alterTable);

            // Add the alter table button to the layout
    layout->addWidget(alterTableButton);


    setLayout(layout);
    setWindowTitle("Table Data - " + tableName);
}

TableDataDialog::~TableDataDialog()
{
    delete tableModel;
}



void TableDataDialog::alterTable()
{
    // Create a new dialog for altering the table structure
    QDialog alterDialog(this);
    alterDialog.setWindowTitle("Alter Table");

            // Create input fields for modifying the table structure
    QVBoxLayout* alterLayout = new QVBoxLayout(&alterDialog);
    QFormLayout* formLayout = new QFormLayout();
    QVector<QPair<QLineEdit*, QComboBox*>> inputFields;

            // Retrieve the existing column names and types from the table
    QSqlQuery columnsQuery(QSqlDatabase::database());
    columnsQuery.exec(QString("PRAGMA table_info(%1)").arg(tableName));

    while (columnsQuery.next()) {
        QString columnName = columnsQuery.value("name").toString();
        QString columnType = columnsQuery.value("type").toString();

        QLineEdit* nameEdit = new QLineEdit(columnName);
        QComboBox* typeComboBox = new QComboBox();
        typeComboBox->addItems({"INTEGER", "REAL", "TEXT"});
        typeComboBox->setCurrentText(columnType);

        formLayout->addRow(columnName + ":", nameEdit);
        formLayout->addRow("Type:", typeComboBox);

        inputFields.append(qMakePair(nameEdit, typeComboBox));
    }

            // Add an empty row for a new column
    QLineEdit* newNameEdit = new QLineEdit();
    QComboBox* newTypeComboBox = new QComboBox();
    newTypeComboBox->addItems({"INTEGER", "REAL", "TEXT"});
    formLayout->addRow("New Column:", newNameEdit);
    formLayout->addRow("Type:", newTypeComboBox);
    inputFields.append(qMakePair(newNameEdit, newTypeComboBox));

    alterLayout->addLayout(formLayout);

            // Create buttons for submitting, adding, and canceling
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton* addFieldButton = new QPushButton("Add Field");
    QPushButton* deleteFieldButton = new QPushButton("Delete Field");
    buttonBox->addButton(addFieldButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(deleteFieldButton, QDialogButtonBox::ActionRole);

    connect(buttonBox, &QDialogButtonBox::accepted, [&]() {
        // Get the modified table structure from the input fields
        QVector<QPair<QString, QString>> modifiedColumns;
        for (const auto& inputField : qAsConst(inputFields)) {
            QString columnName = inputField.first->text();
            QString columnType = inputField.second->currentText();
            if (!columnName.isEmpty())
                modifiedColumns.append(qMakePair(columnName, columnType));
        }

                // ... (the rest of the code remains the same) ...
    });

    connect(buttonBox, &QDialogButtonBox::rejected, &alterDialog, &QDialog::reject);
    connect(addFieldButton, &QPushButton::clicked, [&]() {
        QLineEdit* newNameEdit = new QLineEdit();
        QComboBox* newTypeComboBox = new QComboBox();
        newTypeComboBox->addItems({"INTEGER", "REAL", "TEXT"});
        formLayout->addRow("New Column:", newNameEdit);
        formLayout->addRow("Type:", newTypeComboBox);
        inputFields.append(qMakePair(newNameEdit, newTypeComboBox));
    });

    connect(deleteFieldButton, &QPushButton::clicked, [&]() {
        if (!inputFields.isEmpty()) {
            QLineEdit* nameEdit = inputFields.last().first;
            QComboBox* typeComboBox = inputFields.last().second;
            formLayout->removeRow(typeComboBox);
            formLayout->removeRow(nameEdit);
            inputFields.removeLast();
        }
    });

    alterLayout->addWidget(buttonBox);

            // Execute the alter dialog
    alterDialog.exec();
}

void TableDataDialog::refreshData()
{
    // Refresh the data in the table model
    tableModel->select();
}
