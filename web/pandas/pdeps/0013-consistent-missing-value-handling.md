# PDEP-13: Consistent missing value handling (with a single NA scalar)

- Created: 16 September 2023
- Status: Draft
- Discussion: [#52711](https://github.com/pandas-dev/pandas/pull/52711)
              [#52509](https://github.com/pandas-dev/pandas/issues/52509)
- Author: [Joris Van den Bossche](https://github.com/jorisvandenbossche)
- Revision: 1

Proposal: strive for a consistent handling of missing values across data types in pandas through:

- Single NA object to denote a scalar missing value
- Use mask based approach to store missing values (on the array level) for all dtypes

This document mainly focuses on the first item (the new NA scalar) and its consequences, as this has the most user-facing impact and potential API changes to discuss. The mask-based approach to store the missing value information (like the nullable integers currently do) is also important, but more an implementation detail (although important one) that can be discussed separately.

This is being discussed at:

- Mailing list: https://mail.python.org/pipermail/pandas-dev/2019-October/001099.html
- Github issue on NA scalar: https://github.com/pandas-dev/pandas/issues/28095
- Boolean dtype with missing value support: https://github.com/pandas-dev/pandas/issues/28778

## Summary

A summary of the proposal is to introduce **a new NA value (singleton) for representing scalar missing values** (instead of np.nan) that can be used consistently across all data types.

Motivation for this change:

- **Consistent user interface.**
  Currently, the value you get back for a missing scalar (eg from scalar access `s[idx]`) depends on the data type (np.nan for many, but pd.NaT for datetime-likes). Some types support missing values, others don't. This proposal would ensure you get back `pd.NA` regardless of the dtype.
- **No "mis-use" of the np.nan floating point value.**
  The NaN value is a specific floating point value, and not necessarily an indicator for missing values (although pandas has always used it that way). And because we also use it for other dtypes, you get back a float value for non-float dtypes, giving misleading dtype information.
- **A missing value that behaves accordingly.**
  Our current behaviour of missing values is inherited of the np.nan behaviour. Other languages that have a NA/NULL value that is distinguished from NaN (eg Julia, SQL, R) typically have different behaviour in comparison and logical operations. For example, comparison with NA could give NA instead of False, and consequently we need to have a boolean dtype with NA support. A new NA value opens up the possibility of having such behaviour.
- An "NA" scalar **matches the terminology** that is used throughout pandas in functions and argument names (`isna`, `dropna`, `fillna`, `skipna`, ...).


## Motivation

Currently, we use `np.nan` as missing value indicator in floating point data, `np.nan` or `None` in object dtype data (eg strings, or booleans with missing values are cast to object), `pd.NaT` in datetimelike data. For categorical, interval or nullable integer data, the physical storage is different for each of them, but they return `np.nan` on access.

This situation has some drawbacks:

- *Inconsistent user interface.*
  The value you get back for a missing scalar depends on the data type (eg through scalar access `s[idx]`). Some dtypes have their custom "not a value" (float, datetime-likes), other re-use `np.nan`. Some types support missing values, some types don't.
- *Proliferation of "not a value" types.*
  For example, when introducing a string dtype in the near future, do we want to add another "not a string" (NaS) object in line with NaN and NaT? But how does this scale with additional types, and how do functions (eg `pd.isna`) recognize those, certainly in case of externally defined ExtensionArrays? (See for example discussion in [GH-27825](https://github.com/pandas-dev/pandas/issues/27825)).
- *"Mis-use" of the `np.nan` floating point value.*
  The [NaN definition](https://en.wikipedia.org/wiki/NaN) if part of the IEEE 754 floating-point standard. It is an actual, specific float value (and therefore also only representable in a float array), and not necessarily an indicator for "missing" values.
  Other libraries or languages that have proper missing value support also distinguish between NaN and NA (e.g Arrow has `NULL`, [R has `NA`](https://adv-r.hadley.nz/vectors-chap.html#missing-values), [Julia has `missing`](https://julialang.org/blog/2018/06/missing)).
  This is not only a "theoretical" issue, but also means that functionality from e.g. numpy that can be used to work with `np.nan` (e.g. `np.isnan`) does not work generally with data from pandas that holds NaNs. It is also strange to get a value back of type float from non-float datatypes, as this gives misleading dtype information.



## Proposal

Therefore, this proposal puts forward a single `pd.NA` value that is used as missing value indicator across data types in the user facing interfaces (under the hood, it could still be implemented in the Array using a mask-based approach or sentinel-based approach).


A simplified definition (for illustration purposes) of such a `NA` singleton would look like:

```python
class _NAType:

    __eq__(self, other):
        return False  # or return NA (see below for discussion)

    __str__(self):
        return "NA"

    ...

NA = _NAType()
```

Advantages:

- Consistent user experience for accessing and displaying missing values across data types (independent from the data types specific implementation).
- A "proper" missing value (no float type) which can behave as missing in operations.
- Matches the terminology that is used throughout pandas in function and argument names (`isna`, `dropna`, `fillna`, `skipna`, ...).


This object would be returned when accessing a scalar missing element:

```python
>>> s = pd.Series([1, 2, pd.NA], dtype='Int64')
>>> s
0    1
1    2
2   NA
dtype: Int64
>>> s[2]
NA
```


## Counter arguments

### Similar scalar API for missing values

The dtype-specific "not a value" objects can have an API similar to the normal scalars of that data type. For example, `pd.NaT` has the same methods and attributes as a `pd.Timestamp`. This makes that you more easily write robust code that works with scalars accessed from a timestamp series, whether they are missing or not.

Having a "not a value" object for each data type could make that:

```python
>>> isinstance(s[scalar], s.dtype.type)
```

always being true, regardless of whether the scalar is NA or valid?

However, this is currently only true for datetimelike data (using `pd.NaT`), and not generally the case anyway for other data types (unless we would introduce also a "not an integer", "not an interval", ...).
And even for datetimelike data, the above `isinstance` check does not hold true as `pd.NaT` is only an instance of ``datetime.datetime`` (but see discussion below regarding discussion of adding multiple NaTs).


### Predictability of types

When having a non-dtype-specific NA value, the resulting dtype of certain operations will be undefined.

For example, consider a `timedelta64[ns]` Series. Depending on the type that is added, the type of the resulting Series differs:

```python
>>> s = pd.Series([pd.Timedelta('1 days'), pd.Timedelta('2 days')])

>>> s + pd.Timedelta('1 days')
0   2 days
1   3 days
dtype: timedelta64[ns]

>>> s + pd.Timestamp('2019-01-01')
0   2019-01-02
1   2019-01-03
dtype: datetime64[ns]

>>> s + pd.NaT
0   NaT
1   NaT
dtype: timedelta64[ns]
```

In this case, `pd.NaT` is considered as another (but missing) timedelta and not as timestamp. Exactly for this reason, it was proposed to add versions of "NaT" specific to timedelta and period to be able to distinguish this ([GH-24983](https://github.com/pandas-dev/pandas/issues/24983)).

So we currently already have this issue to some extent. But, it could be solved by introducing multiple "not a value" scalars for each dtype. While introducing a single scalar `pd.NA` will make this worse.

Related to this is the introduction of a "NA dtype", which is what could be used in such "undefined" cases (to at least ensure the result has no incorrect dtype). See more on that below.


### Ecosystem around use of np.nan as missing value indicator

In Python, and in the pandas-based data analysis ecosystem specifically, people have used `np.nan` generally as the missing value indicator in absence of a "true" missing value.
And a lot of code is built upon that assumption (e.g. scikit-learn also uses it as default missing value indicator in imputation methods).

This proposal does not change anything for numpy-based libraries. Algorithmic code, when the pandas data gets converted to numpy arrays, currently works with float data and np.nan if there are missing values present. This would not change (at least initially, we would still output float with np.nan for missing values on conversion to numpy).

### Backwards compatibility

A change to output `pd.NA` instead of `np.nan` or `pd.NaT` on access for the existing data types would certainly be a backwards incompatible change. Therefore, we will need to introduce this gradually and in the beginning optionally. But personally, I think the consistent interface is worth the breakages, even for the default data types (on the longer term, in a major new pandas version). See more below.


## Implementation strategy

This is of course not backwards compatible. It can therefore only be implemented gradually. Possible steps:

**On the short term (ideally for 1.0)**:

* Already implement and provide the `pd.NA` scalar, and recognize it in the appropriate places as missing value (e.g. `pd.isna`). This way, it can already be used in external ExtentionArrays
* Implement a `BooleanArray` with support for missing values and appropriate NA behaviour. To start, we can just use a numpy masked array approach (similar to the existing IntegerArray), not involving any pyarrow memory optimizations.
  This boolean array will initially only be used as the result of boolean operations involving dtypes that use `pd.NA` as missing value indicator.
* Start using this BooleanArray as the boolean result of comparison operations for IntegerArray/StringArray (breaking change for nullable integers)
  * Other arrays will keep using the numpy bool, this means we have two "boolean" dtypes side by side with different behaviour, and which one you get depends on the original data type (potentially confusing for users)
*  Start using `pd.NA` as the missing value indicator for Integer/String/BooleanArray (breaking change for nullable integers)

Although a breaking change, I think it is important to start using the `pd.NA` and `BooleanArray` retro-actively for the experimental nullable IntegerArray, since this was only introduced recently and it is the perfect use case for it.

**On the intermediate term**

* Investigate if it can be implemented optionally for other data types and "activited" to have users opt-in for existing dtypes.

**On the long term (2.0?)**

* Start using the types with NA support as the default types. This can only happen in a major (breaking) release.


## Discussion points / implementation details

### Type stability

Or, what is the dtype of the result given the dtypes of the input?

- What is `pd.Series([pd.NA]).dtype`? (with `np.nan` this would be `float64`)
- What is `pd.Series([pd.Timestamp('2000-01-01')]) - pd.NA`? (could be either `Timestamp` or `Timedelta`)
- What is `pd.Series([1], dtype='Int64') + pd.NA`? (could be either int of float)

For those operations with NA where the resulting dtype is not obvious, a NADtype result could be returned:

```python
>>> pd.Series([0, 1]) + pd.NA
Series([NA, NA], dtype=NADtype)
# or
>>> pd.Series([0, 1]) + pd.NA
Series([NA, NA], dtype=int/float)
```

This would mainly occur with scalar NA values. Because when doing operations with arrays (even if all NA), the dtype of the result can be known from the input dtypes:

```python
>>> pd.Series([0, 1]) + pd.Series([pd.NA, pd.NA], dtype=int)
Series([NA, NA], dtype=int)

>>> pd.Series([0, 1]) + pd.Series([pd.NA, pd.NA], dtype=float)
Series([NA, NA], dtype=float)
```

Different options to deal with this:

1. For those cases with a potentially undefined behaviour, make a choice.
  This way, we still have consistent defined behaviour, although it is somewhat arbitrary. For example, we could choose to preserve the original type if possible: `int + NA = int`, `Timedelta + NA = Timedelta`, `Timestamp - NA = Timestamp`. Or, we could choose to interpret the NA as the same type as the value, if possible: with this logic third example would become `Timestamp - NA = Timedelta`.

2. Introduce an "NA dtype".
   This way, we don't need to make this arbitrary choice, and preserve the fact that the resulting dtype is actually unknown. However, this gives a new dtype that needs to be handled. See item below on that.

3. Have dtype-specific realizations of "NA".
  Instead of a single NA singleton, we could have NA variants for each dtype (so it can carry information on its originating type), while for the user it still all looks as "NA". However, this seems a lot more work to implement (would eg this "NA_period" have all the methods of a Period?) and also potentially to code against. And we would also still need to define the behaviour for the generic NA anyway.

### Do we need a `NA` dtype?

For the all-NA cases with undefined dtype, an "NA dtype" could be used (see above).
Additionally, it could also be used as the dtype of `pd.Series([pd.NA])` upon creation (currently, `pd.Series([np.nan])` gives a float dtype, as well as `pd.Series([])`, although we want to change this to object dtype).

For this, we could introduce an "NA dtype" (and a NAArray which only needs to store a length):

```python
>>> pd.Series([pd.NA, pd.NA])
Series([NA, NA], dtype=NADtype)
```

In element-wise operations it would propagate NAs, and it can be upcasted to any other dtype (eg in concat).

Alternatively, we could also choose a default, existing dtype to store all-NA arrays. For example object dtype.

Some open questions: how should it behave in dtype specific methods? (eg `.dt` or `.str` methods) Should those work (as if it were all NA of that dtype), or error?



### Behaviour in comparison operations

In numerical operations, NA propagates (see also above). But for boolean operations the situation is less clear. Currently, we use the behaviour of `np.nan` for missing values in pandas. This means:

```python
>>> np.nan == 1
False
>>> np.nan < 1
False
>>> np.nan != 1
True
```

However, a missing value could also propagate:

```python
>>> pd.NA == 1
NA
>>> pd.NA < 1
NA
>>> PD.NA != 1
NA
```

This is for example what [Julia](https://docs.julialang.org/en/v1/manual/missing/index.html#Equality-and-Comparison-Operators-1) and R do.

### Boolean data type with missing values and logical operations

If we propagate NA in comparison operations (see above), the consequence is that you end up with boolean masks with missing values. This means that we need to support a boolean dtype with NA support, and define the behaviour in logical operations and indexing.

- What to return in logical operations? (eg `True & NA`)
- How to handle NA's in indexing operations (raise error, or consider as False ?)


Currently, the logical operations are not very consistently defined. On Series/DataFrame, it returns mostly False, and for scalars it is not defined:

```python
>>> pd.Series([True, False, np.nan]) & True
0     True
1    False
2    False
dtype: bool

>>> pd.Series([True, False, np.nan]) | True
0     True
1     True
2    False
dtype: bool

>>> np.nan & True
TypeError: unsupported operand type(s) for &: 'float' and 'bool'
```

For those logical operations, Julia, R and SQL choose for the "three-valued logic" (only propagate missing values when it is logically required). See https://docs.julialang.org/en/v1/manual/missing/index.html for a good explanation. This would give:


```python
>>> pd.Series([True, False, pd.NA]) & True
0     True
1    False
2       NA
dtype: bool

>>> pd.Series([True, False, pd.NA]) | True
0     True
1     True
2     True
dtype: bool

>>> pd.NA & True
NA

>>> pd.NA & False
False

>>> pd.NA | True
True

>>> pd.NA | False
NA
```



Sidenote: we regard `None` as missing value, which means that the behaviour of None in a Series differs compared to the scalar behaviour of None (another reason to not use `None` as missing value indicator).
