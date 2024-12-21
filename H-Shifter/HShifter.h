#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_HShifter.h"

class HShifter : public QMainWindow
{
    Q_OBJECT

public:
    HShifter(QWidget *parent = nullptr);
    ~HShifter();
    void setSlider();

private:
    Ui::HShifterClass ui;
};
