# C++ Features

This file will contain a list of C++ features I will
be using, and the justification for them. This may be of
particular interest to those who have been working with
Arduinos, following examples and believe that you are
programming in C++. Most Arduino programs are writting in
a pidgin of C++, often for good reasons.

## [Constexpr All the Things](https://www.youtube.com/watch?v=PJwd4JLYJJY)

I first learned this idiom from [Jason Turner](https://www.linkedin.com/in/lefticus/),
[@lefticus](https://twitter.com/lefticus) on Twitter.

The main advantage of declaring everything you can
as [constexpr](https://en.cppreference.com/w/cpp/language/constexpr) is
it declares that it is possible to evaluate the value of the function or
variable at compile time. This means that it can be
used to initialize constants and used at runtime.
Imagine that you have a `constexpr` version of a
`sin()` function:

```constexpr double sin(double const x);```

When you pass a compile time constant to this function
the compiler is able to evaluate it at compile time:

```c++
constexpr static double M_PI_4 = M_PI / 4;
constexpr static double SIN_M_PI_4 = sin(M_PI_4);
 ...
for (double x = -M_PI; x < M_PI; x += M_PI/10)
    std::cout << sin(x) << '\n';
```

You could do this other ways, but this is very
expressive and robust. Also when the compiler finds
a `constexpr` function in code that is being compiled
to binary it is able to compute the value at compile
time and replace all the generated code and function
calls that would be needed otherwise.

## [RAII](https://en.cppreference.com/w/cpp/language/raii)

This is a foundational C++ technique. You may also wish to
read [this blog post by Tom Dalling](https://www.tomdalling.com/blog/software-design/resource-acquisition-is-initialisation-raii-explained/)

## [std::tuple](https://en.cppreference.com/w/cpp/utility/tuple) and [Structured Binding](https://en.cppreference.com/w/cpp/language/structured_binding)

Returning multiple values by passing pointers to the location in
which results are to be saved has been a long time C, and therefore
C++ idiom. It is error prone however. Starting with C++17 there is
direct language and standard library support for a solution. C++ has
long had the ability to return structures and generics. With the
arrival of Structured Bindings and `auto` we have a nice tidy solution.
Let's have a look.

Compare the declaration of `DateTime::gettime()` from HamClock V5.4 when
I first encountered it.
```c++
void gettime(int& year, uint8_t& mon, uint8_t& day, uint8_t& h, uint8_t& m, uint8_t& s);
```
The calling sequence for this function is:
```c++
...
    int year;
    uint8_t mon, day, h, m, s;
    gettime(year, mon, day, h, m, s);
...
```
This is very common C++ idiom and does work, but is not intuitive and is prone
to errors. Using the reference instead of a pointer (the common C idiom) solves
some of the problems, but not all. Using Structured Binding and `std::tuple`
changes the declaration to:
```c++
std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t> gettime();
```
And the calling sequence is:
```c++
...
    auto [year, mon, day, h, m, s] = gettime();
...
```
Once one is familiar with the standard library and Structured Binding this is a
much more intuitive and compact, without being overly terse, expression of the
intent.

One argument I've heard against this idiom is that the return values aren't named
in the declaration. But even when provided how much information does `unit8_t &s`
realy give you? You could probably infer the intended meaning, a more expressive
name would certainly help. The proper place for the definitive meaning of each
argument or return value belongs in the documentation. So, if Doxygen is being 
used a proper declaration would be:
```c++
    /**
     * Compute the calendar date and clock time 
     * @return a tuple with year, month, day, hour, minute, seconds
     */
    [[nodiscard]] std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t>
            gettime() const;
```
This also makes use of the attribute `[[nodiscard]]` which will cause an error
if the return value is not used.
