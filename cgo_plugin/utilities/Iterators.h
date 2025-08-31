namespace cgo
{

template <typename T, typename TIter = decltype (std::begin (std::declval<T>())), typename = decltype (std::end (std::declval<T>()))>
constexpr auto enumerate (T&& iterable)
{
    struct iterator
    {
        size_t i;
        TIter iter;
        bool operator!= (const iterator& other) const { return iter != other.iter; }
        void operator++()
        {
            ++i;
            ++iter;
        }
        auto operator*() const { return std::tie (i, *iter); }
    };
    struct iterable_wrapper
    {
        T iterable;
        auto begin() { return iterator { 0, std::begin (iterable) }; }
        auto end() { return iterator { 0, std::end (iterable) }; }
    };
    return iterable_wrapper { std::forward<T> (iterable) };
}

template <typename T, typename S, typename TIter = decltype (std::begin (std::declval<T>())), typename = decltype (std::end (std::declval<T>())), typename SIter = decltype (std::begin (std::declval<S>())), typename = decltype (std::end (std::declval<S>()))>
constexpr auto zip (T&& iterableT, S&& iterableS)
{
    struct iterator
    {
        TIter titer;
        SIter siter;

        const TIter tend;
        const SIter send;
        bool operator!= (const iterator& other) const
        {
            return titer != other.titer && siter != other.siter;
        }
        void operator++()
        {
            ++siter;
            ++titer;

            if (titer == tend)
                siter = send;
            else if (siter == send)
                titer = tend;
        }
        auto operator*() const { return std::tie (*titer, *siter); }
    };
    struct iterable_wrapper
    {
        T titerable;
        S siterable;
        auto begin()
        {
            return iterator { std::begin (titerable), std::begin (siterable), std::end (titerable), std::end (siterable) };
        }
        auto end()
        {
            return iterator { std::end (titerable), std::end (siterable), std::end (titerable), std::end (siterable) };
        }
    };
    return iterable_wrapper { std::forward<T> (iterableT), std::forward<S> (iterableS) };
}

} // namespace cgo
