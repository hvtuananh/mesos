// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License

#ifndef __PROCESS_METRICS_METRIC_HPP__
#define __PROCESS_METRICS_METRIC_HPP__

#include <atomic>
#include <memory>
#include <string>

#include <process/future.hpp>
#include <process/owned.hpp>
#include <process/statistics.hpp>
#include <process/timeseries.hpp>

#include <stout/duration.hpp>
#include <stout/option.hpp>
#include <stout/synchronized.hpp>

namespace process {
namespace metrics {

// Describing different metric types.
enum MetricType
{
  UNKNOWN,
  COUNTER,
  GAUGE,
  TIMER,
};

// The base class for Metrics such as Counter and Gauge.
class Metric {
public:
  virtual ~Metric() {}

  virtual Future<double> value() const = 0;

  const std::string& name() const
  {
    return data->name;
  }

  const MetricType type() const
  {
    return data->type;
  }

  Option<Statistics<double>> statistics() const
  {
    Option<Statistics<double>> statistics = None();

    if (data->history.isSome()) {
      synchronized (data->lock) {
        statistics = Statistics<double>::from(*data->history.get());
      }
    }

    return statistics;
  }

protected:
  // Only derived classes can construct.
  Metric(const std::string& name, const Option<Duration>& window)
    : data(new Data(name, window)) {}

  Metric(
      const std::string& name,
      const MetricType type,
      const Option<Duration>& window)
    : data(new Data(name, type, window)) {}

  // Inserts 'value' into the history for this metric.
  void push(double value) {
    if (data->history.isSome()) {
      Time now = Clock::now();

      synchronized (data->lock) {
        data->history.get()->set(value, now);
      }
    }
  }

private:
  struct Data {
    Data(const std::string& _name, const Option<Duration>& window)
      : Data(_name, UNKNOWN, window) {}

    Data(
        const std::string& _name,
        const MetricType _type,
        const Option<Duration>& window)
      : name(_name),
        type(_type),
        history(None())
    {
      if (window.isSome()) {
        history =
          Owned<TimeSeries<double>>(new TimeSeries<double>(window.get()));
      }
    }

    const std::string name;

    const MetricType type;

    std::atomic_flag lock = ATOMIC_FLAG_INIT;

    Option<Owned<TimeSeries<double>>> history;
  };

  std::shared_ptr<Data> data;
};

} // namespace metrics {
} // namespace process {

#endif // __PROCESS_METRICS_METRIC_HPP__
