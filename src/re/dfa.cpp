#include "dfa.h"

#include <iostream>
#include <iomanip>
#include <queue>
#include <unordered_set>
#include <algorithm>
#include <map>

namespace {

std::map<void *, int> id_map;
int cur_id = 0;

void PrintPointer(void *ptr) {
    auto it = id_map.find(ptr);
    if (it != id_map.end()) {
        std::cout << it->second;
    }
    else {
        auto it = id_map.insert({ptr, cur_id++});
        std::cout << it.first->second;
    }
}

} // namespace

namespace rex {

bool DFAModel::TestString(const std::string &str) {
    auto state = initial_;
    for (const auto &c : str) {
        bool switch_flag = false;
        for (const auto &edge : state->out_edges()) {
            if (edge->symbol()->TestChar(c)) {
                state = edge->next_state();
                switch_flag = true;
                break;
            }
        }
        if (!switch_flag) return false;
    }
    return final_states_.find(state) != final_states_.end();
}

void DFAModel::GenerateStateTable() {
    std::queue<DFAStatePtr> state_queue;
    int cur_id = 0;
    std::unordered_set<DFAStatePtr> visit;
    state_queue.push(initial_);
    do {
        auto cur_state = state_queue.front();
        state_queue.pop();
        if (!visit.insert(cur_state).second) continue;
        std::cout << "state: ";
        PrintPointer(cur_state.get());
        auto it = std::find(final_states_.begin(), final_states_.end(), cur_state);
        if (it != final_states_.end()) std::cout << " (final)";
        std::cout << std::endl;
        for (const auto &edge : cur_state->out_edges()) {
            auto next = edge->next_state();
            std::cout << "  symbol(" << edge->symbol() << "): ";
            PrintPointer(next.get());
            std::cout << std::endl;
            state_queue.push(next);
        }
    } while (!state_queue.empty());
}

} // namespace rex
