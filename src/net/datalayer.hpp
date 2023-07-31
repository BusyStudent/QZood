#include "../common/promise.hpp"
#include <QDateTime>

#include "../common/danmaku.hpp"
#include "client.hpp"


/**
 * @brief Pure virtual interface for access the data structure
 * 
 */
class DataService : public VideoInterface {
    public:
        static DataService *instance();
};
