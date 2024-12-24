#pragma once

#include <QtWidgets/QMainWindow>
#include <QTimer>
#include "ui_HShifter.h"
#include "BonusFFB.h"
#include "Telemetry.h"

class HShifter : public QMainWindow
{
    Q_OBJECT

public:
    HShifter(QWidget *parent = nullptr);
    ~HShifter();
    void initializeBonusFFB();
    void setSlider();


private:
    Ui::HShifterClass ui;
    BonusFFB bffb;
    QTimer telemetryTimer;
    Telemetry telemetry;
};
