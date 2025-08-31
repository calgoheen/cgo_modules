namespace cgo
{

class CircularIterator
{
public:
    CircularIterator() = default;
    CircularIterator (int length) : length (length) {}
    CircularIterator (const CircularIterator& other) : head (other.get()), length (other.getLength()) {}

    CircularIterator& operator= (const CircularIterator& other)
    {
        head = other.get();
        length = other.getLength();
        return *this;
    }

    int operator()() { return getAndInc(); }

    int get() const { return head; }
    int get (int offset) const { return (head + offset) % length; }
    int getAndInc()
    {
        int h = head;
        inc();
        return h;
    }

    void inc() { head = (head + 1) % length; }
    void reset() { head = 0; }

    void setLength (int l)
    {
        length = l;
        head = head % length;
    }
    int getLength() const { return length; }

private:
    int head { 0 };
    int length { 0 };
};

} // namespace cgo
