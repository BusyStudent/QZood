#include "logWidget.hpp"
#include "ui_customLabel.h"

class LogWidgetPrivate {
    public:
        LogWidgetPrivate(LogWidget *parent) : self(parent), ui(new Ui::CustomLabel()) {

        }
        ~LogWidgetPrivate() {
            delete ui;
        }

        void setupUi() {
            ui->setupUi(self);

            QWidget::connect(self->timer, &QTimer::timeout, self, [this]() {
                if (logs.size() > 0) {
                    logs.pop_front();
                }
                if (logs.size() > 0) {
                    ui->label->setText(logs.front());
                    self->show();
                    self->hideLater();
                }
                ui->label->setText("");
            });
        }

        void pushLog(const QString& log) {
            // logs.push_back(log);
            // if (self->isHidden()) {
            //     ui->label->setText(log);
            //     self->show();
            //     self->hideLater();
            // }
            ui->label->setText(ui->label->text() + "\n" + log);
            ui->label->resize(ui->label->sizeHint());
            self->resize(self->sizeHint());
            self->show();
            self->stopHideTimer();
            self->hideLater();
        }
    
    public:
        Ui_CustomLabel* ui;
        QStringList logs;
    
    private:
        LogWidget *self;
};

LogWidget::LogWidget(QWidget *parent) : PopupWidget(parent), d(new LogWidgetPrivate(this)) {
    d->setupUi();
}

LogWidget::~LogWidget() {
    delete d;
}

void LogWidget::pushLog(const QString& log) {
    d->pushLog(log);
}