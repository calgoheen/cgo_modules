namespace cgo
{

/** @class OptionalPointer
 *  Starts off as a unique_ptr, but can also release ownership and retain a dangling pointer.
 *  Make sure not to use after the object is gone.
*/
template <typename T>
class OptionalPointer
{
public:
    OptionalPointer() = default;
    OptionalPointer (std::unique_ptr<T> ptr) { reset (ptr.release()); }
    OptionalPointer (OptionalPointer&& other) { *this = other.release(); }
    OptionalPointer& operator= (std::unique_ptr<T> ptr)
    {
        reset (ptr.release());
        return *this;
    }
    OptionalPointer& operator= (OptionalPointer&& other)
    {
        *this = other.release();
        return *this;
    }

    std::unique_ptr<T> release() { return std::move (owning); }
    void clear() { reset (nullptr); }

    T* get() { return nonOwning; }
    const T* get() const { return nonOwning; }
    T* operator->() { return nonOwning; }
    const T* operator->() const { return nonOwning; }
    T& operator*() { return *nonOwning; }
    const T& operator*() const { return *nonOwning; }

private:
    void reset (T* ptr)
    {
        owning.reset (ptr);
        nonOwning = owning.get();
    }

    std::unique_ptr<T> owning;
    T* nonOwning { nullptr };

    JUCE_DECLARE_NON_COPYABLE (OptionalPointer)
};

} // namespace cgo
