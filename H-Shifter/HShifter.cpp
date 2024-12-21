#include "HShifter.h"

HShifter::HShifter(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

}

void HShifter::setSlider()
{
    ui.horizontalSlider->setValue(50);
    ui.clutchDeviceComboBox->addItem("Fanatec pedals");
    ui.clutchDeviceComboBox->addItem("Moza pedals");
}

HShifter::~HShifter()
{}
