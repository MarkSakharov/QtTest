#pragma once
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include "ui_MainWindow.h"
#include <memory>

class SearchDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

public slots:

    void openSearchDialog();

private:

    std::unique_ptr<SearchDialog> m_searchDialog;
    Ui::MainWindowClass m_ui;
};

#endif // MAINWINDOW_H
