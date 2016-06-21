#include "MainWindow.h"

#include "SearchDialog.h"
#include <QTimer>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_ui.setupUi(this);
    m_searchDialog = std::make_unique<SearchDialog>(this);

    QTimer::singleShot(0, this, &MainWindow::openSearchDialog);
}

MainWindow::~MainWindow()
{

}

void MainWindow::openSearchDialog()
{
    m_searchDialog->exec();
}
