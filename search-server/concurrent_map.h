#include <string>
#include <vector>
#include <mutex>
#include <map>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {

private:
    
    struct Bucket {
        std::mutex mutex_;
        std::map<Key, Value> values_;
    };
    
    std::vector<Bucket> bucket_storage;

public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> lock_guard_mutex;
        Value& ref_to_value;
        
        Access (const Key& key, Bucket& bucket) : lock_guard_mutex(bucket.mutex_), ref_to_value(bucket.values_[key]) {}
    };

    explicit ConcurrentMap(size_t bucket_count) : bucket_storage(bucket_count) {}

    Access operator[](const Key& key) {
        Bucket& bucket = bucket_storage[uint64_t(key) % bucket_storage.size()];
        return {key, bucket};
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        
        for (auto& bucket : bucket_storage) {
            std::lock_guard<std::mutex> lock_guard_mutex(bucket.mutex_);
            result.insert(bucket.values_.begin(), bucket.values_.end());
        }
        
        return result;
    }
    
    void erase (const Key& key) {
        Bucket& bucket = bucket_storage[uint64_t(key) % bucket_storage.size()];
        std::lock_guard<std::mutex> lock_guard_mutex(bucket.mutex_);
        bucket.values_.erase(key);
    }
};