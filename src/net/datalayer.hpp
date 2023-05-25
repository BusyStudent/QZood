#include "promise.hpp"
#include "client.hpp"

class Bangumi : public VideoList {
    
};

/**
 * @brief Pure virtual interface for access the data structure
 * 
 */
class DataService {
    public:
        // virtual NetPromise<QStringList> fetchData() = 0;
    protected:
        DataService() = default;
        ~DataService() = default;
};

DataService *GetDataService();
void         InitializeDataService();