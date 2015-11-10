#include "gtest\gtest.h"
#include "TinyLinq.h"
#include <numeric>
using namespace TinyLinq;
using namespace std;

#define FOR_EACH(iter,_container) \
	for (auto iter = std::begin(_container);iter!=std::end(_container);++iter)


int test_int_array[] = {1,2,3,4,5,6,7,8,9,10,0};

auto is_even = [=](int n){return n%2==0;};
auto is_odd  = [=](int n) {return n%2==1;};
auto double_it = [=](int n){return n*2;};
auto add = [=](int a,int b){return a+b;};


struct Person
{
	int			id;
	std::string name;
};

Person fabio = {1,"fabio"};
Person ivan = {2,"ivan"};
Person kidding = {3,"kidding"};
Person person_array[] = {fabio,ivan,kidding};

TEST(test_where,all)
{
	auto c = from(test_int_array).where(is_even).to_vector();

	std::vector<int> result;
	for(int i = 0; i < sizeof(test_int_array)/sizeof(int); ++i)
	{
		if (is_even(test_int_array[i]))
			result.push_back(test_int_array[i]);
	}
	EXPECT_EQ(c,result);
}

TEST(test_select,all)
{
	auto c = from(test_int_array).select(double_it).to_vector();

	std::vector<int> result;
	for(int i = 0; i < sizeof(test_int_array)/sizeof(int); ++i)
	{
		result.push_back(double_it(test_int_array[i]));
	}
	EXPECT_EQ(c,result);
}

TEST(test_select_many,all)
{
	auto c = from(person_array)
			.select_many([=](const Person& person){return (person.name);})
			.to_vector();


	for(size_t i = 0; i < c.size(); ++i)
	{
		printf("%c",c[i]);
	}
	//EXPECT_EQ(c,result);
}

TEST(test_ref,all)
{
	int test_int_array[] = {1,2,3,4,5,6,7,8,9,10,0};
	auto a = from(test_int_array).ref().to_vector();
	auto b = from(test_int_array).select(double_it).to_vector();

	for (int i = 0; i < sizeof(test_int_array)/sizeof(int); ++i)
	{
		test_int_array[i] *=2;
	}

	for (int i = 0; i < sizeof(test_int_array)/sizeof(int); ++i)
	{
		EXPECT_EQ(a[i].get(),b[i]);
	}
}

TEST(test_take,all)
{
	int count = 3;
	auto c = from(test_int_array).take(count).to_vector();
	for (int i = 0; i < sizeof(test_int_array)/sizeof(int) && i < count; ++i)
	{
		EXPECT_EQ(test_int_array[i],c[i]);
	}
}

TEST(test_aggregate,all)
{
	auto a = from(test_int_array).aggregate(0,add);
	auto b = std::accumulate(std::begin(test_int_array),std::end(test_int_array),0);
	EXPECT_EQ(a,b);
}

TEST(test_any,all)
{
	auto a = from(test_int_array).select(double_it).to_vector();
	EXPECT_EQ(from(a).any(is_odd),false);
}

TEST(test_all,all)
{
	auto a = from(test_int_array).select(double_it).to_vector();
	EXPECT_EQ(from(a).all(is_even),true);
}