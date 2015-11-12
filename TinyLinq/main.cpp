// TinyLinq.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "TinyLinq.h"
#include "gtest\gtest.h"

#include <vector>
#include <iostream>
using namespace std;
using namespace TinyLinq;

int main(int c, char* v[])
{
	int arr[] = { 1,2,3,4,5,6,7,8 };
	std::vector<int> a(std::begin(arr),std::end(arr));
	//auto r = from(a)
	//	.where([=](int a) {return a % 2 == 0; })
	//	.select([=](int a) {return a / 2; })
	//	.take(2)
	//	.to_vector();
	////auto w1 = to_where(r, [=](int a) {return a % 2 == 0; });
	////auto w = to_where(w1, [=](int a) {return a % 4 == 0; });


	//for (; b.next();)
	//{
	//	cout << b.front() << " ";
	//}

	::testing::InitGoogleTest(&c,v);
	auto ret = RUN_ALL_TESTS();
	return ret;
}

