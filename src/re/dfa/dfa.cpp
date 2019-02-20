#include <re/dfa/dfa.h>
#include <re/util/util.h>

#include <deque>
#include <unordered_map>
#include <utility>

#if NDEBUG
#else
#include <iostream>
#endif

namespace {

// re-define types in 'namespace rex'
using DFAStatePtr = rex::re::DFAStatePtr;
using DFAStateSet = std::unordered_set<DFAStatePtr>;

using PosHash = std::size_t;
using DFAHashMap = std::unordered_map<PosHash, DFAStateSet>;
using SetQueue = std::deque<DFAStateSet>;
using StateMap = std::unordered_map<DFAStatePtr, DFAStatePtr>;
using EdgeMap = std::unordered_map<rex::re::SymbolPtr, rex::re::DFAEdgePtr>;

// insert state into hash map
inline void InsertIntoMap(DFAHashMap &hash_map, PosHash hash,
        const DFAStatePtr &state) {
    auto ret = hash_map.insert({hash, {state}});
    if (!ret.second) {
        ret.first->second.insert(state);
    }
}

// get the position of state in set queue for hashing
int GetPosByState(const SetQueue &queue,
        const DFAStatePtr &state, int size) {
    if (state) {
        for (int i = 0; i < size; ++i) {
            const auto &cur_set = queue[i];
            if (cur_set.find(state) != cur_set.end()) {
                return i;
            }
        }
    }
    return -1;
}

// part of DFA simplify algorithm
void GetDivision(SetQueue &set_queue, const rex::re::SymbolSet &symbols) {
    // store the size of queue for comparison
    int queue_size;
    // get the divisions of current DFA states
    do {
        queue_size = set_queue.size();
        for (int i = 0; i < queue_size; ++i) {
            const auto &front = set_queue.front();
            if (front.size() <= 1) {
                // cannot be split
                set_queue.push_back(front);
                set_queue.pop_front();
                continue;
            }
            // to store new divisions of state set
            DFAHashMap hash_map;
            for (const auto &state : front) {
                PosHash hash_val = 0;
                for (const auto &symbol : symbols) {
                    // for each symbols in symbol set
                    DFAStatePtr next = nullptr;
                    // find next state
                    for (const auto &edge : state->out_edges()) {
                        if (edge->symbol()->Equal(symbol.get())) {
                            next = edge->next_state();
                            break;
                        }
                    }
                    // get the position of set that includes next state
                    rex::re::HashCombile(hash_val, GetPosByState(set_queue,
                            next, queue_size - i));
                }
                // insert into hash map
                InsertIntoMap(hash_map, hash_val, state);
            }
            // push into queue
            for (const auto &it : hash_map) {
                set_queue.push_back(it.second);
            }
            set_queue.pop_front();
        }
    } while (set_queue.size() != queue_size);
}

// get the state map for mapping old states to new states
StateMap GetStateMap(const SetQueue &set_queue, const DFAStateSet &finals,
        const DFAStatePtr &initial, DFAStatePtr &init_ptr,
        DFAStateSet &states, DFAStateSet &final_states) {
    StateMap state_map;
    for (const auto &state_set : set_queue) {
        auto cur_state = std::make_shared<rex::re::DFAState>();
        if (!init_ptr && state_set.find(initial) != state_set.end()) {
            init_ptr = cur_state;
        }
        bool added = false;
        for (const auto &state : state_set) {
            if (!added) {
                if (finals.find(state) != finals.end()) {
                    final_states.insert(cur_state);
                }
                else {
                    states.insert(cur_state);
                }
                added = true;
            }
            state_map[state] = cur_state;
        }
    }
    return std::move(state_map);
}

// create edge & insert edge into edge map
inline void InsertEdge(EdgeMap &edge_map, const rex::re::SymbolPtr &symbol,
        const DFAStatePtr &next_state) {
    auto new_edge = std::make_shared<rex::re::DFAEdge>(symbol, next_state);
    edge_map.insert({symbol, new_edge});
}

// part of DFA simplify algorithm
void RebuildDFAState(const SetQueue &set_queue,
        const rex::re::SymbolSet &symbols, StateMap &state_map) {
    for (const auto &state_set : set_queue) {
        // to store edges of current state
        EdgeMap edge_map;
        DFAStatePtr cur_state = nullptr;
        // add edges for current state
        for (const auto &state : state_set) {
            // set current state
            if (!cur_state) cur_state = state_map[state];
            // the edge map is full
            if (edge_map.size() == symbols.size()) break;
            // for each symbols
            for (const auto &symbol : symbols) {
                // find next state
                DFAStatePtr next = nullptr;
                for (const auto &edge : state->out_edges()) {
                    if (edge->symbol() == symbol) {
                        next = edge->next_state();
                        break;
                    }
                }
                // insert edge into edge map
                if (!next) {
                    edge_map.insert({symbol, nullptr});
                }
                else if (next == state) {
                    InsertEdge(edge_map, symbol, state_map[next]);
                }
                else {
                    const auto &next_state = state_map[next];
                    if (state_map[state] != next_state) {
                        InsertEdge(edge_map, symbol, next_state);
                    }
                }
            }
        }
        // insert edges to current state
        for (const auto &it : edge_map) {
            if (it.second) cur_state->AddEdge(it.second);
        }
    }
}

} // namespace

namespace rex::re {

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
// TODO: optimize sub-functions
void DFAModel::Simplify() {
    // initial the queue of state sets
    SetQueue set_queue;
    set_queue.push_back(states_);
    set_queue.push_back(final_states_);
    // get the divisions of simplified DFA states
    GetDivision(set_queue, symbols_);
    // rebuild the simplified states of DFA
    DFAStatePtr initial_state = nullptr;
    DFAStateSet states, final_states;
    // get the state map for mapping old states to new states
    auto state_map = GetStateMap(set_queue, final_states_, initial_,
            initial_state, states, final_states);
    RebuildDFAState(set_queue, symbols_, state_map);
    // replace states of current model
    Release(false);
    initial_ = initial_state;
    states_ = states;
    final_states_ = final_states;
}

void DFAModel::GenerateStateTable() {
    //
}

#if NDEBUG
#else
void DFAModel::Debug() {
    using namespace std;
    // print info of symbols
    for (const auto &s : symbols_) {
        cout << "symbol " << s << ':' << endl << "  ";
        CharSet set;
        set.InsertSymbol(s);
        int i = 0;
        for (const auto &c : set) {
            cout << c << ' ';
            if (++i % 20 == 0) cout << endl << "  ";
        }
        cout << endl;
    }
    // print info of states
    std::unordered_map<DFAStatePtr, int> id_map;
    int cur_id = 0;
    auto GetStateId = [&id_map, &cur_id](const DFAStatePtr &state) {
        auto ret = id_map.insert({state, cur_id});
        if (ret.second) ++cur_id;
        return ret.first->second;
    };
    auto PrintStateSet = [this, GetStateId]
            (const DFAStateSet &set, bool fin) {
        for (const auto &s : set) {
            cout << "state " << GetStateId(s) << ' ';
            if (s == initial_) cout << "(initial) ";
            if (fin) cout << "(final) ";
            cout << ':' << endl;
            for (const auto &e : s->out_edges()) {
                cout << "  edge to state " << GetStateId(e->next_state());
                cout << " with symbol " << e->symbol() << endl;
            }
        }
    };
    PrintStateSet(states_, false);
    PrintStateSet(final_states_, true);
}
#endif

} // namespace rex::re
