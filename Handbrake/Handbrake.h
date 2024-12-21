#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Handbrake.h"

class Handbrake : public QMainWindow
{
    Q_OBJECT

public:
    Handbrake(QWidget *parent = nullptr);
    ~Handbrake();

private:
    Ui::HandbrakeClass ui;
};
