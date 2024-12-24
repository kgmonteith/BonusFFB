#include "HShifter.h"
#include <QtWidgets/QApplication>
#include <QDebug>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("Fusion");

    HShifter w;
    w.show();

    w.initializeBonusFFB();
    w.setSlider();

    return a.exec();
}
