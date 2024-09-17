#pragma once

namespace Graphics {

template<typename T>
class BitFlag {
public:
    typedef T type;

    BitFlag();

    void SetFlag(T flag, bool set = true);
    void ClearFlag(T flag);
    bool GetFlag(T flag) const;

private:
    T m_flags;
};

} // namespace Graphics

#include "BitFlag.tpp"
