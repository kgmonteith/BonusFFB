#include "Handbrake.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Handbrake w;
    w.show();
    return a.exec();
}
