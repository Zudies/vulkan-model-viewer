#include "BitFlag.h"

namespace Graphics {

template<typename T>
BitFlag<T>::BitFlag()
  : m_flags(static_cast<T>(0)) {
}

template<typename T>
void BitFlag<T>::SetFlag(T flag, bool set) {
    if (set) {
        m_flags |= flag;
    }
    else {
        ClearFlag(flag);
    }
}

template<typename T>
void BitFlag<T>::ClearFlag(T flag) {
    m_flags &= ~flag;
}

template<typename T>
bool BitFlag<T>::GetFlag(T flag) const {
    return (m_flags & flag) == flag;
}

} // namespace Graphics
