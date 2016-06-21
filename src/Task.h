#pragma once
#ifndef TASK_H
#define TASK_H

#include <QObject>
#include <QUrl>
#include <QRunnable>

class SearchDialog;

class TaskWorker : public QObject
{
    Q_OBJECT

public:

    struct Status
    {
        enum class State : int
        {
            STATE_UNKNOWN = 0,
            STATE_CREATED_PENDING,
            STATE_STARTED_DOWNLOADING,

            STATE_FINISHED_STATES, // states below this line indicate work finished
            STATE_FINISHED,
            STATE_TEXT_FOUND,
            STATE_DOWNLOAD_ERROR,
            STATE_ABORTED_BY_USER
        };

        State m_state;
        int m_progress;
        int m_taskId;
    };

    TaskWorker(int taskId, const QUrl& url, const QString& searchText, int searchDepth);
    ~TaskWorker();

    void startWork();
    const QUrl& getUrl() const;

signals:

    void downloadRequested(TaskWorker* task);    
    void updated(Status status, QString debugMsg);
    void newTask(QString url, int depth);
    void workDone();

public slots :
 
    void downloadFinished(QString content);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void userAbort();

private:

    void setErrorState();

private:

    QUrl m_url;
    QString m_searchText;
    Status m_status;
    int m_searchDepth;
};

class Task : public QRunnable
{
public:

    Task(SearchDialog* searchDialog, int taskId, const QUrl& url, const QString& searchText, int searchDepth);
    ~Task();

    //QRunnable
    void run() override;

private:

    SearchDialog* m_searchDialog;
    QUrl m_url;
    QString m_searchText;
    int m_taskId;
    int m_searchDepth;
};

#endif // TASK_H
