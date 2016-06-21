#include "SearchDialog.h"

#include <QCloseEvent>
#include <QThread>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTableWidgetItem>
#include <QProgressBar>
#include <QMessageBox>
#include <QScrollBar>
#include <cassert>

SearchDialog::SearchDialog(QWidget* parent)
    : QDialog(parent)
    , m_running(false)
    , m_totalTasks(0)
    , m_finishedTasks(0)
{
    qRegisterMetaType<TaskWorker*>("TaskWorker*");
    qRegisterMetaType<TaskWorker::Status>("Status");

    m_ui.setupUi(this);
    m_ui.sbThreadsCount->setValue(std::max(QThread::idealThreadCount(), 1));
    m_ui.tbLog->setColumnWidth(ROW_N, 40);
    m_ui.tbLog->setColumnWidth(ROW_URL, 550);

    connect(&m_networkAccessManager, &QNetworkAccessManager::finished, this, &SearchDialog::downloadFinished);
}

SearchDialog::~SearchDialog()
{

}

void SearchDialog::addTask(QString url, int depth)
{
    if(!m_running)
    {
        return;
    }

    if(m_processedURLs.contains(url)) //prevent links recursion
    {
        return;
    }

    m_processedURLs.insert(url);

    int row = m_ui.tbLog->rowCount();

    m_ui.tbLog->setRowCount(row + 1);
    m_ui.tbLog->setItem(row, ROW_N, new QTableWidgetItem(QString::number(row + 1)));
    m_ui.tbLog->setItem(row, ROW_URL, new QTableWidgetItem(url));
    m_ui.tbLog->setCellWidget(row, ROW_PROGRESS, new QProgressBar(m_ui.tbLog));
    m_ui.tbLog->setItem(row, ROW_STATUS, new QTableWidgetItem("Queued..."));
    setTasksCounts(m_finishedTasks, m_totalTasks + 1);

    m_threadPool.start(new Task(this, row, url, m_searchText, depth));
}

void SearchDialog::downloadPage(TaskWorker* taskWorker)
{
    if(!m_running)
    {
        return;
    }

    QNetworkReply* reply = m_networkAccessManager.get(QNetworkRequest(taskWorker->getUrl()));

    reply->setProperty("Task", QVariant::fromValue(taskWorker));
    connect(this, &SearchDialog::userAbort, reply, &QNetworkReply::abort, Qt::ConnectionType::QueuedConnection);
    connect(reply, &QNetworkReply::downloadProgress, taskWorker, &TaskWorker::downloadProgress);
}

void SearchDialog::downloadFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if(!m_running)
    {
        return;
    }

    QVariant task = reply->property("Task");

    if(!task.data_ptr().is_null)
    {
        QString content;
        
        if(reply->error() == QNetworkReply::NetworkError::NoError)
        {
            content = reply->readAll();
        }

        QMetaObject::invokeMethod(task.value<TaskWorker*>(), "downloadFinished", Qt::ConnectionType::QueuedConnection, QGenericReturnArgument("void"), Q_ARG(QString, content));
    }
}

void SearchDialog::setTasksCounts(int finishedTasks, int totalTasks)
{
    m_finishedTasks = finishedTasks;
    m_totalTasks = totalTasks;

    if(m_totalTasks == 0)
    {
        m_ui.lbProgress->setText("Progress log :");
    }
    else
    {
        m_ui.lbProgress->setText("Progress log (" + QString::number(m_finishedTasks) + "/" + QString::number(m_totalTasks) + "):");
    }
}

void SearchDialog::closeEvent(QCloseEvent* e)
{
    //event->accept();
    //e->ignore();

    stopSearch();
    m_threadPool.waitForDone();
}

void SearchDialog::startSearch()
{
    if(m_running)
    {
        QMessageBox::warning(this, "", "Search already running. Try stop button.");

        return;
    }

    if(m_ui.edURL->text().isEmpty())
    {
        QMessageBox::warning(this, "", "Search URL is empty.");

        return;
    }

    if(m_ui.txSearchText->toPlainText().isEmpty())
    {
        QMessageBox::warning(this, "", "Search text is empty.");

        return;
    }

    resetSearch();
    m_threadPool.waitForDone(); //sanity check

    m_searchText = m_ui.txSearchText->toPlainText();
    m_threadPool.setMaxThreadCount(m_ui.sbThreadsCount->value());
    m_running = true;

    addTask(m_ui.edURL->text(), m_ui.sbSearchDepth->value());
}

void SearchDialog::stopSearch()
{
    m_running = false;
    emit userAbort();
    m_threadPool.clear();
}

void SearchDialog::resetSearch()
{
    stopSearch();

    m_processedURLs.clear();
    m_ui.txResults->clear();    
    setTasksCounts(0, 0);

    m_ui.tbLog->setUpdatesEnabled(false);

    while(m_ui.tbLog->rowCount() > 0)
    {
        m_ui.tbLog->removeRow(0);
    }

    m_ui.tbLog->setUpdatesEnabled(true);
}

void SearchDialog::updateTaskStatus(TaskWorker::Status status, QString debugMsg)
{
    if(status.m_taskId < 0 || status.m_taskId >= m_ui.tbLog->rowCount())
    {
        return;
    }

    int row = status.m_taskId;

    switch(status.m_state)
    {
    case TaskWorker::Status::State::STATE_STARTED_DOWNLOADING:
    {
        m_ui.tbLog->item(row, ROW_STATUS)->setText("DOWNLOADING...");
        static_cast<QProgressBar*>(m_ui.tbLog->cellWidget(row, ROW_PROGRESS))->setValue(status.m_progress);

        break;
    }

    case TaskWorker::Status::State::STATE_TEXT_FOUND:
    {
        m_ui.tbLog->item(row, ROW_STATUS)->setText("TEXT_FOUND.");
        m_ui.tbLog->item(row, ROW_STATUS)->setBackgroundColor({ 100, 255, 100 });
        static_cast<QProgressBar*>(m_ui.tbLog->cellWidget(row, ROW_PROGRESS))->setValue(100);
        m_ui.txResults->append(m_ui.tbLog->item(row, ROW_URL)->text());
        m_ui.txResults->verticalScrollBar()->setSliderPosition(m_ui.txResults->verticalScrollBar()->maximum());

        break;
    }

    case TaskWorker::Status::State::STATE_FINISHED:
    {
        m_ui.tbLog->item(row, ROW_STATUS)->setText("FINISHED.");
        m_ui.tbLog->item(row, ROW_STATUS)->setBackgroundColor({ 200, 255, 200 });
        static_cast<QProgressBar*>(m_ui.tbLog->cellWidget(row, ROW_PROGRESS))->setValue(100);

        break;
    }

    case TaskWorker::Status::State::STATE_UNKNOWN: //pass through
    case TaskWorker::Status::State::STATE_DOWNLOAD_ERROR:
    {
        m_ui.tbLog->item(row, ROW_STATUS)->setText("DOWNLOAD_ERROR");
        m_ui.tbLog->item(row, ROW_STATUS)->setBackgroundColor({255, 150, 150});
        static_cast<QProgressBar*>(m_ui.tbLog->cellWidget(row, ROW_PROGRESS))->setValue(0);

        break;
    }

    case TaskWorker::Status::State::STATE_ABORTED_BY_USER:
    {
        m_ui.tbLog->item(row, ROW_STATUS)->setText("ABORTED_BY_USER");
        m_ui.tbLog->item(row, ROW_STATUS)->setBackgroundColor({ 255, 150, 150 });
        static_cast<QProgressBar*>(m_ui.tbLog->cellWidget(row, ROW_PROGRESS))->setValue(0);

        break;
    }

    default:
    {
        assert(0);
    }
    }

    if(status.m_state > TaskWorker::Status::State::STATE_FINISHED_STATES)
    {
        setTasksCounts(m_finishedTasks + 1, m_totalTasks);

        if(m_finishedTasks == m_totalTasks)
        {
            m_running = false;
        }
    }
}
