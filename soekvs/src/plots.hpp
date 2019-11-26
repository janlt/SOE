#ifndef SOE_KVS_SRC_PLOTS_H_
#define SOE_KVS_SRC_PLOTS_H_

#include <string>

namespace LzKVStore {

class Plots
{
public:
    Plots()
    {
    }
    ~Plots()
    {
    }

    void Clear();
    void Add(double value);
    void Merge(const Plots& other);

    std::string ToString() const;

private:
    double min_;
    double max_;
    double num_;
    double sum_;
    double sum_squares_;

    enum {
        kNumBuckets = 154
    };
    static const double kBucketLimit[kNumBuckets];
    double buckets_[kNumBuckets];

    double Median() const;
    double Percentile(double p) const;
    double Average() const;
    double StandardDeviation() const;
};

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_PLOTS_H_
