#include "dfa.h"

#include <deque>
#include <list>
#include <unordered_map>

namespace {

using DFAStatePtr = rex::DFAStatePtr;

class DFAHashSet : public std::unordered_set<DFAStatePtr> {
public:
    using HashType = hasher::result_type;

    DFAHashSet() : std::unordered_set<DFAStatePtr>() {}

    void push(const DFAStatePtr &state) {
        auto ret = insert(state);
        if (!ret.second) return;
        auto new_hash = hash_function()(state);
        rex::HashCombile(hash_value_, new_hash);
    }

    auto hash_value() const { return hash_value_; }

private:
    HashType hash_value_;
};

using StateList = std::list<DFAStatePtr>;
using DFAHashMap = std::unordered_map<DFAHashSet::HashType, DFAHashSet>;
using HashSetQueue = std::deque<DFAHashSet>;

DFAHashSet ConvertToHashSet(const std::unordered_set<DFAStatePtr> &set) {
    DFAHashSet hash_set;
    for (const auto &i : set) hash_set.push(i);
    return hash_set;
}

void InsertIntoMap(DFAHashMap &hash_map, DFAHashSet::HashType hash,
        const DFAStatePtr &state) {
    auto it = hash_map.find(hash);
    if (it == hash_map.end()) {
        DFAHashSet hash_set;
        hash_set.push(state);
        hash_map.insert({hash, hash_set});
    }
    else {
        it->second.push(state);
    }
}

DFAHashSet::HashType GetHashByState(const HashSetQueue &queue,
        const DFAStatePtr &state, int size) {
    for (int i = 0; i < size; ++i) {
        const auto &cur_set = queue[i];
        if (cur_set.find(state) != cur_set.end()) {
            return cur_set.hash_value();
        }
    }
    return 0;
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

// a rough implementation of Hopcroft's algorithm
// TODO: optimize
void DFAModel::Simplify() {
    // initial the queue of state sets
    HashSetQueue set_queue;
    set_queue.push_back(ConvertToHashSet(states_));
    set_queue.push_back(ConvertToHashSet(final_states_));
    // store the size of queue for comparison
    int queue_size;
    do {
        queue_size = set_queue.size();
        for (int i = 0; i < queue_size; ++i) {
            const auto &front = set_queue.front();
            // to store new division of state set
            DFAHashMap hash_map;
            for (const auto &symbol : symbols_) {
                // for each symbols in symbol set
                for (const auto &state : front) {
                    DFAStatePtr next = nullptr;
                    // find next state
                    for (const auto &edge : state->out_edges()) {
                        if (edge->symbol()->Equal(symbol.get())) {
                            next = edge->next_state();
                            break;
                        }
                    }
                    // get the hash value of set that includes next state
                    auto hash_val = GetHashByState(set_queue,
                            next, queue_size - i);
                    // insert into hash map
                    InsertIntoMap(hash_map, hash_val, state);
                }
            }
            // push into queue
            for (const auto &it : hash_map) {
                set_queue.push_back(it.second);
            }
            set_queue.pop_front();
        }
    } while (set_queue.size() != queue_size);
    // rebuild the states of DFA
    //
}

void DFAModel::GenerateStateTable() {
    //
}

} // namespace rex
