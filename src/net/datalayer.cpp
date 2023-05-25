#include <QApplication>
#include "datalayer.hpp"

class DataServicePrivate : public DataService, public QObject {
    public:
        DataServicePrivate(QObject *parent) : QObject(parent) { }
};


static DataServicePrivate *dataService = nullptr;

void InitializeDataService() {
    dataService = new DataServicePrivate(qApp);
}