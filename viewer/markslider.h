#ifndef __MARKSLIDER_H__
#define __MARKSLIDER_H__

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QVector>

class MarkSlider : public QWidget
{
    Q_OBJECT

public:
    explicit MarkSlider(QWidget* parent = 0);
    ~MarkSlider();

    const QSlider* getSlider() const { return slider; }
    void setRange(int beg, int end);
    void setValue(int val);

public slots:
    void putLabels(int curr);

protected:
    virtual void resizeEvent(QResizeEvent* e);
    void initLabels();

private:
    static const int sliderWidthOffset = 12;
    static const int edge2tick = 0;
    static const int lastMarkOffset = 3;

    int tickInterval;
    QSlider* slider;
    QLabel* labelBeg;
    QLabel* labelEnd;
    QLabel* labelCur;

    int nSliderNum() const;
    int nSliderTicks() const;
    int sliderMarkWidth() const;
};

#endif // __MARKSLIDER_H__
