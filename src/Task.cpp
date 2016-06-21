#include "Task.h"

#include "SearchDialog.h"
#include <QEventLoop>
#include <regex>
#include <cassert>

TaskWorker::TaskWorker(int taskId, const QUrl& url, const QString& searchText, int searchDepth)
    : m_url(url)
    , m_searchText(searchText)
    , m_searchDepth(searchDepth)
{
    m_status.m_state = m_url.isEmpty() ? Status::State::STATE_UNKNOWN : Status::State::STATE_CREATED_PENDING;
    m_status.m_progress = 0;
    m_status.m_taskId = taskId;    
}

TaskWorker::~TaskWorker()
{

}

void TaskWorker::startWork()
{
    if(m_status.m_state != Status::State::STATE_CREATED_PENDING)
    {
        setErrorState();

        return;
    }

    m_status.m_state = Status::State::STATE_STARTED_DOWNLOADING;
    emit downloadRequested(this);    
    emit updated(m_status, "startWork");
}

void TaskWorker::downloadFinished(QString content)
{
    //QNetworkReply* reply = static_cast<QNetworkReply*>(sender());  Spontaneous invalid pointers + reply->readAll() not thread safe?

    if(m_status.m_state > Status::State::STATE_FINISHED_STATES)
    {
        return;
    }
    
    if(m_status.m_state != Status::State::STATE_STARTED_DOWNLOADING)
    {
        setErrorState();

        return;
    }
    
    //do work

    /*volatile float sum = 0.0f;    // dummy load test

    for(int k = 0; k < 1000000; k++)
    for(int i = 0; i < 1000; i++)
    {
        sum += sinf((float)i);
    }*/

    if(content.isEmpty())
    {
        m_status.m_state = Status::State::STATE_DOWNLOAD_ERROR;
    }
    else
    {
        if(content.contains(m_searchText))
        {
            m_status.m_state = Status::State::STATE_TEXT_FOUND;
        }
        else
        {
            m_status.m_state = Status::State::STATE_FINISHED;
        }

        if(m_searchDepth > 0)
        {
            std::string ss(content.toStdString());
            std::regex rx(R"__(\b(([\w-]+://?|www[.])[^\s()<>]+(?:\([\w\d]+\)|([^[:punct:]\s]|/))))__");
            auto words_begin = std::sregex_iterator(ss.begin(), ss.end(), rx);
            auto words_end = std::sregex_iterator();
            //int nn = std::distance(words_begin, words_end);

            for(std::sregex_iterator i = words_begin; i != words_end; ++i)
            {
                emit newTask(QString(i->str().c_str()), m_searchDepth - 1);
            }
        }
    }

    emit updated(m_status, "downloadFinished");
    emit workDone();
}

void TaskWorker::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if(m_status.m_state != Status::State::STATE_STARTED_DOWNLOADING)
    {
        return;
    }

    m_status.m_progress = bytesTotal > 0 ? bytesReceived * 100 / bytesTotal : 0;
    emit updated(m_status, "downloadProgress");
}

void TaskWorker::userAbort()
{
    if(m_status.m_state > Status::State::STATE_FINISHED_STATES)
    {
        return;
    }

    m_status.m_state = Status::State::STATE_ABORTED_BY_USER;
    emit updated(m_status, "userAbort");
    emit workDone();
}

const QUrl& TaskWorker::getUrl() const
{
    return m_url;
}

void TaskWorker::setErrorState()
{
    assert(0);

    m_status.m_state = Status::State::STATE_UNKNOWN;
    emit updated(m_status, "STATE_UNKNOWN");
    emit workDone();
}

Task::Task(SearchDialog* searchDialog, int taskId, const QUrl& url, const QString& searchText, int searchDepth)
    : m_searchDialog(searchDialog)
    , m_url(url)
    , m_searchText(searchText)
    , m_taskId(taskId)
    , m_searchDepth(searchDepth)
{
    assert(m_searchDialog != nullptr);
    assert(!m_url.isEmpty());
    assert(!m_searchText.isEmpty());
    assert(m_searchDepth >= 0);

    setAutoDelete(true);
}

Task::~Task()
{

}

void Task::run()
{
    TaskWorker taskWorker(m_taskId, m_url, m_searchText, m_searchDepth);    
    QEventLoop eventLoop;

    QObject::connect(&taskWorker, &TaskWorker::downloadRequested, m_searchDialog, &SearchDialog::downloadPage, Qt::ConnectionType::QueuedConnection);
    QObject::connect(&taskWorker, &TaskWorker::updated, m_searchDialog, &SearchDialog::updateTaskStatus, Qt::ConnectionType::QueuedConnection);
    QObject::connect(&taskWorker, &TaskWorker::newTask, m_searchDialog, &SearchDialog::addTask, Qt::ConnectionType::QueuedConnection);
    QObject::connect(&taskWorker, &TaskWorker::workDone, &eventLoop, &QEventLoop::quit, Qt::ConnectionType::QueuedConnection);
    QObject::connect(m_searchDialog, &SearchDialog::userAbort, &taskWorker, &TaskWorker::userAbort, Qt::ConnectionType::QueuedConnection);

    taskWorker.startWork();

    // QThread::currentThread()->exec();  - protected, can't call here
    // QEventLoop must be exec() in this run() function, because QThreadPoolThread::run() doesn't do this like default QThread::run() does.
    eventLoop.exec();
}
