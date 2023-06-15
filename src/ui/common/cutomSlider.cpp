#include "customSlider.hpp"

#include <QStylePainter>
#include <QStyleOptionSlider>
#include <QMouseEvent>
#include <QToolTip>

class CustomSliderPrivate {
    public:
        CustomSliderPrivate(CustomSlider* parent) : parent(parent), tipLabel(new QLabel(parent)) {
            setupUi();
        }

        void setupUi() {
            tipLabel->setWindowFlags(Qt::ToolTip | Qt::BypassGraphicsProxyWidget);
            tipLabel->setFocusPolicy(Qt::FocusPolicy::NoFocus);
            tipLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
            tipLabel->setStyleSheet(R"(
                QLabel{
                    border: 1px solid #C0C0C0;
                }
            )");
            tipLabel->setMinimumWidth(20);
            tipLabel->setAlignment(Qt::AlignCenter);
            tipLabel->hide();
        }
    
        void updateGrooveRect() {
            QStyleOptionSlider opt;
            parent->initStyleOption(&opt);
            grooveRect = parent->style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, parent);
        }

    private:
        int preloadValue = 0;
        CustomSlider* parent;
        QLabel* tipLabel = nullptr;
        bool flagShowTip = true;
        QRect grooveRect;

    friend class CustomSlider;
};


CustomSlider::CustomSlider(QWidget *parent)
    : QSlider(parent), d(new CustomSliderPrivate(this)) {
    setOrientation(Qt::Horizontal);
    setMinimum(0);
    setMaximum(100);
}

void CustomSlider::setShowTipLabel(bool flag) {
    d->flagShowTip = flag;
}

bool CustomSlider::showTipLabel() {
    return d->flagShowTip;
}

void CustomSlider::setTipLable(QLabel* label) {
    d->tipLabel->setParent(nullptr);
    delete d->tipLabel;
    d->tipLabel = label;
    label->setParent(this);
    d->setupUi();
}

void CustomSlider::setPreloadValue(int value) {
    value = std::clamp(value, minimum(), maximum());
    d->preloadValue = value;
    update();
    emit preloadValueChange(value);
}

int CustomSlider::preloadValue() {
    return d->preloadValue;
}

void CustomSlider::mousePressEvent(QMouseEvent *ev) {
    if (ev->button() == Qt::MouseButton::LeftButton) {
        if (d->grooveRect.contains(ev->pos())) {
            auto sliderPos = mapToGlobal(QPoint{ev->pos().x(), d->grooveRect.y()});
            int value = (int)((double)(ev->pos().x() - d->grooveRect.x()) / (d->grooveRect.width()) * (maximum() - minimum()));
            
            setValue(value);
            emit sliderMoved(value);
        }
    }
    
    QSlider::mousePressEvent(ev);
}
void CustomSlider::mouseMoveEvent(QMouseEvent *ev) {
    if (d->flagShowTip && d->grooveRect.contains(ev->pos())) {
        auto sliderPos = mapToGlobal(QPoint{ev->pos().x(), d->grooveRect.y()});
        int value = 0;
        if (orientation() == Qt::Orientation::Horizontal) {
            value = (int)((double)(ev->pos().x() - d->grooveRect.x()) / (d->grooveRect.width()) * (maximum() - minimum()));
            d->tipLabel->setNum(value);
            emit tipBeforeShow(d->tipLabel, value);
            QPoint anchorPoint{d->tipLabel->sizeHint().width() /  2, d->tipLabel->sizeHint().height()};
            d->tipLabel->move(sliderPos - anchorPoint - QPoint(0, 10));
        } else {
            value = (int)((double)(ev->pos().y() - d->grooveRect.y()) / (d->grooveRect.height()) * (maximum() - minimum()));
            d->tipLabel->setNum(value);
            emit tipBeforeShow(d->tipLabel, value);
            QPoint anchorPoint{d->tipLabel->sizeHint().width(), d->tipLabel->sizeHint().height() / 2};
            d->tipLabel->move(sliderPos - anchorPoint - QPoint(10, 0));
        }
        d->tipLabel->show();

        emit tipShowed(d->tipLabel, value);
    } else {
        d->tipLabel->hide();
    }

    QSlider::mouseMoveEvent(ev);
}

void CustomSlider::paintEvent(QPaintEvent *ev){
    QSlider::paintEvent(ev);

    QStylePainter painter(this);
    painter.setPen(Qt::NoPen);
    
    QRect loadRect;
    if (orientation() == Qt::Orientation::Horizontal) {
        int sliderPosition = style()->sliderPositionFromValue(minimum(), maximum(), value(), d->grooveRect.width());
        int loadWidth = d->grooveRect.width() * (double)preloadValue() / (double)(maximum() - minimum()) - d->grooveRect.width();
        loadRect = d->grooveRect.adjusted(sliderPosition + 3, 1, loadWidth, -1);
    } else {
        int sliderPosition = style()->sliderPositionFromValue(minimum(), maximum(), value(), d->grooveRect.height());
        int loadHeight = d->grooveRect.height() * (double)preloadValue() / (double)(maximum() - minimum()) - d->grooveRect.height();
        loadRect = d->grooveRect.adjusted(1, sliderPosition + 3, -1, loadHeight);
    }

    painter.setBrush(QColor(255, 255, 255, 100));
    painter.drawRect(loadRect);
}

void CustomSlider::resizeEvent(QResizeEvent* ev) {
    d->updateGrooveRect();

    QSlider::resizeEvent(ev);
}

CustomSlider::~CustomSlider() {
    delete d;
}
