#ifndef EATHREAD_XENON_EATHREAD_ATOMIC_XENON_H
#define EATHREAD_XENON_EATHREAD_ATOMIC_XENON_H

// Xbox 360: los enteros atomicos son las intrinsecas Interlocked del compilador (lwarx/stwcx. entre mfmsr y mtmsrd,
// que es como cl de Xenon las expande). El EGM las trae en linea en el codigo de los hilos y fuera de linea en
// 0x82655220 (SetValue), 0x82655248 (SetValueConditional), 0x826552A0 (Increment), 0x826552C8 (Decrement) y
// 0x82656050 (el constructor con valor), copias de LTCG con los argumentos constantes de sus llamantes.

extern "C" {
long _InterlockedIncrement(long volatile *lpAddend);
long _InterlockedDecrement(long volatile *lpAddend);
long _InterlockedExchange(long volatile *Target, long Value);
long _InterlockedExchangeAdd(long volatile *Addend, long Value);
long _InterlockedCompareExchange(long volatile *Destination, long Exchange, long Comperand);
}
#pragma intrinsic(_InterlockedIncrement, _InterlockedDecrement, _InterlockedExchange, _InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)

template <typename T> class AtomicInt {
  public:
    typedef T ValueType;

    AtomicInt() {}

    AtomicInt(ValueType n) {
        SetValue(n);
    }

    ValueType GetValue() const {
        return mValue;
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
        return _InterlockedIncrement(reinterpret_cast<long volatile *>(&mValue));
    }

    ValueType Decrement() {
        return _InterlockedDecrement(reinterpret_cast<long volatile *>(&mValue));
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
