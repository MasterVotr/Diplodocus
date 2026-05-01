#pragma once

#include "io/stats/stats_exporter.h"

namespace diplodocus {

class JsonStatsExporter : public StatsExporter {
   public:
    virtual void ExportOne(const Config& config, const Stats& stats) override;
    // virtual void ExportAll(const Config& config, std::vector<Stats>& stats_all) override;
};

}  // namespace diplodocus
