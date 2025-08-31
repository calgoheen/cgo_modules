namespace cgo
{

/*  "THE BEER-WARE LICENSE" (Revision 42): Devin Lane wrote this file. As long as you retain
 *  this notice you can do whatever you want with this stuff. If we meet some day, and you
 *  think this stuff is worth it, you can buy me a beer in return. 
 */
template <typename X>
class Spline
{
public:
    /** An empty, invalid spline */
    Spline() = default;

    /** A spline with x and y values */
    Spline (const std::vector<X>& x, const std::vector<X>& y)
    {
        if (x.size() != y.size())
        {
            jassertfalse;
            return;
        }

        if (x.size() < 3)
        {
            jassertfalse;
            return;
        }

        typedef typename std::vector<X>::difference_type size_type;

        size_type n = y.size() - 1;

        std::vector<X> b (n), d (n), a (n), c (n + 1), l (n + 1), u (n + 1), z (n + 1);
        std::vector<X> h (n + 1);

        l[0] = X (1);
        u[0] = X (0);
        z[0] = X (0);
        h[0] = x[1] - x[0];

        for (size_type i = 1; i < n; i++)
        {
            h[i] = x[i + 1] - x[i];
            l[i] = X (2 * (x[i + 1] - x[i - 1])) - X (h[i - 1]) * u[i - 1];
            u[i] = X (h[i]) / l[i];
            a[i] = (X (3) / X (h[i])) * (y[i + 1] - y[i]) - (X (3) / X (h[i - 1])) * (y[i] - y[i - 1]);
            z[i] = (a[i] - X (h[i - 1]) * z[i - 1]) / l[i];
        }

        l[n] = X (1);
        z[n] = c[n] = X (0);

        for (size_type j = n - 1; j >= 0; j--)
        {
            c[j] = z[j] - u[j] * c[j + 1];
            b[j] = (y[j + 1] - y[j]) / X (h[j]) - (X (h[j]) * (c[j + 1] + X (2) * c[j])) / X (3);
            d[j] = (c[j + 1] - c[j]) / X (3 * h[j]);
        }

        for (size_type i = 0; i < n; i++)
            elements.push_back (Element (x[i], y[i], b[i], c[i], d[i]));
    }

    Spline (const Spline<X>& other)
        : elements (other.elements) {}

    Spline (Spline<X>&& other)
        : elements (std::move (other.elements)) {}

    virtual ~Spline() = default;

    Spline<X>& operator= (const Spline<X>& other)
    {
        elements = other.elements;
        return *this;
    }

    Spline<X>& operator= (Spline<X>&& other)
    {
        elements = std::move (other.elements);
        return *this;
    }

    X operator[] (const X& x) const
    {
        return interpolate (x);
    }

    X interpolate (const X& x) const
    {
        if (elements.size() == 0)
            return X();

        typename std::vector<element_type>::const_iterator it;

        it = std::lower_bound (elements.begin(), elements.end(), element_type (x));
        if (it != elements.begin())
            it--;

        return it->eval (x);
    }

    std::vector<X> operator[] (const std::vector<X>& xx) const
    {
        return interpolate (xx);
    }

    /* Evaluate at multiple locations, assuming xx is sorted ascending */
    std::vector<X> interpolate (const std::vector<X>& xx) const
    {
        if (elements.size() == 0)
            return std::vector<X> (xx.size());

        typename std::vector<X>::const_iterator it;
        typename std::vector<element_type>::const_iterator it2;
        it2 = elements.begin();
        std::vector<X> ys;
        for (it = xx.begin(); it != xx.end(); it++)
        {
            it2 = std::lower_bound (it2, elements.end(), element_type (*it));
            if (it2 != elements.begin())
                it2--;

            ys.push_back (it2->eval (*it));
        }

        return ys;
    }

protected:
    class Element
    {
    public:
        Element (X _x) : x (_x) {}

        Element (X _x, X _a, X _b, X _c, X _d)
            : x (_x), a (_a), b (_b), c (_c), d (_d) {}

        X eval (const X& xx) const
        {
            X xix (xx - x);
            return a + b * xix + c * (xix * xix) + d * (xix * xix * xix);
        }

        bool operator< (const Element& e) const { return x < e.x; }
        bool operator< (const X& xx) const { return x < xx; }

        X x;
        X a, b, c, d;
    };

    typedef Element element_type;
    std::vector<element_type> elements;
};

} // namespace cgo
