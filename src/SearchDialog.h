#pragma once
#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include "Task.h"
#include "ui_SearchDialog.h"
#include <QtWidgets/QMainWindow>
#include <QThreadPool>
#include <QNetworkAccessManager>

class SearchDialog : public QDialog
{
    Q_OBJECT

public:

    SearchDialog(QWidget* parent = nullptr);
    ~SearchDialog();

signals:

    void userAbort();

public slots:

    void addTask(QString url, int depth);
    void downloadPage(TaskWorker* task);
    void downloadFinished(QNetworkReply* reply);
    void updateTaskStatus(TaskWorker::Status status, QString debugMsg);
    
    void startSearch();
    void stopSearch();
    void resetSearch();
    
private:

    void setTasksCounts(int finishedTasks, int totalTasks);
    void closeEvent(QCloseEvent* e) override;

private:

    static const int ROW_N = 0;
    static const int ROW_URL = 1;
    static const int ROW_PROGRESS = 2;
    static const int ROW_STATUS = 3;

    QThreadPool m_threadPool;
    QNetworkAccessManager m_networkAccessManager;
    QString m_searchText;
    QSet<QString> m_processedURLs;
    int m_totalTasks, m_finishedTasks;
    bool m_running;
    Ui::SearchDialogUI m_ui;
};

#endif // SEARCHDIALOG_H
