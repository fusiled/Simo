#include "Simo/core/Context.h"

#include <algorithm>
#include <ranges>
#include <unordered_map>

#include "Simo/core/internal/RadixHeap.h"
#include "Simo/core/Time.h"
#include "Simo/module/Module.h"

namespace Simo {

Context::Context() : nextTickHeap(new Internal::RadixHeap()) {}

bool Context::initialize() {
  // TODO add failure returns upon initialization. Need to propagate the error
  //  to the caller of run* methods, maybe with an expected<>, or by throwing
  //  an exception
  for (auto& [module, params] : modules) {
    if (!module->initialize(*this, *params)) {
      return false;
    }
  }
  // Need to add all the times in order to the heap
  std::vector<Time> all_times;
  for (const auto& time : scheduledTasks | std::views::keys) {
    all_times.push_back(time);
  }
  std::ranges::sort(all_times);
  for (const auto& t : all_times) {
    nextTickHeap->push(t.to_picoseconds());
  }

  state = State::RUNNING;
  return true;
}

void Context::run(const Time& time_delta) {
  const Time final_time = currentTime + time_delta;
  run_at(final_time);
}

void Context::run_at(const Time& final_time) {
  if (state == State::INITIALIZATION) {
    if (!initialize()) {
      SIMO_ASSERT(false && "Failed to initialize the simulation");
    }
  }
  state = State::RUNNING;
  SIMO_ASSERT(final_time > currentTime);
  while (!nextTickHeap->empty()) {
    const uint64_t next_tick = nextTickHeap->peek();
    if (next_tick > final_time.to_picoseconds()) {
      // Next scheduled event is beyond final time.
      // Stop at the desired time
      currentTime = final_time;
      return;
    }
    // Advance time to next tick
    currentTime = Time(next_tick);
    nextTickHeap->pop();
    for (const auto& task : scheduledTasks[currentTime]) {
      task(currentTime);
    }
    scheduledTasks[currentTime].clear();
  }
  state = State::STOPPED;
}

void Context::schedule_at(const Time& time_target,
                          const SimulationCallable& callable) {
  SIMO_ASSERT(state == State::INITIALIZATION || time_target > currentTime);
  scheduledTasks[time_target].emplace_back(callable);
  if (state == State::INITIALIZATION) {
    return;
  }
  if (scheduledTasks[time_target].size() == 1) {
    // New tick to add to the event heap
    nextTickHeap->push(time_target.to_picoseconds());
  }
}

void Context::schedule_at(const Time& time_target,
                          const std::function<void()>& callable) {
  schedule_at(time_target, [callable](const Time) { callable(); });
}

void Context::schedule_in(const Time& time_delta,
                          const SimulationCallable& callable) {
  schedule_at(currentTime + time_delta, callable);
}

void Context::schedule_in(const Time& time_delta,
                          const std::function<void()>& callable) {
  schedule_in(time_delta, [callable](const Time) { callable(); });
}

Context::State Context::get_state() const { return state; }

Time Context::current_time() const {
  return currentTime;
  ;
}

void Context::add(Module& m, Parameters& p) { modules.emplace_back(&m, &p); }

void Context::remove(const Module& m) {
  std::erase_if(modules, [&m](const auto& m_el) { return m_el.first == &m; });
}

}  // namespace Simo