#include "reobj.h"

#include <cassert>

namespace rex {

REObject RE::Nil() {
    return REObject(new RENilObj());
}

REObject RE::Word(const std::string &word) {
    REObject reo;
    for (const auto &i : word) {
        auto symbol = std::make_shared<CharSymbol>(i);
        auto cur_char = REObject(new RESymbolObj(symbol));
        reo = reo ? reo & std::move(cur_char) : std::move(cur_char);
    }
    return std::move(reo);
}

REObject RE::Range(char c1, char c2) {
    assert(c1 <= c2);
    auto symbol = std::make_shared<RangeSymbol>(c1, c2);
    return REObject(new RESymbolObj(symbol));
}

REObject RE::Lambda(LambdaSymbol::SymbolDef func) {
    auto symbol = std::make_shared<LambdaSymbol>(func);
    return REObject(new RESymbolObj(symbol));
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
    auto kleene = REObject(new REKleeneObj(reo));
    return REObject(new REAndObj(reo, kleene));
}

REObject RE::Optional(REObject reo) {
    auto nil = RE::Nil();
    return REObject(new REOrObj(std::move(reo), std::move(nil)));
}

NFAModelPtr RENilObj::GenerateNFA() {
    auto node = std::make_shared<NFANode>();
    auto edge = std::make_shared<NFAEdge>(nullptr, node);
    auto model = std::make_shared<NFAModel>();
    model->AddNode(node);
    model->set_entry(edge);
    model->set_tail(node);
    return model;
}

NFAModelPtr RESymbolObj::GenerateNFA() {
    auto node = std::make_shared<NFANode>();
    auto edge = std::make_shared<NFAEdge>(symbol_, node);
    auto model = std::make_shared<NFAModel>();
    model->AddNode(node);
    model->set_entry(edge);
    model->set_tail(node);
    model->AddSymbol(symbol_);
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
    // merge char set
    model->AddCharSet(lhs->char_set());
    model->AddCharSet(rhs->char_set());
    return model;
}

NFAModelPtr REOrObj::GenerateNFA() {
    // create entry edge & state nodes
    auto node = std::make_shared<NFANode>();
    auto entry = std::make_shared<NFAEdge>(nullptr, node);
    // create tail node & some necessary edges
    auto tail = std::make_shared<NFANode>();
    auto back0 = std::make_shared<NFAEdge>(nullptr, tail);
    auto back1 = std::make_shared<NFAEdge>(nullptr, tail);
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
    // merge char set
    model->AddCharSet(lhs->char_set());
    model->AddCharSet(rhs->char_set());
    return model;
}

NFAModelPtr REKleeneObj::GenerateNFA() {
    // create tail node & empty edges
    auto tail = std::make_shared<NFANode>();
    auto entry = std::make_shared<NFAEdge>(nullptr, tail);
    auto back = std::make_shared<NFAEdge>(nullptr, tail);
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
    // add char set
    model->AddCharSet(src->char_set());
    return model;
}

} // namespace rex
