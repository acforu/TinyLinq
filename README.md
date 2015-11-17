# TinyLinq [![Build status](https://ci.appveyor.com/api/projects/status/4f9egwqnhtdwabeu/branch/master?svg=true)](https://ci.appveyor.com/project/acforu/tinylinq/branch/master)
The C++ implementation of minimum useful set of LINQ

### where
```c++
int array[] = {1,2,3,4,5};
auto result = from(array)
            .where([](int i){return i%2 == 0;})
            .to_vector();

// result is 2,4

```

### select
```c++
int array[] = {1,2,3,4,5};
auto result = from(array)
            .select([](int i){return i * 2;})
            .to_vector();

// result is 2,4,6,8,10
```

### select_many
```c++
struct Person
{
	int			id;
	std::string name;
};

Person x = {1,"abc"};
Person y = {2,"def"};
Person z = {3,"ghij"};

Person array[] = {x,y,z};
	auto result = from(array)
			.select_many([=](const Person& person)->string {return (person.name);})
			.to_vector();

// result is a,b,c,d,e,f,g,h,i,j
```

### concat
```c++
int array[] = {1,2,3};
	auto c = from(array)
		.concat(single(4))
		.concat(5)
		.to_vector();

// result is 1,2,3,4,5
```

### take
```c++
int array[] = {1,2,3,4,5};
	auto c = from(array)
		.take(3)
		.to_vector();

// result is 1,2,3
```

The support interface list:
* from
* from_copy
* where
* select
* select_many
* ref
* concat
* take
* aggregate
* any
* all
* join
