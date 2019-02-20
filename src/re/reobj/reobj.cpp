#include <re/reobj/reobj.h>

#include <cassert>

namespace rex::re {

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

REObject RE::Lambda(CharSet::SymbolDef func) {
    CharSet charset;
    charset.InsertLambda(func);
    return REObject(new RESymbolObj(charset.MakeSymbol()));
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
    return REObject(new REAndObj(reo, std::move(kleene)));
}

REObject RE::Optional(REObject reo) {
    auto nil = RE::Nil();
    return REObject(new REOrObj(std::move(reo), std::move(nil)));
}

NFAModelPtr RENilObj::GenerateNFA() {
    auto node = std::make_shared<NFANode>();
    auto edge = std::make_shared<NFAEdge>(nullptr, node);
    auto model = std::make_shared<NFAModel>();
    model->set_entry(edge);
    model->set_tail(node);
    return model;
}

NFAModelPtr RESymbolObj::GenerateNFA() {
    auto node = std::make_shared<NFANode>();
    auto edge = std::make_shared<NFAEdge>(symbol_, node);
    auto model = std::make_shared<NFAModel>();
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
    // merge char set
    model->AddSymbolSet(lhs->symbol_set());
    model->AddSymbolSet(rhs->symbol_set());
    return model;
}

void REOrObj::PreprocOrLogic(NFAModelPtr model,
        SymbolPtr common, SymbolPtr symbol) {
    if (symbol) {
        auto entry = model->entry();
        entry->set_symbol(symbol);
        // add edge for the common part of symbol
        auto edge = std::make_shared<NFAEdge>(common, entry->tail());
        // add new node for merging two edges
        auto node = std::make_shared<NFANode>();
        node->AddEdge(entry);
        node->AddEdge(edge);
        // add & set new entry
        auto new_entry = std::make_shared<NFAEdge>(nullptr, node);
        model->set_entry(new_entry);
        // add symbols
        model->AddSymbol(common);
        model->AddSymbol(symbol);
    }
    else {
        model->entry()->set_symbol(common);
        model->AddSymbol(common);
    }
}

NFAModelPtr REOrObj::GenerateNFA() {
    // get lhs & rhs
    auto lhs = lhs_->GenerateNFA();
    auto rhs = rhs_->GenerateNFA();
    // create charset for two models
    CharSet lhs_set, rhs_set;
    lhs_set.InsertSymbol(lhs->entry()->symbol());
    rhs_set.InsertSymbol(rhs->entry()->symbol());
    // judge if charsets have intersections
    if (lhs_set != rhs_set && lhs_set.HasIntersection(rhs_set)) {
        // extract the common part of two sets
        auto common = lhs_set;
        common.Intersect(rhs_set);
        lhs_set.SymDiffer(common);
        rhs_set.SymDiffer(common);
        // make new symbols
        auto common_symbol = common.MakeSymbol();
        auto lhs_symbol = lhs_set.MakeSymbol();
        auto rhs_symbol = rhs_set.MakeSymbol();
        // preprocess or logic for lhs & rhs
        PreprocOrLogic(lhs, common_symbol, lhs_symbol);
        PreprocOrLogic(rhs, common_symbol, rhs_symbol);
    }
    // create entry edge & state nodes
    auto node = std::make_shared<NFANode>();
    auto entry = std::make_shared<NFAEdge>(nullptr, node);
    // create tail node & some necessary edges
    auto tail = std::make_shared<NFANode>();
    auto back0 = std::make_shared<NFAEdge>(nullptr, tail);
    auto back1 = std::make_shared<NFAEdge>(nullptr, tail);
    // generate the 'or' logic
    node->AddEdge(lhs->entry());
    node->AddEdge(rhs->entry());
    lhs->tail()->AddEdge(back0);
    rhs->tail()->AddEdge(back1);
    // create the final model
    auto model = std::make_shared<NFAModel>();
    model->set_entry(entry);
    model->set_tail(tail);
    // merge char set
    model->AddSymbolSet(lhs->symbol_set());
    model->AddSymbolSet(rhs->symbol_set());
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
    // add char set
    model->AddSymbolSet(src->symbol_set());
    return model;
}

} // namespace rex::re
