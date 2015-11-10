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
			,function(_function)
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


	template<typename TRange, typename TFunction>
	class SelectManyRange
	{
	public:
		static typename TRange::return_type	dummy_return_type();
		static TFunction					dummy_function();

		typedef	typename cleanup_type<decltype(dummy_function()(dummy_return_type()))>::type				inner_data_type;
		static inner_data_type				dummy_inner_data_type();

		typedef decltype(std::begin(dummy_inner_data_type())) inner_data_iterator_type;

		static inner_data_iterator_type dummy_inner_data_iterator_type();
		typedef typename decltype(Range<inner_data_iterator_type>(dummy_inner_data_iterator_type(),dummy_inner_data_iterator_type()))		inner_range_type;

		typedef typename inner_range_type::value_type														value_type;
		typedef typename inner_range_type::return_type														return_type;

		SelectManyRange(const TRange& _range, TFunction _function)
			:range(_range)
			,function(_function)
			,inner_range(NULL)
		{}

		bool next()
		{
			if (inner_range && inner_range->next())
			{
				return true;
			}

			if (range.next())
			{
				inner_data = function(range.front());
				inner_range = new inner_range_type(std::begin(inner_data),std::end(inner_data));
				return inner_range->next();
			}

			if (inner_range)
			{
				delete inner_range;
			}
			//need to free inner_range here

			return false;
		}

		return_type front()
		{
			return inner_range->front();
		}
	private:
		TRange				range;
		TFunction			function;
		inner_range_type*	inner_range;
		inner_data_type		inner_data;
	};

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

	

	template<typename TRange>
	class Linq
	{
	public:
		Linq(const TRange& _range)
			:range(_range)
		{}


		typedef typename TRange::value_type		value_type;
		typedef typename TRange::return_type	return_type;

		template<typename TFunction>
		auto where(const TFunction& predicate)->Linq<WhereRange<TRange, TFunction>>
		{
			auto result = WhereRange<TRange, TFunction>(range, predicate);
			return Linq<WhereRange<TRange, TFunction>>(result);
		}

		template<typename TFunction>
		auto select(const TFunction& function)->Linq<SelectRange<TRange, TFunction>>
		{
			auto result = SelectRange<TRange, TFunction>(range, function);
			return Linq<SelectRange<TRange, TFunction>>(result);
		}

		template<typename TFunction>
		auto select_many(const TFunction& function)->Linq<SelectManyRange<TRange, TFunction>>
		{
			auto result = SelectManyRange<TRange, TFunction>(range, function);
			return Linq<SelectManyRange<TRange, TFunction>>(result);
		}

		auto ref()->Linq<RefRange<TRange>>
		{
			return Linq<RefRange<TRange>>(range);
		}

		auto take(int count)->Linq<TakeRange<TRange>>
		{
			auto result = TakeRange<TRange>(range, count);
			return Linq<TakeRange<TRange>>(result);
		}

		template<typename TFunction>
		auto aggregate(typename TRange::value_type init_value,const TFunction& function)
			->typename TRange::value_type
		{
			while (range.next())
			{
				init_value = function(init_value,range.front());
			}
			return init_value;
		}

		//any
		template<typename TFunction>
		bool any(const TFunction& function)
		{
			while (range.next())
			{
				if(function(range.front()))
					return true;
			}
			return false;
		}

		template<typename TFunction>
		bool all(const TFunction& function)
		{
			while (range.next())
			{
				if(!function(range.front()))
					return false;
			}
			return true;
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

		TRange range;
	};

	template<typename TContainer>
	auto from(const TContainer& container)->Linq<Range<decltype(std::begin(container))>>
	{
		typedef decltype(std::begin(container)) TIterator;

		auto range = Range<TIterator>(std::begin(container), std::end(container));
		return Linq<Range<TIterator>>(range);
	}
}