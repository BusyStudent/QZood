#include <QApplication>
#include "datalayer.hpp"
#include "bilibili.hpp"

namespace {
    
class DataServicePrivate : public DataService {
    public:
        DataServicePrivate(QObject *parent) {
            setParent(parent);
            InitializeVideoInterface();
        }

        NetResult<Timeline> fetchTimeline() override {
            if (GetVideoInterfaceList().empty()) {
                return NetResult<Timeline>::Alloc().putLater(std::nullopt);
            }
            return GetVideoInterfaceList()[0]->fetchTimeline();
        }
        NetResult<BangumiList> searchBangumi(const QString &what) override {
            return NetResult<BangumiList>::Alloc().putLater(std::nullopt);
        }
        QString name() override {
            return "DataService";
        }


        QMap<QString, QString> strDict; //< Map 
        BiliClient             biliClient;
};

static DataServicePrivate *dataService = nullptr;




}


DataService *DataService::instance() {
    if (!dataService) {
        dataService = new DataServicePrivate(qApp);
    }
    return dataService;
}
