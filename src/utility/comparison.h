/***************************************************************************
 *      Mechanized Assault and Exploration Reloaded Projectfile            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef utility_comparisonH
#define utility_comparisonH

#include <type_traits>

namespace detail {

template <typename T1, typename T2>
struct select_most_precise
{
	static const bool second_larger = sizeof (T2) > sizeof (T1);
	static const bool one_not_fundamental = !(std::is_fundamental<T1>::value && std::is_fundamental<T2>::value);

	static const bool both_same_kind = std::is_floating_point<T1>::value == std::is_floating_point<T2>::value;

	typedef typename std::conditional
	<
		one_not_fundamental,
		typename std::conditional<std::is_fundamental<T1>::value, T2, T1>::type, // both not fundamental?! -> specialize
		typename std::conditional
		<
			both_same_kind,
			typename std::conditional<second_larger, T2, T1>::type,
			typename std::conditional<std::is_floating_point<T1>::value, T1, T2>::type
		>::type
	>::type type;
};

struct compare_strategy_direct
{
	template<typename T1, typename T2>
	static inline bool equal (const T1& t1, const T2& t2)
	{
		return t1 == t2;
	}

	template<typename T1, typename T2>
	static inline bool less (const T1& t1, const T2& t2)
	{
		return t1 < t2;
	}

	template<typename T1, typename T2>
	static inline bool less_equal (const T1& t1, const T2& t2)
	{
		return t1 <= t2;
	}

	template<typename T1, typename T2>
	static inline bool greater (const T1& t1, const T2& t2)
	{
		return t1 > t2;
	}

	template<typename T1, typename T2>
	static inline bool greater_equal (const T1& t1, const T2& t2)
	{
		return t1 >= t2;
	}
};

struct compare_strategy_tolerance
{
	template<typename T1, typename T2>
	static inline bool equal (const T1& t1, const T2& t2)
	{
		return (!less (t1, t2) && !greater (t1, t2));
	}

	template<typename T1, typename T2>
	static inline bool less (const T1& t1, const T2& t2)
	{
		typedef typename select_most_precise<T1, T2>::type most_precise_type;

		auto weight = (std::abs (static_cast<most_precise_type>(t1)) + std::abs (static_cast<most_precise_type>(t2))) * 0.5;
		weight = weight > 1 ? weight : 1;

		return ((1e3*std::numeric_limits<most_precise_type>::epsilon ()) * weight < (static_cast<most_precise_type>(t2)-static_cast<most_precise_type>(t1)));
	}

	template<typename T1, typename T2>
	static inline bool less_equal (const T1& t1, const T2& t2)
	{
		return !greater (t1, t2);
	}

	template<typename T1, typename T2>
	static inline bool greater (const T1& t1, const T2& t2)
	{
		return less (t2, t1);
	}

	template<typename T1, typename T2>
	static inline bool greater_equal (const T1& t1, const T2& t2)
	{
		return !less (t1, t2);
	}
};

template<typename E>
struct compare_strategy_epsilon
{
	compare_strategy_epsilon (const E& epsilon_) :
		epsilon (epsilon_)
	{}

	template<typename T1, typename T2>
	inline bool equal (const T1& t1, const T2& t2) const
	{
		return (!less (t1, t2) && !greater (t1, t2));
	}

	template<typename T1, typename T2>
	inline bool less (const T1& t1, const T2& t2) const
	{
		typedef typename select_most_precise<T1, T2>::type most_precise_type;

		return (static_cast<most_precise_type>(epsilon) < (static_cast<most_precise_type>(t2)-static_cast<most_precise_type>(t1)));
	}

	template<typename T1, typename T2>
	inline bool less_equal (const T1& t1, const T2& t2) const
	{
		return !greater (t1, t2);
	}

	template<typename T1, typename T2>
	inline bool greater (const T1& t1, const T2& t2) const
	{
		return less (t2, t1);
	}

	template<typename T1, typename T2>
	inline bool greater_equal (const T1& t1, const T2& t2) const
	{
		return !less (t1, t2);
	}

private:
	E epsilon;
};

template<typename T1, typename T2>
struct compare_strategy_default_selector
{
private:
    template<typename ET>
    struct ErrorType
    {
        static_assert(sizeof(ET)==0, "compare_strategy_default works only on arithmetic types!");
    };
public:
    typedef typename std::conditional
    <
        std::is_arithmetic<T1>::value && std::is_arithmetic<T2>::value,
        typename std::conditional
        <
            std::is_floating_point<T1>::value || std::is_floating_point<T2>::value,
            compare_strategy_tolerance,
            compare_strategy_direct
        >::type,
        ErrorType<T1>
    >::type type;
};

template<typename T1, typename T2>
struct compare_strategy_default : public compare_strategy_default_selector<T1,T2>::type {};

} // detail namespace


template<typename Lhs, typename Rhs, typename CompareStrategy>
inline bool equals (const Lhs& lhs, const Rhs& rhs, CompareStrategy strategy)
{
	return strategy.equal (lhs, rhs);
}

template<typename Lhs, typename Rhs>
inline bool equals (const Lhs& lhs, const Rhs& rhs)
{
	return equals (lhs, rhs, detail::compare_strategy_default<Lhs, Rhs> ());
}


template<typename Lhs, typename Rhs, typename CompareStrategy>
inline bool less_than (const Lhs& lhs, const Rhs& rhs, CompareStrategy strategy)
{
	return strategy.less (lhs, rhs);
}

template<typename Lhs, typename Rhs>
inline bool less_than (const Lhs& lhs, const Rhs& rhs)
{
	return less_than (lhs, rhs, detail::compare_strategy_default<Lhs, Rhs> ());
}


template<typename Lhs, typename Rhs, typename CompareStrategy>
inline bool less_equal (const Lhs& lhs, const Rhs& rhs, CompareStrategy strategy)
{
	return strategy.less_equal (lhs, rhs);
}

template<typename Lhs, typename Rhs>
inline bool less_equal (const Lhs& lhs, const Rhs& rhs)
{
	return less_equal (lhs, rhs, detail::compare_strategy_default<Lhs, Rhs> ());
}


template<typename Lhs, typename Rhs, typename CompareStrategy>
inline bool greater_than (const Lhs& lhs, const Rhs& rhs, CompareStrategy strategy)
{
	return strategy.greater (lhs, rhs);
}

template<typename Lhs, typename Rhs>
inline bool greater_than (const Lhs& lhs, const Rhs& rhs)
{
	return greater_than (lhs, rhs, detail::compare_strategy_default<Lhs, Rhs> ());
}


template<typename Lhs, typename Rhs, typename CompareStrategy>
inline bool greater_equal (const Lhs& lhs, const Rhs& rhs, CompareStrategy strategy)
{
	return strategy.greater_equal (lhs, rhs);
}

template<typename Lhs, typename Rhs>
inline bool greater_equal (const Lhs& lhs, const Rhs& rhs)
{
	return greater_equal (lhs, rhs, detail::compare_strategy_default<Lhs, Rhs> ());
}

#endif // utility_comparisonH
