//
// Created by richard on 2020-08-26.
//

#ifndef HAMUTILITIES_CONSTEXPERTRIG_H
#define HAMUTILITIES_CONSTEXPERTRIG_H


#include <type_traits>
#include <limits>
#include <array>
#include <cmath>
#include <iostream>

template<std::size_t... Is>
struct seq {
};

template<std::size_t N, std::size_t... Is>
struct gen_seq : gen_seq<N - 1, N, Is...> {
};

template<std::size_t... Is>
struct gen_seq<0, Is...> : seq<Is...> {
};

namespace math {
    template<typename T> constexpr T pi = 3.14159265358979323846264338327L;
    template<typename T> constexpr T two_pi = 6.28318530717958647692528676656L;
    template<typename T> constexpr T half_pi = pi<T> * 0.5;

    constexpr static double pi_v = pi<double>;
    constexpr static double two_pi_v = two_pi<double>;
    constexpr static double half_pi_v = half_pi<double>;

    template<class T, class dcy = std::decay_t<T>>
    constexpr inline std::enable_if_t<std::is_floating_point<T>::value, dcy> inverse(T value) {
        return (value == 0) ? 0.0 : 1.0 / value;
    }

    constexpr inline long double factorial(std::intmax_t const &n) {
        if (n == 0) { return 1; }
        long double result = n;
        for (std::intmax_t i = n - 1; i > 0; --i) {
            result *= i;
        }
        return result;
    }

    constexpr inline std::size_t max_factorial() {
        std::size_t i = 0;
        long double d = 0;
        while ((d = factorial(i)) < std::numeric_limits<long double>::max()) { ++i; }
        return i;
    }

    template<class base, std::size_t N>
    class trig_coeffs {
        using T = typename base::value_type;
        using array_type = std::array<T, N>;

        template<std::size_t ... NS>
        constexpr static inline array_type _coeffs(seq<NS ...>) {
            return {{base::coeff(NS) ...}};
        }

    public:
        constexpr static array_type coeffs = _coeffs(gen_seq<N>{});
    };

    template<class base, std::size_t N>
    constexpr typename trig_coeffs<base, N>::array_type trig_coeffs<base, N>::coeffs;

    template<class base, std::size_t N, class dcy = std::decay_t<typename base::value_type>>
    constexpr std::enable_if_t<std::is_floating_point<dcy>::value, dcy>
    _sincos(typename base::value_type x) noexcept {
        using c = trig_coeffs<base, N>;

        if (std::isnan(x) && std::numeric_limits<dcy>::has_quiet_NaN) {
            return static_cast<dcy>(std::numeric_limits<dcy>::quiet_NaN());
        } else if (std::isinf(x) && std::numeric_limits<dcy>::has_infinity) {
            return static_cast<dcy>(std::numeric_limits<dcy>::infinity());
        } else {
            dcy result = 0.0;//result accumulator
            //do input range mapping
            dcy _x = base::range_reduce(x);
            //taylor series
            {
                const dcy x_2 = _x * _x; //store x^2
                dcy pow = base::initial_condition(_x);
                for (auto &&cf: c::coeffs) {
                    result += cf * pow;
                    pow *= x_2;
                }
            }
            return result;
        }
    }

    namespace detail {
        template<class T>
        struct _sin {
            using value_type = T;

            constexpr static inline T coeff(std::size_t n) noexcept {
                return (n % 2 ? 1 : -1) * inverse(factorial((2 * n) - 1));
            }

            constexpr static inline T range_reduce(T x) noexcept {
                T _x = x;
                _x += math::pi<T>;
                _x -= static_cast<std::size_t>(_x / math::two_pi<T>) * math::two_pi<T>;
                _x -= math::pi<T>;
                return _x;
            }

            constexpr static inline T initial_condition(T x) noexcept {
                return x;
            }

            constexpr static inline std::size_t default_N() noexcept {
                return 16;
            }
        };
    }

    template<class T, std::size_t N = detail::_sin<T>::default_N()>
    constexpr inline std::decay_t<T> sin(T x) noexcept {
        return _sincos<detail::_sin<T>, N>(x);
    }

    template<class T, std::size_t N = detail::_sin<T>::default_N()>
    constexpr inline std::decay_t<T> cos(T x) noexcept {
        return _sincos<detail::_sin<T>, N>(math::half_pi<T> - x);
    }

}

#if 0
int main(int argc,char** argv){
    double phs =0;
    double stp = math::two_pi_v/100.0;
    for(int i = 0;i<100;++i){
        std::cout<<math::sin(phs)<<std::endl;
        phs+=stp;
    }
    return 0;
}

#endif


#endif //HAMUTILITIES_CONSTEXPERTRIG_H
