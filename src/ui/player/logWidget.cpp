#include "logWidget.hpp"

#include <QFontMetrics>

class LogWidgetPrivate {
    public:
        LogWidgetPrivate(LogWidget *parent) : self(parent), ui(new Ui::CustomLabel()) {

        }
        ~LogWidgetPrivate() { }

        void setupUi() {
            ui->setupUi(self);

            QWidget::connect(self->mTimer, &QTimer::timeout, self, [this]() {
                if (logs.size() > 0) {
                    logs.pop_front();
                }
                if (logs.size() > 0) {
                    ui->label->setText(logs.front());
                    self->show();
                    self->hideLater();
                }
                ui->label->setText("");
                count = 0;
            });
        }

        void pushLog(const QString& log) {
            // 附加当前消息到最后一条。
            ui->label->setText((ui->label->text().isEmpty() ? "" : (ui->label->text() + "\n")) + log);
            count ++;
            // 超过10条后开始删除最早的消息。
            if (count > 10) {
                int index = ui->label->text().indexOf("\n");
                ui->label->setText(ui->label->text().mid(index + 1));
                count --;
            }
            // 计算当前消息文本的大小。
            QFontMetrics metrics(ui->label->font());
            QSize size = metrics.boundingRect(QRect(0, 0, 200, 200), Qt::TextWordWrap, ui->label->text()).size();
            // 设置到显示消息的标签上。
            ui->label->resize(size + QSize(20, 20));
            self->resize(ui->label->size());
            // 重新唤起展示标签刷新位置和显示时长。
            self->hide();
            self->show();
            self->stopHideTimer();
            self->hideLater();
        }
    
    public:
        QScopedPointer<Ui_CustomLabel> ui;
        QStringList logs;
        int count = 0;
    
    private:
        LogWidget *self;
};

LogWidget::LogWidget(QWidget *parent) : PopupWidget(parent), d(new LogWidgetPrivate(this)) {
    d->setupUi();
}

LogWidget::~LogWidget() { }

void LogWidget::pushLog(const QString& log) {
    d->pushLog(log);
}