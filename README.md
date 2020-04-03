# TblGen - A tool for structured code generation

TblGen allows you to define data in a structured and human-readable format, and then generate code based on that data. To do this, you need to provide two types of files:

- A _TblGen definition_ (.tg) file, which contains the actual data you want to generate code based on.
- A _backend_, which can either be a dynamic library containing functions with a specific signature, or a _template file_.

For more information on the syntax of these files, refer to the documentation section.

## Building TblGen from source

TblGen is built using CMake. It requires a C++17 compatible compiler (gcc or clang) to work. To build TblGen, simply clone the repository and run

```
cmake .
make
```

## Running the examples

Examples for usage of TblGen are found in the `examples/` directory. To run them, the `tblgen` executable needs to be available in your PATH. Every example includes a `run.sh` script, which builds the necessary backend (if one is used) and executes TblGen.

# Documentation

## Tblgen (.tg) file syntax

This file is where you define the data that is then parsed, instantiated, and passed to your backend or template. There are two basic declaration types that make up a tg file: classes and records. Both of these contain properties of one of the basic _data types_ of TblGen.

### Data types

#### Integers

Integers represent integral numbers within a fixed range. There are ten different integer types in TblGen, five of which are signed and five unsigned. Signed integer types start with `i`, while unsigned types start with `u`; both followed by the number of bits (1, 8, 16, 32 or 64). For example, the equivalent of a Java `int`, which is a signed integer of 32-bits, is `i32`. An single byte can be represented by the `u8` type, while a boolean value can use the `i1` or `u1` types, which are equivalent.

#### Floating point numbers

There are two floating point types in TblGen, `float` and `double`. Both behave as you would expect according to the IEEE754 specification. A `float` has a size of 32 bits, while a `double` has 64 bits.

#### Strings

Strings are sequences of 8-bit characters. There is no additional unicode verification or normalization performed on them; because of their similar format to ASCII however, UTF-8 strings are supported. The single string type in TblGen is `string`.

#### Lists

A list is a container type that can contain multiple values of the same type. Lists types are introduced using the `list` keyword, followed by the element type in angle braces.

#### Dictionaries

Dictionaries are a hash map with string keys. They are introduced with the `dict` keyword, followed by the value type in angle braces.

#### Custom types

In the next chapter you will learn how to define custom data types using the `class` and `enum` keywords. Each class (or enum) can be used as a data type in all locations where the builtin types can be used.

### Expressions

#### Integer literals

The syntax of integer literals is based on C, with the addition of binary literals. Integer values can be represented in binary, decimal, octal or hexadecimal bases.

```typescript
// Prefix `0b` or `0B` for binary
let binaryTen = 0b1010 // => i32

// No prefix for decimal
let decimalTen = 10  // => i32

// Prefix `0` for octal
let octalTen: i64 = 012  // => i64

// Prefix `0x` or `0X` for hexadecimal
let hexaDecimalTen: u32 = 0xA  // => u32

// Integer literals can be negative contain underscores.
let readableBigNumber = -100_000_000 // => i32

// Scientific notation
let exponent = +10e8 // => i32
```

Integer literals can start with a `+` or `-` sign, and can contain underscores for improved readability. Exponents can be specified using an `e` suffix.

If no contextual information is available, integer literals will default to the `i32` type. The compiler will issue an error if a value does not fit in the given type.

#### Character literals

Character literals are surrounded by single quotes and may contain any single ASCII-character. The default type of these literals is `u8`, an unsigned byte representing the ASCII value of the character. Unicode literals are not supported at this time. 

```typescript
let char1 = 'A'  // => u8
let char2: i32 = '+'  // => i32
```

#### Boolean literals

Boolean literals are introduced by the `true` and `false` keywords. Their default type is `i1`. `true` is equivalent to the number 1, while `false` is 0.

```typescript
let writingDocumentationIsExciting = false
```

#### Floating point literals

Floating point literals are once again similar to the C notation. A `.` is used as the decimal separator, and exponents can be specified with the `e` or `p` syntax for hexadecimal floating point literals. The default type of floating point literals is `float`.

```typescript
let PI = 3.141592 // => float
let exponent: double = 1e-8 // => double
let hexLiteral = 0x0.123p-1 // => float
```

#### String literals

String literals are surrounded by double quotes and may contain any valid sequence of UTF-8 characters. String literals are always of `string` type.

```typescript
let message = "Hello, World! 🌎" // => string
```

#### Code literals

Because TblGen is primarily meant as a tool for code generation, code literals have a special syntax in the language. They are delimited using curly braces, and are otherwise treated like multi-line strings. Code literals are always of `string` type.

```typescript
let code = {
    var header = document.getElementById("div#header")
    console.log(header)
}  // => string
```

#### List literals

A list literal is surrounded by square brackets and contains a comma-separated list of values. If the values are all of the same type, the type of the list can be inferred.

A list's members can be accessed by providing a 0-based index value. The compiler will raise an error if this value is higher than or equal to the list's size.

```typescript
// Define a list with three elements.
let myFamily = ["Max", "Maria", "Björn"]  // => list<string>
let firstFamilyMember = myFamily[0]
```

#### Dictionary literals

A list literal is surrounded by square brackets and contains a comma-separated list of key-value pairs, where the key is separated from the value with a colon. If the values are all of the same type, the type of the dictionary can be inferred.

A dictionary's members are accessed by providing a string typed key. The compiler will raise an error if this key is not present in the dictionary.

```typescript
// Define a dictionary with three key-value pairs.
let cityPopulations = [
    "Berlin": 3_769_000,
    "New York City": 8_623_000,
    "Paris": 2_148_000,
] // => dict<i32>

let populationOfBerlin = cityPopulations["Berlin"]
```

#### Records

Every record you define in your TblGen file can be used as an expression. Its respective type is that of the class it implements. If a record implements more than one class and there is no contextual information available, you have to specify which type you want.

```typescript
class Animal
def Dog: Animal

let max: Animal = Dog
```

#### Anonymous records

Sometimes it can be cumbersome to define custom records with `def` declarations if the records are small and only referenced once. In this case, you can create anonymous instantiations of a class. These can not be referenced outside of their creation site, and since they can only provide values for template properties, they cannot be used for classes with required body properties.

```typescript
class Human<let name: string>

let myFriends: list<Human> = [
    Human<"Marc">,
    Human<"Fred">,
    Human<"Joe">,
]
```

#### Enum values

Enums are special types of classes that can only take one of several predefined values. An enum case is referenced by using the enum name, followed by a colon and the name of the enum case. If the type can be inferred from context, you can leave out the name of the enum and just leave a leading colon.

```typescript
enum MyEnum {
    Value1,
    Value2,
    Value3,
}

let myEnum1 = MyEnum.Value1

// Leave out the enum name if it can be inferred
let myEnum2: MyEnum = .Value2
```

#### Functions

TblGen provides some builtin functions to manipulate expressions. Functions are referenced using an exclamation mark followed by the function name. The arguments of the function are specified as a comma-separated list in parentheses after the function name. Currently, the following functions are supported:

- **!eq(v1: any, v2: any): i1**

    Compares two values for equality.
- **!ne(v1: any, v2: any): i1**

    Compares two values for inequality.
- **![gt, lt, ge, le](v1: [integer / floating pt], v2: [integer / floating pt]): i1**

    Compares two numerical values (gt: greater than, lt: less than, ge: greater than or equal to, le: less than or equal to).
- **!not(i: [integer]): i1**

    Returns `false` if `i` is equal to zero, `true` otherwise.
- **!allof(className: string): list<?>**

    Returns a list containing all implementations of the class `className`.
- **!push(l: list\<T>, value: T): list\<T>**

    Return a new list of type `T`, containing all of the elements of `l` as well as `value`.
- **!pop(l: list\<T>): list\<T>**

    Remove the last element from `l` and return the result as a new list.
- **!first(l: list\<T>): T**

    Returns the first element of `l`.
- **!last(l: list\<T>): T**

    Returns the last element of `l`.
- **!empty(l: list\<T>): i1**

    Returns `true` if `l` contains no elements, `false` otherwise.
- **!empty(l: dict\<T>): i1**

    Returns `true` if `l` contains no elements, `false` otherwise.
- **!contains(l: list\<T>, value: T): i1**

    Returns `true`, if `value` is contained in `l`, `false` otherwise.
- **!contains(l: dict\<T>, key: string, value: T): i1**

    Returns `true`, if a pair of `key` and `value` is contained in `l`, `false` otherwise.
- **!containsKey(l: dict\<T>, key: string): i1**

    Returns `true`, if `key` is a valid key of `l`, `false` otherwise.
- **!concat(l1: list\<T>, l2: list\<T>): list\<T>**

    Returns a new list containing the elements of `l1` and `l2`.
- **!str_concat(s1: string, s2: string): string**

    Concatenates `s1` and `s2` and returns the result as a new string.
- **!upper(s: string): string**

    Returns the uppercase version of `s`.
- **!lower(s: string): string**

    Returns the lowercase version of `s`.


### Classes

Classes in TblGen are similar to their counterparts in object-oriented programming languages. They allow you to define the _structure_ of your data, but are not actual data points themselves. A class can define _properties_ with a name and a type. Additionally, classes can inherit from other classes.

#### Properties

A class property is a single point of data that each instance of that class must provide. A property is defined with the `let` keyword, and can either be defined as a template parameter after the class name, or in the class body. A template property must be provided whenever the class name is referenced, while a body property can be implemented elsewhere.

```typescript
class ExampleClassOne<let templateProperty: string> {
    let bodyProperty: i32
    let bodyPropertyWithDefaultValue: i1 = true
}

class ExampleClassTwo {
    let bodyProperty: list<string>
}

class ExampleClassThree<
    let templatePropertyWithDefaultValue: string = "">
```

Both types of properties can optionally define a default value, which will be used if no value is given.

#### Inheritance

After the class name, you can optionally specify one or more inheritance clauses. A class inheriting from another class inherits all of its body properties, and those of its superclasses recursively. Template properties must be specified in the inheritance clause.

```typescript
class Animal<let laysEggs: i1>

// All birds lay eggs
class Bird : Animal<true>

// Some mammals (e.g. platypus) lay eggs 
class Mammal<let laysEggs: i1 = false> : Animal<laysEggs>
```

### Records

Records are instantiations of one or more classes. They represent the actual data in your TblGen file. Records are introduced using the `def` keyword followed by the record name. A record can implement any number of classes, but must implement at least one.

```typescript
class Animal {
    let sound: string
}

def Dog : Animal {
    sound = "woof"
}

def Cat : Animal {
    sound = "meow"
}
```
