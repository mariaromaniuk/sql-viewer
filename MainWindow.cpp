#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QString>
#include <QSqlError>
#include <QMessageBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QTableWidget>

#include "AddRowDialog.h"
#include "AddTableDialog.h"
#include "TableDataDialog.h"

MainWindow::MainWindow(DatabaseConfig databaseConfig, QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      databaseConfig_(std::move(databaseConfig))
{
    ui->setupUi(this);
    tableListWidget = new QListWidget(this);
    // Ініціалізація tableWidget
    tableWidget = new QTableWidget(this);
    ui->centralWidget->layout()->addWidget(tableWidget);
    tableWidget->setVisible(false);
    ui->centralWidget->layout()->addWidget(tableListWidget);
    connect(ui->actionExit, &QAction::triggered, QCoreApplication::instance(),
            &QCoreApplication::quit);
    typeComboBoxes = {}; // Ініціалізація typeComboBoxes

    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::createNewDb);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openExistingDb);
    connect(tableListWidget, &QListWidget::itemClicked, this, &MainWindow::deleteTableFromWidget);
    connect(ui->actionAdd_Table, &QAction::triggered, this, &MainWindow::addTable);
    connect(ui->actionView_Tables, &QAction::triggered, this, &MainWindow::viewTables);
    connect(ui->actionUpdate_Table, &QAction::triggered, this, &MainWindow::updateTable);
    connect(ui->actionDelete_Table, &QAction::triggered, this, &MainWindow::deleteTable);
    connect(&addTableDialog, &AddTableDialog::submitClicked, this, &MainWindow::createTable);
    connect(tableListWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::showTableData);
    QStyle* style{QApplication::style()};
    ui->actionNew->setIcon(style->standardIcon(QStyle::SP_FileDialogNewFolder));
    ui->actionOpen->setIcon(style->standardIcon(QStyle::SP_DialogOpenButton));
    ui->actionExit->setIcon(style->standardIcon(QStyle::SP_DialogCloseButton));

}
bool MainWindow::validateType(const QString& value, const QString& type)
{
    if (value.isEmpty()) {
        return type == "INTEGER" || type == "REAL" || type == "TEXT";
    }

    bool ok;
    if (type == "INTEGER") {
        value.toInt(&ok);
    } else if (type == "REAL") {
        value.toDouble(&ok);
    } else if (type == "TEXT") {
        ok = true; // Для текстових значень завжди повертаємо true
    } else {
        ok = false;
    }
    return ok;
}
MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::alterTable(const QString& tableName)
{
    QDialog alterDialog(this);
    alterDialog.setWindowTitle("Alter Table");

    QVBoxLayout* layout = new QVBoxLayout(&alterDialog);

    QVector<QWidget*> inputFields;
    QVector<QComboBox*> typeComboBoxes;

    auto addField = [&]() {
        QWidget* fieldWidget = new QWidget(&alterDialog);
        QHBoxLayout* fieldLayout = new QHBoxLayout(fieldWidget);

        QLineEdit* columnNameEdit = new QLineEdit(fieldWidget);
        QComboBox* columnTypeComboBox = new QComboBox(fieldWidget);
        columnTypeComboBox->addItems({"INTEGER", "REAL", "TEXT"});

        fieldLayout->addWidget(columnNameEdit);
        fieldLayout->addWidget(columnTypeComboBox);

        inputFields.append(columnNameEdit);
        typeComboBoxes.append(columnTypeComboBox);

        layout->addWidget(fieldWidget);
    };

    auto removeField = [&]() {
        if (!inputFields.isEmpty()) {
            QWidget* fieldWidget = inputFields.last()->parentWidget();
            inputFields.removeLast();
            typeComboBoxes.removeLast();
            delete fieldWidget;
        }
    };

    QPushButton* addFieldButton = new QPushButton("Add Field", &alterDialog);
    connect(addFieldButton, &QPushButton::clicked, addField);

    QPushButton* removeFieldButton = new QPushButton("Remove Field", &alterDialog);
    connect(removeFieldButton, &QPushButton::clicked, removeField);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(addFieldButton);
    buttonLayout->addWidget(removeFieldButton);

    layout->addLayout(buttonLayout);

            // Add initial field
    addField();

            // Create buttons for submitting and canceling
    QPushButton* submitButton = new QPushButton("Submit", &alterDialog);
    QPushButton* cancelButton = new QPushButton("Cancel", &alterDialog);

    QHBoxLayout* confirmButtonLayout = new QHBoxLayout();
    confirmButtonLayout->addWidget(submitButton);
    confirmButtonLayout->addWidget(cancelButton);

    layout->addLayout(confirmButtonLayout);

    connect(submitButton, &QPushButton::clicked, [&]() {
        QSqlDatabase database = QSqlDatabase::database();
        QSqlQuery query(database);

                // Begin transaction
        database.transaction();

        try {
            // Rename the existing table to a temporary name
            QString tempTableName = tableName + "_temp";
            QString renameQuery = QString("ALTER TABLE %1 RENAME TO %2").arg(tableName, tempTableName);
            if (!query.exec(renameQuery)) {
                throw query.lastError();
            }

                    // Create the new table with the updated fields
            QString createQuery = QString("CREATE TABLE %1 (").arg(tableName);
            for (int i = 0; i < inputFields.count(); ++i) {
                QLineEdit* columnNameEdit = qobject_cast<QLineEdit*>(inputFields[i]);
                QString columnName = columnNameEdit->text();
                QString columnType = typeComboBoxes[i]->currentText();
                createQuery += QString("%1 %2").arg(columnName, columnType);
                if (i < inputFields.count() - 1) {
                    createQuery += ", ";
                }
            }
            createQuery += ")";
            if (!query.exec(createQuery)) {
                throw query.lastError();
            }

                    // Copy the data from the temporary table to the new table
            QString insertQuery = QString("INSERT INTO %1 SELECT * FROM %2").arg(tableName, tempTableName);
            if (!query.exec(insertQuery)) {
                throw query.lastError();
            }

                    // Drop the temporary table
            QString dropQuery = QString("DROP TABLE %1").arg(tempTableName);
            if (!query.exec(dropQuery)) {
                throw query.lastError();
            }

                    // Commit the transaction
            database.commit();

            QMessageBox::information(this, "Success", "Table altered successfully.");
        } catch (const QSqlError& error) {
            // Rollback the transaction in case of an error
            database.rollback();
            QMessageBox::critical(this, "Error", "Failed to alter table: " + error.text());
        }

        alterDialog.accept();
    });

    connect(cancelButton, &QPushButton::clicked, [&]() {
        alterDialog.reject();
    });

    alterDialog.exec();
}
void MainWindow::createTable(const QString& tableName, const QVector<QPair<QString, QString>>& columnInfo)
{
    QSqlDatabase database = QSqlDatabase::database();
    if (!database.isOpen()) {
        if (!database.open()) {
            ui->statusBar->showMessage(tr("Failed to open database"));
            return;
        }
    }

    QString createQuery = QString("CREATE TABLE %1 (").arg(tableName);
    for (int i = 0; i < columnInfo.size(); ++i) {
        // Перевірка типу колонки
        createQuery += columnInfo[i].first + " " + columnInfo[i].second;
        if (i < columnInfo.size() - 1)
            createQuery += ", ";
    }
    createQuery += ")";

    QSqlQuery query(database);
    database.transaction();
    if (!query.exec(createQuery)) {
        database.rollback();
        ui->statusBar->showMessage(tr("Failed to create table: %1").arg(query.lastError().text()));
    } else {
        database.commit();
        ui->statusBar->showMessage(tr("Table created successfully"));
        viewTables();
    }
    database.close();
}
void MainWindow::showTableData(QListWidgetItem* item)
{
    if (!item) {
        return;
    }
    QString itemText = item->text();
    QString tableName = itemText.split("\n").first().split(":").last().trimmed();
    QSqlDatabase database = QSqlDatabase::database();
    if (!database.isOpen()) {
        if (!database.open()) {
            qDebug() << "Failed to open database";
            return;
        }
    }
    // Створюємо екземпляр QSqlTableModel
    QSqlTableModel* model = new QSqlTableModel(nullptr, database);
    model->setTable(tableName);
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->select();

            // Отримуємо типи колонок таблиці
    QSqlQuery columnTypesQuery(database);
    columnTypesQuery.exec(QString("PRAGMA table_info(%1)").arg(tableName));
    QVector<QString> columnTypes;
    while (columnTypesQuery.next()) {
        columnTypes.append(columnTypesQuery.value("type").toString().toUpper());
    }

            // Створюємо QTableView і встановлюємо модель даних
    QDialog dialog(this);
    dialog.setWindowTitle("Table Data - " + tableName);
    QTableView* tableView = new QTableView(&dialog);
    tableView->setModel(model);
    tableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);

            // Перевірка типу даних після редагування комірки
    connect(tableView->itemDelegate(), &QAbstractItemDelegate::closeEditor, [&](QWidget* editor, QAbstractItemDelegate::EndEditHint hint) {
        Q_UNUSED(hint);
        QModelIndex index = tableView->currentIndex();
        if (!index.isValid()) {
            return;
        }
        int row = index.row();
        int column = index.column();
        QString columnType = columnTypes[column];
        QString value = static_cast<QLineEdit*>(editor)->text();
        if (validateType(value, columnType)) {
            // Якщо тип даних коректний, застосовуємо зміни
            model->setData(index, value);
            model->submitAll();
        } else {
            QMessageBox::warning(&dialog, "Invalid Input", QString("Invalid value for column %1 (expected type: %2)").arg(model->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString()).arg(columnType));
            model->revertAll();
        }
    });

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->addWidget(tableView);

    // Додаємо меню "Edit" до компонування
    QMenuBar* menuBar = new QMenuBar(&dialog);
    layout->setMenuBar(menuBar);
    QMenu* editMenu = menuBar->addMenu("&Edit");
    QAction* addRowAction = editMenu->addAction("Add Row");
    QAction* deleteRowAction = editMenu->addAction("Delete Row");
    QAction* addColumnAction = editMenu->addAction("Add Column");
    QAction* deleteColumnAction = editMenu->addAction("Delete Column");

            // Підключаємо обробники подій для кнопок меню
    connect(addRowAction, &QAction::triggered, [&]() {
        addRow(dialog, tableView, tableName);
    });
    connect(deleteRowAction, &QAction::triggered, [&]() {
        deleteRow(dialog, tableView, tableName);
    });
    connect(addColumnAction, &QAction::triggered, [&]() {
        addColumn(dialog, tableView, tableName);
    });
    connect(deleteColumnAction, &QAction::triggered, [&]() {
        deleteColumn(dialog, tableView, tableName);
    });

    layout->addWidget(tableView);
    dialog.exec();
}



void MainWindow::deleteTableFromWidget(QListWidgetItem* item)
{
    if (!item) {
        return;
    }

    QString itemText = item->text();
    QString tableName = itemText.split("\n").first().split(":").last().trimmed();
    selectedTableName = tableName;
}

void MainWindow::closeCurrentDatabase()
{
    // QAbstractItemModel* currentModel{ui->tableView->model()};
    // ui->tableView->setModel(nullptr);
    // delete currentModel;

    // QString currentDatabasePath;
    // {
    //     QSqlDatabase database{QSqlDatabase::database()};
    //     currentDatabasePath = database.connectionName();
    //     database.close();
    // }
    // QSqlDatabase::removeDatabase(currentDatabasePath);
}

QSqlTableModel* MainWindow::createNewModel(QSqlDatabase& database) const
{
    // QSqlTableModel* model{new QSqlTableModel(ui->tableView, database)};
    // model->setTable(databaseConfig_.getTableName());
    // model->setEditStrategy(QSqlTableModel::OnFieldChange);

    // const QVector<QString> userFriendlyColumnNames{
    //                                                databaseConfig_.getUserFriendlyColumnNames()};
    // for (int i = 0; i < userFriendlyColumnNames.size(); ++i) {
    //     // Skip primary key column (column 0).
    //     model->setHeaderData(i + 1, Qt::Horizontal, userFriendlyColumnNames[i]);
    // }

    // model->select();
    // return model;
}


void MainWindow::prepareView(QSqlDatabase& database)
{/*
    ui->tableView->clearFocus();
    ui->tableView->setModel(createNewModel(database));
    ui->tableView->hideColumn(0);

    connect(ui->tableView->selectionModel(),
            &QItemSelectionModel::currentRowChanged, this,
            &MainWindow::rowSelectionChanged);*/
}

void MainWindow::openDatabaseFile(const QString& databaseFilePath)
{
    if (databaseFilePath.isNull())
        return;

    closeCurrentDatabase();

    QSqlDatabase database{QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"))};
    database.setDatabaseName(databaseFilePath);

    if (!database.open()) {
        ui->statusBar->showMessage(tr("Cannot open database") + " " + databaseFilePath);
        return;
    }


    ui->actionAdd_row->setEnabled(true);
    ui->actionDelete_row->setEnabled(false);
    ui->actionDelete_Table->setEnabled(true);

    ui->statusBar->showMessage(tr("Connected to") + " " + databaseFilePath + "...");
}

void MainWindow::addTable()
{
    AddTableDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString tableName = dialog.getTableName();
        QVector<QPair<QString, QString>> columnInfo = dialog.getColumnInfo();
        createTable(tableName, columnInfo);
    }
}

void MainWindow::deleteTable()
{
    QMessageBox::StandardButton confirmation = QMessageBox::question(
        this, tr("Delete Table"),
        tr("Are you sure you want to delete the table '%1'?").arg(selectedTableName),
        QMessageBox::Yes | QMessageBox::No);

    if (confirmation == QMessageBox::No) {
        return;
    }

    if (!selectedTableName.isEmpty()) {
        QSqlDatabase database = QSqlDatabase::database();
        if (!database.isOpen()) {
            if (!database.open()) {
                ui->statusBar->showMessage(tr("Failed to open database"));
                return;
            }
        }

        QSqlQuery query(database);
        database.transaction();

        QString deleteQuery = QString("DROP TABLE IF EXISTS %1").arg(selectedTableName);
        if (!query.exec(deleteQuery)) {
            database.rollback();
            ui->statusBar->showMessage(tr("Failed to delete table: %1").arg(query.lastError().text()));
        } else {
            database.commit();
            ui->statusBar->showMessage(tr("Table deleted successfully"));
            viewTables();
            selectedTableName.clear();
        }
        database.close();
    }
}

void MainWindow::viewTables()
{
    if (!tableListWidget) {
        ui->statusBar->showMessage(tr("Table list widget not initialized"));
        return;
    }

    QSqlDatabase database = QSqlDatabase::database();
    if (!database.isOpen()) {
        if (!database.open()) {
            ui->statusBar->showMessage(tr("Failed to open database"));
            return;
        }
    }

    QSqlQuery query(database);
    if (!query.exec("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name")) {
        qDebug() << "SQL query failed:" << query.lastError().text();
        ui->statusBar->showMessage(tr("Failed to retrieve table information"));
        return;
    }

    tableListWidget->clear();

    while (query.next()) {
        QString tableName = query.value(0).toString();

                // Check if the table exists in the database
        QSqlQuery existsQuery(database);
        QString existsQueryString = QString("SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%1'").arg(tableName);
        if (!existsQuery.exec(existsQueryString) || !existsQuery.next() || existsQuery.value(0).toInt() == 0) {
            continue; // Skip to the next table if it doesn't exist
        }

        QString tableInfo = QString("Table: %1\nColumns: ").arg(tableName);

        QSqlQuery columnQuery(database);
        QString pragmaQuery = QString("PRAGMA table_info(%1)").arg(tableName);
        if (!columnQuery.exec(pragmaQuery)) {
            qDebug() << "PRAGMA query failed:" << columnQuery.lastError().text();
            continue; // Skip to the next table
        }

        while (columnQuery.next()) {
            QString columnName = columnQuery.value(1).toString();
            tableInfo += columnName + ", ";
        }

        tableInfo.chop(2); // Remove the last ", "
        tableListWidget->addItem(tableInfo);
    }

    if (tableListWidget->count() == 0) {
        ui->statusBar->showMessage(tr("No tables found in the database"));
    } else {
        ui->statusBar->showMessage(tr("Tables loaded in the list"));
    }
    database.close();
}

void MainWindow::updateTable()
{
   // Implement the logic to update (modify the structure of) an existing table
   // You can use QSqlQuery and execute the appropriate SQL statements
   // You may also need to create a dialog to get user input for the changes
}

void MainWindow::createNewDb()
{
    const QString newDatabasePath{QFileDialog::getSaveFileName(
        this, QStringLiteral("Create new DB file"), QStringLiteral(""),
        tr("SQLiteDB (*.sqlite3);; All (*.*))"))};

    openDatabaseFile(newDatabasePath);
}

void MainWindow::openExistingDb()
{
    const QString databasePath{QFileDialog::getOpenFileName(
        this, QStringLiteral("Open DB file"), QStringLiteral(""),
        tr("SQLiteDB (*.sqlite3);; All (*.*))"))};

    openDatabaseFile(databasePath);
}

void MainWindow::rowSelectionChanged(const QModelIndex& current,
                                     const QModelIndex& /*previous*/)
{
    ui->actionDelete_row->setEnabled(current.isValid());
}

void MainWindow::deleteRow()
{
    // auto* model{qobject_cast<QSqlTableModel*>(ui->tableView->model())};
    // const int rowToBeRemove{ui->tableView->currentIndex().row()};

    // QSqlDatabase database = QSqlDatabase::database();
    // if (!database.isOpen()) {
    //     if (!database.open()) {
    //         ui->statusBar->showMessage(tr("Failed to open database"));
    //         return;
    //     }
    // }

    // database.transaction();
    // if (model->removeRow(rowToBeRemove)) {
    //     if (database.commit()) {
    //         ui->statusBar->showMessage(tr("Row removed."));
    //     } else {
    //         database.rollback();
    //         ui->statusBar->showMessage(tr("Failed to remove row."));
    //     }
    // } else {
    //     database.rollback();
    //     ui->statusBar->showMessage(tr("Failed to remove row."));
    // }

    // model->select();
    // ui->actionDelete_row->setEnabled(false);
    // database.close();
}

void MainWindow::addRow(QDialog& dialog, QTableView* tableView, const QString& tableName)
{
    // Create a new dialog for entering row data
    QDialog inputDialog(&dialog);
    inputDialog.setWindowTitle("Add Row");

            // Retrieve the column names and types from the table
    QSqlDatabase database = QSqlDatabase::database();
    QSqlQuery columnsQuery(database);
    columnsQuery.exec(QString("PRAGMA table_info(%1)").arg(tableName));

    QVector<QString> columnNames;
    QVector<QString> columnTypes;
    while (columnsQuery.next()) {
        columnNames.append(columnsQuery.value("name").toString());
        columnTypes.append(columnsQuery.value("type").toString().toUpper());
    }

            // Create input fields for each column
    QVector<QWidget*> inputFields;
    for (int i = 0; i < columnNames.size(); ++i) {
        const QString& columnName = columnNames[i];
        const QString& columnType = columnTypes[i];
        if (columnName == "id") {
            QLineEdit* lineEdit = new QLineEdit(&inputDialog);
            inputFields.append(lineEdit);
        } else if (columnName == "age") {
            QSpinBox* spinBox = new QSpinBox(&inputDialog);
            spinBox->setMinimum(1);
            spinBox->setMaximum(100);
            inputFields.append(spinBox);
        } else {
            QLineEdit* lineEdit = new QLineEdit(&inputDialog);
            inputFields.append(lineEdit);
        }
    }

            // Create a form layout and add the input fields
    QFormLayout* formLayout = new QFormLayout();
    for (int i = 0; i < columnNames.size(); ++i) {
        formLayout->addRow(columnNames[i] + ":", inputFields[i]);
    }

            // Create buttons for submitting and canceling
    QPushButton* submitButton = new QPushButton("Submit", &inputDialog);
    QPushButton* cancelButton = new QPushButton("Cancel", &inputDialog);

            // Create a button layout and add the buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(submitButton);
    buttonLayout->addWidget(cancelButton);

            // Create a main layout and add the form layout and button layout
    QVBoxLayout* mainLayout = new QVBoxLayout(&inputDialog);
    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(buttonLayout);

            // Connect the button signals to the appropriate slots
    connect(submitButton, &QPushButton::clicked, [&]() {
        // Get the values from the input fields
        QVector<QVariant> values;
        for (int i = 0; i < inputFields.size(); ++i) {
            const QString& columnType = columnTypes[i];
            if (qobject_cast<QLineEdit*>(inputFields[i])) {
                QString value = qobject_cast<QLineEdit*>(inputFields[i])->text();
                if (!validateType(value, columnType)) {
                    QMessageBox::warning(&dialog, "Invalid Input", QString("Invalid value for column %1 (expected type: %2)").arg(columnNames[i]).arg(columnType));
                    return;
                }
                if (columnType == "INTEGER") {
                    bool ok;
                    int intValue = value.toInt(&ok);
                    if (!ok || intValue < 20 || intValue > 100) {
                        QMessageBox::warning(&dialog, "Invalid Input", QString("Value for column %1 must be in the range 20-100").arg(columnNames[i]));
                        return;
                    }
                }
                values.append(value);
            } else if (qobject_cast<QSpinBox*>(inputFields[i])) {
                values.append(qobject_cast<QSpinBox*>(inputFields[i])->value());
            }
        }

                // Execute an INSERT query to add the row to the database
        QSqlQuery insertQuery(database);
        QString insertQueryString = QString("INSERT INTO %1 (").arg(tableName);
        insertQueryString += columnNames.join(", ");
        insertQueryString += ") VALUES (";
        insertQueryString += QString("?, ").repeated(columnNames.size() - 1) + "?)";
        insertQuery.prepare(insertQueryString);
        for (const QVariant& value : values) {
            insertQuery.addBindValue(value);
        }
        if (!insertQuery.exec()) {
            qDebug() << "Insert query execution failed:" << insertQuery.lastError().text();
            return;
        }

                // Refresh the table view to reflect the changes
        QSqlQuery selectQuery(database);
        selectQuery.exec(QString("SELECT * FROM %1").arg(tableName));
        static_cast<QSqlQueryModel*>(tableView->model())->setQuery(std::move(selectQuery));

                // Close the input dialog
        inputDialog.accept();
    });

    connect(cancelButton, &QPushButton::clicked, [&]() {
        // Close the input dialog without adding a row
        inputDialog.reject();
    });

            // Execute the input dialog
    inputDialog.exec();
}
void MainWindow::deleteRow(QDialog& dialog, QTableView* tableView, const QString& tableName)
{
    // Get the selected row from the table view
    QModelIndex selectedIndex = tableView->currentIndex();
    if (!selectedIndex.isValid())
        return;

    int rowToDelete = selectedIndex.row();

            // Execute a DELETE query to remove the selected row from the database
    QSqlDatabase database = QSqlDatabase::database();
    QSqlQuery deleteQuery(database);
    deleteQuery.prepare(QString("DELETE FROM %1 WHERE rowid = (SELECT rowid FROM %1 LIMIT 1 OFFSET %2)").arg(tableName).arg(rowToDelete));
    if (!deleteQuery.exec()) {
        qDebug() << "Delete query execution failed:" << deleteQuery.lastError().text();
        return;
    }

            // Refresh the table view to reflect the changes
    QSqlQuery selectQuery(database);
    selectQuery.exec(QString("SELECT * FROM %1").arg(tableName));
    static_cast<QSqlQueryModel*>(tableView->model())->setQuery(std::move(selectQuery));
}

void MainWindow::addRow(){
    // todo: remove this method later
}
void MainWindow::addColumn(QDialog& dialog, QTableView* tableView, const QString& tableName)
{
    QSqlTableModel* model = qobject_cast<QSqlTableModel*>(tableView->model());
    if (!model) {
        qDebug() << "Failed to cast model to QSqlTableModel";
        return;
    }

    QDialog inputDialog(&dialog);
    inputDialog.setWindowTitle("Add Column");

    QVBoxLayout* layout = new QVBoxLayout(&inputDialog);

    QLineEdit* columnNameEdit = new QLineEdit(&inputDialog);
    layout->addWidget(new QLabel("Enter column name:", &inputDialog));
    layout->addWidget(columnNameEdit);

    QComboBox* columnTypeComboBox = new QComboBox(&inputDialog);
    columnTypeComboBox->addItems({"INTEGER", "REAL", "TEXT"});
    layout->addWidget(new QLabel("Select column type:", &inputDialog));
    layout->addWidget(columnTypeComboBox);

    QCheckBox* defaultValueCheckBox = new QCheckBox("Set default value", &inputDialog);
    layout->addWidget(defaultValueCheckBox);

    QLineEdit* defaultValueEdit = new QLineEdit(&inputDialog);
    defaultValueEdit->setEnabled(false);
    layout->addWidget(defaultValueEdit);

    connect(defaultValueCheckBox, &QCheckBox::toggled, [defaultValueEdit](bool checked) {
        defaultValueEdit->setEnabled(checked);
    });

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &inputDialog);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &inputDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &inputDialog, &QDialog::reject);

    if (inputDialog.exec() != QDialog::Accepted)
        return;

    QString columnName = columnNameEdit->text();
    QString columnType = columnTypeComboBox->currentText();
    bool hasDefaultValue = defaultValueCheckBox->isChecked();
    QString defaultValue = defaultValueEdit->text();

            // Перевірка типу колонки та значення за замовчуванням після введення даних користувачем
    if (!validateType("", columnType)) {
        QMessageBox::warning(&dialog, "Invalid Column Type", QString("Invalid column type: %1").arg(columnType));
        return;
    }

    if (hasDefaultValue) {
        if (!validateType(defaultValue, columnType)) {
            QMessageBox::warning(&dialog, "Invalid Default Value", QString("Invalid default value for type %1").arg(columnType));
            return;
        }
    }
    QSqlDatabase database = QSqlDatabase::database();
    QSqlQuery alterQuery(database);
    QString alterQueryString = QString("ALTER TABLE %1 ADD COLUMN %2 %3").arg(tableName).arg(columnName).arg(columnType);
    if (hasDefaultValue)
        alterQueryString += QString(" DEFAULT '%1'").arg(defaultValue);
    alterQuery.prepare(alterQueryString);
    if (!alterQuery.exec()) {
        qDebug() << "Alter query execution failed:" << alterQuery.lastError().text();
        return;
    }

    model->insertColumn(model->columnCount());
    model->setHeaderData(model->columnCount() - 1, Qt::Horizontal, columnName);

            // Заблокувати сигнали про зміну моделі
    QSignalBlocker blocker(model);

            // Оновлення даних у таблиці з урахуванням значення за замовчуванням
    QSqlQuery selectQuery(database);
    QString selectQueryString = QString("SELECT * FROM %1").arg(tableName);
    if (!selectQuery.exec(selectQueryString)) {
        qDebug() << "Select query execution failed:" << selectQuery.lastError().text();
        return;
    }

    int rowCount = model->rowCount();
    int columnCount = model->columnCount();
    int newColumnIndex = columnCount - 1;

    while (selectQuery.next()) {
        for (int row = 0; row < rowCount; ++row) {
            QVariant value = selectQuery.value(newColumnIndex);
            if (!value.isValid() && hasDefaultValue) {
                value = defaultValue; // Встановлення значення за замовчуванням
            }
            model->setData(model->index(row, newColumnIndex), value);
        }
    }

            // Розблокувати сигнали про зміну моделі
    blocker.unblock();
}
void MainWindow::deleteColumn(QDialog& dialog, QTableView* tableView, const QString& tableName)
{
    QSqlTableModel* model = qobject_cast<QSqlTableModel*>(tableView->model());
    if (!model) {
        qDebug() << "Failed to cast model to QSqlTableModel";
        return;
    }

    QModelIndexList selectedColumns = tableView->selectionModel()->selectedColumns();
    if (selectedColumns.isEmpty())
        return;

    int columnToDelete = selectedColumns.first().column();
    QString columnName = model->headerData(columnToDelete, Qt::Horizontal).toString();

    QMessageBox::StandardButton confirmation = QMessageBox::question(
        &dialog, "Delete Column", QString("Are you sure you want to delete the column '%1'?").arg(columnName),
        QMessageBox::Yes | QMessageBox::No);

    if (confirmation == QMessageBox::No)
        return;

    QSqlDatabase database = QSqlDatabase::database();
    QSqlQuery alterQuery(database);
    alterQuery.prepare(QString("ALTER TABLE %1 DROP COLUMN %2").arg(tableName).arg(columnName));
    if (!alterQuery.exec()) {
        qDebug() << "Alter query execution failed:" << alterQuery.lastError().text();
        return;
    }

    model->removeColumn(columnToDelete);
}
