#pragma once
#include <type_traits>
#include <functional>
#include <vector>
namespace TinyLinq
{
	template<typename TValue>
	struct cleanup_type
	{
		typedef typename std::remove_cv<typename std::remove_reference<TValue>::type>::type type;
	};

	template<typename TIterator>
	class Range
	{
	public:
		static TIterator get_iterator();
		typedef decltype(*get_iterator())						raw_value_type;
		typedef typename cleanup_type<raw_value_type>::type		value_type;
		typedef const value_type&								return_type;
	public:
		Range(const TIterator& _beg, const TIterator& _end)
			:beg(_beg)
			, end(_end)
			, is_first_visit(true)
		{}

		bool next() //move forward and return true if the current iter is validate
		{
			if (beg == end) return false;
			if (!is_first_visit)
			{
				++beg;
			}
			is_first_visit = false;
			return (beg != end);
		}

		return_type front()
		{
			return *beg;
		}

	protected:
		TIterator beg;
		TIterator end;
		bool is_first_visit;
	};

	template<typename TRange, typename TFunction>
	class WhereRange
	{
	public:
		typedef typename TRange::value_type value_type;
		typedef typename TRange::return_type return_type;

		WhereRange(const TRange& _range, TFunction _predicate)
			:range(_range)
			, predicate(_predicate)
		{}

		bool next()
		{
			while (range.next())
			{
				if (predicate(range.front()))
					return true;
			}
			return false;
		}

		return_type front()
		{
			return range.front();
		}


	private:
		TRange range;
		TFunction predicate;
	};

	template<typename TRange, typename TFunction>
	class SelectRange
	{
	public:
		static typename TRange::return_type	dummy_return_type();
		static TFunction					dummy_function();

		typedef typename cleanup_type<decltype(dummy_function()(dummy_return_type()))>::type	value_type;
		typedef typename value_type																return_type;

		SelectRange(const TRange& _range, TFunction _function)
			:range(_range)
			, function(_function)
		{}

		bool next()
		{
			return range.next();
		}

		return_type front()
		{
			return function(range.front());
		}
	private:
		TRange range;
		TFunction function;
	};

	//ref

	template<typename TRange>
	class RefRange
	{
	public:
		typedef typename std::reference_wrapper<typename const TRange::value_type > value_type;
		typedef value_type															return_type;
		RefRange(const TRange& _range)
			:range(_range)
		{}

		bool next()
		{
			return range.next();
		}

		return_type front()
		{
			return range.front();
		}
	private:
		TRange range;
	};

	//take
	template<typename TRange>
	class TakeRange
	{
	public:
		static typename TRange::value_type dummy_value_type();

		typedef typename TRange::value_type		value_type;
		typedef typename TRange::return_type	return_type;

		TakeRange(const TRange& _range, int _count)
			:range(_range)
			, count(_count)
		{}

		bool next()
		{
			if (count > 0)
			{
				--count;
				return range.next();
			}
			else
			{
				return false;
			}

		}

		return_type front()
		{
			return range.front();
		}
	private:
		TRange range;
		int count;
	};

	//any
	//all
	//aggregate





	template<typename TRange>
	class linq
	{
	public:
		linq(const TRange& _range)
			:range(_range)
		{}

		template<typename TFunction>
		auto where(const TFunction& predicate)->linq<WhereRange<TRange, TFunction>>
		{
			auto result = WhereRange<TRange, TFunction>(range, predicate);
			return linq<WhereRange<TRange, TFunction>>(result);
		}

		template<typename TFunction>
		auto select(const TFunction& function)->linq<SelectRange<TRange, TFunction>>
		{
			auto result = SelectRange<TRange, TFunction>(range, function);
			return linq<SelectRange<TRange, TFunction>>(result);
		}


		auto ref()->linq<RefRange<TRange>>
		{
			return linq<RefRange<TRange>>(range);
		}

		auto take(int count)->linq<TakeRange<TRange>>
		{
			auto result = TakeRange<TRange>(range, count);
			return linq<TakeRange<TRange>>(result);
		}

		auto to_vector()->std::vector<typename TRange::value_type>
		{
			std::vector<TRange::value_type> v;
			while (range.next())
			{
				v.push_back(range.front());
			}
			return v;
		}

	private:
		TRange range;
	};

	template<typename TContainer>
	auto from(const TContainer& container)->linq<Range<decltype(std::begin(container))>>
	{
		typedef decltype(std::begin(container)) TIterator;

		auto range = Range<TIterator>(std::begin(container), std::end(container));
		return linq<Range<TIterator>>(range);
	}
}