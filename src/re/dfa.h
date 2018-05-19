#ifndef REX_RE_DFA_H_
#define REX_RE_DFA_H_

#include <memory>
#include <utility>

namespace rex {

class DFAInterface {
public:
    virtual ~DFAInterface() = default;

    virtual void GenerateTable() = 0;
};

using DFAPtr = std::shared_ptr<DFAInterface>;

} // namespace rex

#endif // REX_RE_DFA_H_
