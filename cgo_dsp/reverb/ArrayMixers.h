namespace cgo
{

namespace ArrayMixers
{

template <typename T, size_t N>
class Hadamard
{
    static_assert (std::is_floating_point_v<T>, "T must be a floating point type");
    static_assert ((N & (N - 1)) == 0 && N > 0, "N must be a power of 2");

public:
    static void mix (std::array<T, N>& data)
    {
        // Fast Walsh-Hadamard Transform
        for (size_t step = 1; step < N; step *= 2)
        {
            for (size_t i = 0; i < N; i += step * 2)
            {
                for (size_t j = i; j < i + step; j++)
                {
                    const auto a = data[j];
                    const auto b = data[j + step];
                    data[j] = a + b;
                    data[j + step] = a - b;
                }
            }
        }

        // Normalize
        const auto scale = static_cast<T> (1.0 / std::sqrt (static_cast<double> (N)));
        for (auto& x : data)
            x *= scale;
    }
};

template <typename T, size_t N>
class Householder
{
    static_assert (std::is_floating_point_v<T>, "T must be a floating point type");
    static_assert (N > 0, "N must be greater than 0");

public:
    static void mix (std::array<T, N>& data)
    {
        const auto sum = std::accumulate (data.begin(), data.end(), static_cast<T> (0));

        constexpr auto scale = static_cast<T> (2.0 / N);
        const auto correction = sum * scale;

        std::for_each (data.begin(), data.end(), [correction] (T& x) { x -= correction; });
    }
};

} // namespace ArrayMixers

} // namespace cgo
