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

	template<typename TContainer>
	class extract_iterator_type
	{
	public:
		static TContainer dummy_container();
		typedef decltype(std::begin(dummy_container())) type;
	};

	template<typename TRange>
	class extract_range_trait
	{
	public:
		typedef typename TRange::value_type value_type;
		typedef typename TRange::return_type return_type;
	};

	template<typename TFunction, typename TArg>
	struct extract_return_type
	{
		static TFunction	dummy_function();
		static TArg			dummy_arg();

		typedef decltype(dummy_function()(dummy_arg())) type;
	};
	
	template<typename TIterator>
	class basic_range
	{
	public:
		static TIterator get_iterator();
		typedef decltype(*get_iterator())						raw_value_type;
		typedef typename cleanup_type<raw_value_type>::type		value_type;
		typedef const value_type&								return_type;
	public:
		basic_range()
			:beg(NULL)
			,end(NULL)
			,is_first_visit(true)
		{
		}
		basic_range(const TIterator& _beg, const TIterator& _end)
			:beg(_beg)
			,end(_end)
			,is_first_visit(true)
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
		TIterator	beg;
		TIterator	end;
		bool		is_first_visit;
	};

	template<class unknow>
	class DebugClass;

	template<typename TContainer>
	class storage_range
	{
	public:
		typedef typename extract_iterator_type<TContainer>::type iterator_type;
		typedef typename basic_range<iterator_type>::value_type	value_type;
		typedef typename basic_range<iterator_type>::return_type	return_type;

	public:
		storage_range(const TContainer& _container)
			:container(std::make_shared<TContainer>(_container))
			,range(basic_range<iterator_type>(container->begin(), container->end()))
		{
		}

		storage_range(TContainer&& _container)
			:container(std::make_shared<TContainer>(std::move(_container)))
			,range(basic_range<iterator_type>(container->begin(), container->end()))
		{
		}

		bool next()
		{
			return range.next();
		}

		return_type front()
		{
			return range.front();
		}

	private:
		std::shared_ptr<TContainer>	container;
		basic_range<iterator_type>	range;
	};

	template<typename TRange, typename TFunction>
	class where_range {
	public:
		typedef typename TRange::value_type		value_type;
		typedef typename TRange::return_type	return_type;

		where_range(const TRange& _range, TFunction _predicate)
			:range(_range)
			,predicate(_predicate) {
		}

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
		TRange		range;
		TFunction	predicate;
	};

	template<typename TRange, typename TFunction>
	class select_range
	{
	public:
		typedef typename extract_return_type<TFunction,typename TRange::return_type>::type						raw_value_type;
		typedef typename cleanup_type<raw_value_type>::type												value_type;
		typedef typename value_type																return_type;

		select_range(const TRange& _range, TFunction _function)
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
		TRange		range;
		TFunction	function;
	};

	template<typename TRange, typename TFunction>
	struct select_many_range_helper
	{
		typedef typename extract_return_type<TFunction, typename TRange::return_type>::type						inner_data_type;
		typedef typename cleanup_type<inner_data_type>::type													clean_inner_data_type;
		typedef typename extract_iterator_type<inner_data_type>::type											inner_data_iterator_type;
	
		typedef typename std::conditional <
			std::is_lvalue_reference<inner_data_type>::value,
			basic_range<inner_data_iterator_type>,
			storage_range < clean_inner_data_type >> ::type inner_range_type;
	};

	template<bool value>
	struct reverse_bool
	{
	};

	template<>
	struct reverse_bool<true>
	{
		static const bool value = false;
	};

	template<>
	struct reverse_bool<false>
	{
		static const bool value = true;
	};

	template<typename TRange, typename TFunction>
	class select_many_range
	{
	public:

		typedef	typename select_many_range_helper<TRange, TFunction>::inner_data_type										inner_data_type;
		typedef	typename select_many_range_helper<TRange, TFunction>::inner_data_iterator_type							inner_data_iterator_type;
		typedef	typename select_many_range_helper<TRange, TFunction>::inner_range_type							inner_range_type;

		typedef typename extract_range_trait<inner_range_type>::value_type value_type;
		typedef typename extract_range_trait<inner_range_type>::return_type return_type;
		select_many_range(const TRange& _range, TFunction _function)
			:range(_range)
			,function(_function)
		{
		}

		bool next()
		{
			if (inner_range && inner_range->next())
			{
				return true;
			}

			if (range.next())
			{
				inner_range = to_inner_range<std::is_lvalue_reference<inner_data_type>::value>(function(range.front()));
				return inner_range->next();
			}

			inner_range.reset();
			return false;
		}

		template<bool value>
		typename std::enable_if<value, std::shared_ptr<inner_range_type>>::type to_inner_range(inner_data_type&& ref)
		{
			return make_shared<inner_range_type>(inner_range_type(std::begin(ref), std::end(ref)));
		}

		template<bool value>
		typename std::enable_if<reverse_bool<value>::value, std::shared_ptr<inner_range_type>>::type to_inner_range(inner_data_type&& ref)
		{
			return make_shared<inner_range_type>(std::move(ref));
		}

		return_type front()
		{
			return inner_range->front();
		}
	private:
		TRange								range;
		TFunction							function;
		std::shared_ptr<inner_range_type>	inner_range;
	};

	template<typename TRange>
	class ref_range
	{
	public:
		typedef typename std::reference_wrapper<typename const TRange::value_type > value_type;
		typedef value_type															return_type;
		ref_range(const TRange& _range)
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
	class take_range
	{
	public:
		typedef typename TRange::value_type		value_type;
		typedef typename TRange::return_type	return_type;

		take_range(const TRange& _range, int _count)
			:range(_range)
			,count(_count)
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
		TRange	range;
		int		count;
	};

	template<typename TRange,typename TOtherRange>
	class concat_range
	{
	public:
		typedef typename TRange::value_type			value_type;
		typedef typename TRange::return_type	return_type;

		concat_range(const TRange& _range, const TOtherRange& _other_range)
			:range(_range)
			,other_range(_other_range)
			,is_visit_first_range(true)
		{}

		bool next()
		{
			if (range.next())
			{
				return true;
			}

			if (other_range.next())
			{
				is_visit_first_range = false;
				return true;
			}

			return false;		
		}

		return_type front()
		{
			if (is_visit_first_range)
				return range.front();
			else
				return other_range.front();
		}
	private:
		TRange		range;
		TOtherRange	other_range;
		bool		is_visit_first_range;
	};

	template<typename TRange>
	class linq
	{
	public:
		linq(const TRange& _range)
			:range(_range)
		{}

		typedef typename TRange::value_type		value_type;
		typedef typename TRange::return_type	return_type;

		template<typename TFunction>
		auto where(const TFunction& predicate)->linq<where_range<TRange, TFunction>>
		{
			auto result = where_range<TRange, TFunction>(range, predicate);
			return linq<where_range<TRange, TFunction>>(result);
		}

		template<typename TFunction>
		auto select(const TFunction& function)->linq<select_range<TRange, TFunction>>
		{
			auto result = select_range<TRange, TFunction>(range, function);
			return linq<select_range<TRange, TFunction>>(result);
		}

		template<typename TFunction>
		auto select_many(const TFunction& function)->linq<select_many_range<TRange, TFunction>>
		{
			auto result = select_many_range<TRange, TFunction>(range, function);
			return linq<select_many_range<TRange, TFunction>>(result);
		}

		template<typename TValue>
		struct get_range_type_helper
		{
			typedef typename storage_range<std::vector<typename cleanup_type<TValue>::type>> type;
		};

		auto concat(const typename TRange::value_type value)->linq<concat_range<TRange, typename get_range_type_helper<typename TRange::value_type>::type>>
		{
			return concat(singleton(value));			
		}

		template<typename TOtherRange>
		auto concat(const linq<TOtherRange>& other_range)->linq<concat_range<TRange, TOtherRange>>
		{
			auto result = concat_range<TRange, TOtherRange>(range, other_range.range);
			return linq<concat_range<TRange, TOtherRange>>(result);
		}

		auto ref()->linq<ref_range<TRange>>
		{
			return linq<ref_range<TRange>>(range);
		}

		auto take(int count)->linq<take_range<TRange>>
		{
			auto result = take_range<TRange>(range, count);
			return linq<take_range<TRange>>(result);
		}

		template<typename TFunction>
		auto aggregate(typename TRange::value_type init_value, const TFunction& function)
			->typename TRange::value_type
		{
			auto range_copy = range;
			while (range_copy.next())
			{
				init_value = function(init_value, range_copy.front());
			}
			return init_value;
		}

		//any
		template<typename TFunction>
		bool any(const TFunction& function)
		{
			auto range_copy = range;
			while (range_copy.next())
			{
				if (function(range_copy.front()))
					return true;
			}
			return false;
		}

		template<typename TFunction>
		bool all(const TFunction& function)
		{
			auto range_copy = range;
			while (range_copy.next())
			{
				if (!function(range_copy.front()))
					return false;
			}
			return true;
		}

		template<typename TOtherRange>
		bool sequence_equal(linq<TOtherRange> other_range)
		{
			auto range_copy = range;

			bool range_next = range_copy.next();
			bool other_range_next = other_range.range.next();
		
			while (range_next && other_range_next)
			{
				if (range_copy.front() != other_range.range.front())
				{
					return false;
				}
				range_next = range_copy.next();
				other_range_next = other_range.range.next(); 
			}

			if (range_next!=other_range_next)
			{
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
	auto from(const TContainer& container)->linq<basic_range<decltype(std::begin(container))>>
	{
		typedef decltype(std::begin(container)) TIterator;

		auto range = basic_range<TIterator>(std::begin(container), std::end(container));
		return linq<basic_range<TIterator>>(range);
	}


	//template<typename TContainer>
	//auto from(TContainer&& container)->linq<storage_range<TContainer>>
	//{
	//	auto range = storage_range<TContainer>(std::forward<TContainer>(container));
	//	return linq<storage_range<TContainer>>(range);
	//}

	template<typename TContainer>
	auto from_copy(TContainer&& container)->linq<storage_range<TContainer>>
	{
		auto range = storage_range<TContainer>(std::forward<TContainer>(container));
		return linq<storage_range<TContainer>>(range);
	}

	template<typename T,size_t N>
	auto from_copy(T(&_array)[N])->linq<storage_range<std::vector<T>>>
	{
		std::vector<T> container(std::begin(_array),std::end(_array));
		auto range = storage_range<std::vector<T>>(std::move(container));
		return linq<storage_range<std::vector<T>>>(range);
	}


	template<typename TValue>
	auto singleton(TValue&& value)->linq<storage_range<std::vector<typename cleanup_type<TValue>::type>>>
	{
		std::vector<typename cleanup_type<TValue>::type> container;
		container.push_back(std::forward<TValue>(value));
		auto range = storage_range<std::vector<typename cleanup_type<TValue>::type>>(std::move(container));
		return linq<storage_range<std::vector<typename cleanup_type<TValue>::type>>>(range);
	}
}


//todo
//concat
//sequence_equal
//join
//order
//reverse
//distinct
//union_with
//intersect_with
//except