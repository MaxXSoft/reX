#include "reobj.h"

#include <cassert>

namespace rex {

REObject RE::Nil() {
    return REObject(new RENilObj());
}

REObject RE::Word(const std::string &word) {
    REObject reo;
    for (const auto &i : word) {
        auto cur_char = REObject(new RECharObj(i));
        reo = reo ? reo & std::move(cur_char) : std::move(cur_char);
    }
    return std::move(reo);
}

REObject RE::Range(char c1, char c2) {   // TODO: optimize
    assert(c2 > c1);
    char c = c1;
    REObject reo;
    while (c <= c2) {
        auto cur_char = REObject(new RECharObj(c++));
        reo = reo ? reo | std::move(cur_char) : std::move(cur_char);
    }
    return std::move(reo);
}

REObject RE::And(REObject lhs, REObject rhs) {
    return REObject(new REAndObj(std::move(lhs), std::move(rhs)));
}

REObject RE::Or(REObject lhs, REObject rhs) {
    return REObject(new REOrObj(std::move(lhs), std::move(rhs)));
}

REObject RE::Many(REObject reo) {
    return REObject(new REKleeneObj(std::move(reo)));
}

REObject RE::Many1(REObject reo) {
    auto kleene = REObject(new REKleeneObj(std::move(reo)));
    return REObject(new REAndObj(reo, kleene));
}

REObject RE::Optional(REObject reo) {
    auto nil = RE::Nil();
    return REObject(new REOrObj(std::move(reo), std::move(nil)));
}

NFAModelPtr RENilObj::GenerateNFA() {
    auto node = std::make_shared<NFANode>();
    auto edge = std::make_shared<NFAEdge>(NFAEdge::kEmpty, node);
    auto model = std::make_shared<NFAModel>();
    model->AddNode(node);
    model->set_entry(edge);
    model->set_tail(node);
    return model;
}

NFAModelPtr RECharObj::GenerateNFA() {
    auto node = std::make_shared<NFANode>();
    auto edge = std::make_shared<NFAEdge>(c_, node);
    auto model = std::make_shared<NFAModel>();
    model->AddNode(node);
    model->set_entry(edge);
    model->set_tail(node);
    return model;
}

NFAModelPtr REAndObj::GenerateNFA() {
    // get lhs & rhs
    auto lhs = lhs_->GenerateNFA();
    auto rhs = rhs_->GenerateNFA();
    auto model = std::make_shared<NFAModel>();
    // set entry & tail
    model->set_entry(lhs->entry());
    model->set_tail(rhs->tail());
    // connect lhs & rhs
    lhs->tail()->AddEdge(rhs->entry());
    // add nodes of lhs & rhs
    model->AddNodes(lhs->nodes());
    model->AddNodes(rhs->nodes());
    return model;
}

NFAModelPtr REOrObj::GenerateNFA() {
    // create entry edge & state nodes
    auto node = std::make_shared<NFANode>();
    auto entry = std::make_shared<NFAEdge>(NFAEdge::kEmpty, node);
    // create tail node & some necessary edges
    auto tail = std::make_shared<NFANode>();
    auto back0 = std::make_shared<NFAEdge>(NFAEdge::kEmpty, tail);
    auto back1 = std::make_shared<NFAEdge>(NFAEdge::kEmpty, tail);
    // get lhs & rhs
    auto lhs = lhs_->GenerateNFA();
    auto rhs = rhs_->GenerateNFA();
    // generate the 'or' logic
    node->AddEdge(lhs->entry());
    node->AddEdge(rhs->entry());
    lhs->tail()->AddEdge(back0);
    rhs->tail()->AddEdge(back1);
    // create the final model
    auto model = std::make_shared<NFAModel>();
    model->set_entry(entry);
    model->set_tail(tail);
    model->AddNode(node);
    model->AddNodes(lhs->nodes());
    model->AddNodes(rhs->nodes());
    model->AddNode(tail);
    return model;
}

NFAModelPtr REKleeneObj::GenerateNFA() {
    // create tail node & empty edges
    auto tail = std::make_shared<NFANode>();
    auto entry = std::make_shared<NFAEdge>(NFAEdge::kEmpty, tail);
    auto back = std::make_shared<NFAEdge>(NFAEdge::kEmpty, tail);
    // get source model
    auto src = reo_->GenerateNFA();
    // generate kleene closure logic
    tail->AddEdge(src->entry());
    src->tail()->AddEdge(back);
    // generate final model
    auto model = std::make_shared<NFAModel>();
    model->set_entry(entry);
    model->set_tail(tail);
    model->AddNode(tail);
    model->AddNodes(src->nodes());
    return model;
}

} // namespace rex
