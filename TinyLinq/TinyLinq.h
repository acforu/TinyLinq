#pragma once
#include <type_traits>
#include <functional>
#include <vector>
#include <map>
namespace TinyLinq
{
	template<typename TValue>
	struct cleanup_type
	{
		typedef typename std::decay<TValue>::type type;
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

		typedef decltype(dummy_function()(dummy_arg()))			type;
	};

	template<typename TFunction, typename TArg1, typename TArg2>
	struct extract_return_type_2_args
	{
		static TFunction	dummy_function();
		static TArg1		dummy_arg1();
		static TArg2		dummy_arg2();

		typedef decltype(dummy_function()(dummy_arg1(),dummy_arg2())) type;
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
				inner_range = to_inner_range(function(range.front()), std::is_lvalue_reference<inner_data_type>());
				return inner_range->next();
			}

			inner_range.reset();
			return false;
		}

		std::shared_ptr<inner_range_type> to_inner_range(inner_data_type&& ref,std::true_type)
		{
			return make_shared<inner_range_type>(inner_range_type(std::begin(ref), std::end(ref)));
		}

		std::shared_ptr<inner_range_type> to_inner_range(inner_data_type&& ref,std::false_type)
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

	template<
		typename TRange,
		typename TOtherRange,
		typename TKeySelector,
		typename TOtherKeySelector,
		typename TCombiner>
	class join_range
	{
	public:

		typedef typename extract_return_type<TKeySelector,typename TRange::value_type>::type				raw_key_type;
		typedef typename cleanup_type<raw_key_type>::type													key_type;
		typedef typename extract_return_type<TOtherKeySelector,typename TOtherRange::value_type>::type		raw_other_key_type;
		typedef typename cleanup_type<raw_other_key_type>::type												other_key_type;
		typedef typename std::multimap<other_key_type, typename TOtherRange::value_type>					map_type;
		typedef typename map_type::iterator																	map_iterator_type;
		typedef typename extract_return_type_2_args<
			TCombiner,
			typename TRange::value_type,
			typename TOtherRange::value_type>::type															return_type;
		typedef typename cleanup_type<return_type>::type													value_type;


		join_range(
			const TRange&				_range,
			const TOtherRange&			_other_range,
			const TKeySelector&			_key_selector,
			const TOtherKeySelector&	_other_key_selector,
			const TCombiner&			_combiner)
			:key_selector(_key_selector)
			,other_key_selector(_other_key_selector)
			,range(_range)
			,other_range(_other_range)
			,combiner(_combiner)
			,is_first_visit(true)
		{

		}

		bool next()
		{
			if (is_first_visit)
			{
				is_first_visit = false;
				while (other_range.next())
				{
					auto value = other_range.front();
					auto key = other_key_selector(value);
					cache.insert(make_pair<key_type,TOtherRange::value_type>(std::move(key), std::move(value)));
				}

				cache_iterator = cache.end();
			}

			if (cache.empty())
				return false;

			if (cache_iterator != cache.end())
			{
				auto prev = cache_iterator;
				++cache_iterator;
				if (cache_iterator != cache.end() && prev->first == cache_iterator->first)
				{
					return true;
				}				
			}

			while (range.next())
			{
				key_type key = key_selector(range.front());
				cache_iterator = cache.find(key);
				if (cache_iterator != cache.end())
				{
					return true;
				}
			}

			return false;
		}

		return_type front()
		{
			return combiner(range.front(), cache_iterator->second);
		}

	private:
		TKeySelector		key_selector;
		TOtherKeySelector	other_key_selector;
		TRange				range;
		TOtherRange			other_range;
		TCombiner			combiner;
		bool				is_first_visit;
		map_type			cache;
		map_iterator_type	cache_iterator;
	};


	template<typename TRange>
	class linq
	{
	public:
		linq(const TRange& _range)
			:range(_range)
		{}

		//todo add move copy here

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
			return concat(single(value));			
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

		template<typename TOtherRange,typename TKeySelector,typename TOtherKeySelector,typename TCombiner>
		auto join(
			const linq<TOtherRange>& other_range,
			const TKeySelector& key_selector,
			const TOtherKeySelector& other_key_selector,
			const TCombiner& combinner)->
			linq<join_range<
			TRange,
			TOtherRange,
			TKeySelector,
			TOtherKeySelector,
			TCombiner >>
		{
			auto result = join_range<
				TRange,
				TOtherRange,
				TKeySelector,
				TOtherKeySelector,
				TCombiner>(range, other_range.range, key_selector, other_key_selector, combinner);
			return linq<join_range<
					TRange,
					TOtherRange,
					TKeySelector,
					TOtherKeySelector,
					TCombiner >>(result);
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

		size_t count()
		{
			size_t ret = 0;
			auto range_copy = range;
			while (range_copy.next())
			{
				++ret;
			}
			return ret;
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
			auto range_copy = range;
			while (range_copy.next())
			{
				v.push_back(range_copy.front());
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
	auto single(TValue&& value)->linq<storage_range<std::vector<typename cleanup_type<TValue>::type>>>
	{
		std::vector<typename cleanup_type<TValue>::type> container;
		container.push_back(std::forward<TValue>(value));
		auto range = storage_range<std::vector<typename cleanup_type<TValue>::type>>(std::move(container));
		return linq<storage_range<std::vector<typename cleanup_type<TValue>::type>>>(range);
	}
}


//todo
//order
//reverse
//distinct
//union_with
//intersect_with
//except