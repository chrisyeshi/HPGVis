#include "markslider.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <iostream>

MarkSlider::MarkSlider(QWidget* parent)
  : QWidget(parent)
{
    slider = new QSlider(Qt::Horizontal, this);
    slider->setTracking(false);
    slider->setTickInterval(5);
    slider->setTickPosition(QSlider::TicksBelow);
    connect(slider, SIGNAL(sliderMoved(int)), this, SLOT(putLabels(int)));
    connect(slider, SIGNAL(valueChanged(int)), this, SLOT(putLabels(int)));

    initLabels();
    putLabels(slider->value());

    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->addWidget(labelBeg);
    topLayout->addWidget(slider);
    topLayout->addWidget(labelEnd);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(5, 0, 5, 0);
    layout->addLayout(topLayout);
    layout->addSpacing(30);

    this->setLayout(layout);
}

MarkSlider::~MarkSlider()
{

}

void MarkSlider::setRange(int beg, int end)
{
    slider->setRange(beg, end);
    labelBeg->setText(QString::number(beg));
    labelEnd->setNum(end);
}

void MarkSlider::setValue(int val)
{
    slider->setValue(val);
}

void MarkSlider::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    putLabels(slider->value());
}

void MarkSlider::initLabels()
{
    labelBeg = new QLabel(QString::number(0), this);
    labelEnd = new QLabel(QString::number(99), this);
    labelCur = new QLabel(QString::number(0), this);

    labelBeg->setAlignment(Qt::AlignCenter);
    labelEnd->setAlignment(Qt::AlignCenter);
    labelCur->setAlignment(Qt::AlignHCenter);
}

void MarkSlider::putLabels(int curr)
{
    labelCur->setText(QString::number(curr));

    int begOffset = labelBeg->width() + 14;
    int currPos = float(slider->width()) / (slider->maximum() - slider->minimum()) * curr;
    int labelHalf = labelCur->width() * 0.5;
    int x = begOffset + currPos - labelHalf;
    int y = slider->height();
    labelCur->move(x, y);
}

int MarkSlider::nSliderNum() const
{
    return slider->maximum() - slider->minimum() + 1;
}

int MarkSlider::nSliderTicks() const
{
    return nSliderNum() / slider->tickInterval();
}

int MarkSlider::sliderMarkWidth() const
{
    return slider->width() - edge2tick * 2;
}
