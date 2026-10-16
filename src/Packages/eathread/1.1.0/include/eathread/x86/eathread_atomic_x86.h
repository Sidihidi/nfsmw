#ifndef EATHREAD_X86_EATHREAD_ATOMIC_X86_H
#define EATHREAD_X86_EATHREAD_ATOMIC_X86_H

// PC (x86, MSVC): los enteros atomicos son las intrinsecas Interlocked del compilador. speed.exe las trae en linea
// (`lock xadd` para leer y sumar, `xchg` para escribir) y tambien fuera de linea en el objeto del semaforo
// (0x834D82 GetValue, 0x834D89 SetValue, 0x834D95 Decrement, 0x834D9E Add).

extern "C" long __cdecl _InterlockedExchange(long volatile *Target, long Value);
extern "C" long __cdecl _InterlockedExchangeAdd(long volatile *Addend, long Value);
extern "C" long __cdecl _InterlockedCompareExchange(long volatile *Destination, long Exchange, long Comparand);
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)

template <typename T> class AtomicInt {
  public:
    typedef T ValueType;

    AtomicInt() {}

    AtomicInt(ValueType n) {
        this->mValue = n;
    }

    ValueType GetValue() const {
        return _InterlockedExchangeAdd(reinterpret_cast<long volatile *>(const_cast<ValueType *>(&mValue)), 0);
    }

    ValueType GetValueRaw() const {
        return mValue;
    }

    ValueType SetValue(ValueType n) {
        return _InterlockedExchange(reinterpret_cast<long volatile *>(&mValue), n);
    }

    bool SetValueConditional(ValueType n, ValueType condition) {
        return _InterlockedCompareExchange(reinterpret_cast<long volatile *>(&mValue), n, condition) == condition;
    }

    ValueType Increment() {
        return _InterlockedExchangeAdd(reinterpret_cast<long volatile *>(&mValue), 1) + 1;
    }

    ValueType Decrement() {
        return _InterlockedExchangeAdd(reinterpret_cast<long volatile *>(&mValue), -1) - 1;
    }

    ValueType Add(ValueType n) {
        return _InterlockedExchangeAdd(reinterpret_cast<long volatile *>(&mValue), n) + n;
    }

    operator const ValueType() const {
        return GetValue();
    }

    AtomicInt<ValueType> &operator++() {
        Increment();
        return *this;
    }

    AtomicInt<ValueType> &operator--() {
        Decrement();
        return *this;
    }

    AtomicInt<ValueType> &operator=(ValueType n) {
        SetValue(n);
        return *this;
    }

  protected:
    volatile ValueType mValue; // offset 0x0, size 0x4
};

typedef AtomicInt<int> AtomicInt32;
typedef AtomicInt<unsigned int> AtomicUint32;
typedef AtomicInt32 AtomicIntPtr;

#endif
