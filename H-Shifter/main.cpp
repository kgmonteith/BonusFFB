#include "HShifter.h"
#include "BonusFFB.h"
#include <QtWidgets/QApplication>
#include <QDebug>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    HShifter w;
    w.show();

    BonusFFB* bffb = new BonusFFB();
    bffb->hello();

    w.setSlider();

    return a.exec();
}
