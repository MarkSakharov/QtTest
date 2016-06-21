#include "MainWindow.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    qApp->setStyleSheet("QProgressBar {\n     border: 0px solid  transparent;\n     border-radius: 0px;\n	background-color: transparent;\n text-align: center; }\n\n QProgressBar::chunk {\n     background-color: #AAAAAA;\n     width: 20px;\n }");

    w.show();

    return a.exec();
}
