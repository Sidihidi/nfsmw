#include <algorithm>

#include "Speed/Indep/Libs/Support/Utility/UCOM.h"

UTL::COM::Object::_IList::_IList(unsigned int icount) {
    reserve(icount);
}

#ifndef EA_BUILD_A124
UTL::COM::Object::_IList::~_IList() {}
#endif

void UTL::COM::Object::_IList::Add(HINTERFACE handle, IUnknown *ref) {
    _IPair pair(handle, ref);

    insert(std::upper_bound(begin(), end(), pair), pair);
}

UTL::COM::IUnknown *UTL::COM::Object::_IList::Find(HINTERFACE handle) {
    iterator iter = std::lower_bound(begin(), end(), _IPair(handle, NULL));

    if (iter != end() && iter->handle == handle) {
        return iter->ref;
    }

    return NULL;
}

void UTL::COM::Object::_IList::Remove(IUnknown *pUnk) {
#ifdef EA_PLATFORM_PLAYSTATION2
    erase(std::remove_if(begin(), end(), _IPair::_Finder(pUnk)));
#else
    _IPair::_Finder finder(pUnk);
    iterator last = end();
    iterator first = std::find_if(begin(), last, finder);
    iterator result;

    if (first == last) {
        result = last;
    } else {
        iterator next = first;
        result = std::remove_copy_if(++next, last, first, finder);
    }
    erase(result);
#endif
}
