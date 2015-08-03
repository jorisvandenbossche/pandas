Unified sorting API
===================

Rationale
---------

This goal is to create a unified and consistent API to Series & DataFrame sorting methods. At this moment there are different functions with different defaults, creating a confusing state.


Current situation
-----------------

The signatures and docstrings in pandas 0.16.2 are:

* **Series.sort**: [docstring](http://pandas.pydata.org/pandas-docs/version/0.16.2/generated/pandas.Series.sort.html)
  * ``Series.sort(axis=0, ascending=True, kind='quicksort', na_position='last', inplace=True)``
* **Series.order**: [docstring](http://pandas.pydata.org/pandas-docs/version/0.16.2/generated/pandas.Series.order.html)
  * ``Series.order(na_last=None, ascending=True, kind='quicksort', na_position='last', inplace=False)``
* **DataFrame.sort**: [docstring](http://pandas.pydata.org/pandas-docs/version/0.16.2/generated/pandas.DataFrame.sort.html)
  * ``DataFrame.sort(columns=None, axis=0, ascending=True, inplace=False, kind='quicksort', na_position='last')``

Further, you also have:

* **Series.sort_index**: [docstring](http://pandas.pydata.org/pandas-docs/version/0.16.2/generated/pandas.Series.sort_index.html)
  * ``Series.sort_index(ascending=True)``
* **DataFrame.sort_index**: [docstring](http://pandas.pydata.org/pandas-docs/version/0.16.2/generated/pandas.DataFrame.sort_index.html)
  * ``DataFrame.sort_index(axis=0, by=None, ascending=True, inplace=False, kind='quicksort', na_position='last')``
* **Series.sortlevel**: [docstring](http://pandas.pydata.org/pandas-docs/version/0.16.2/generated/pandas.Series.sortlevel.html)
  * ``Series.sortlevel(level=0, ascending=True, sort_remaining=True)``
* **DataFrame.sortlevel**: [docstring](http://pandas.pydata.org/pandas-docs/version/0.16.2/generated/pandas.DataFrame.sortlevel.html)
  * ``DataFrame.sortlevel(level=0, axis=0, ascending=True, inplace=False, sort_remaining=True)``

Some observations on inconsistencies:

* ``Series.sort`` is inplace (while all others, including ``DataFrame.sort``, not). This stems from keeping
  compatibility with `numpy.ndarray.sort`
* ``Series.sort`` defaults to quicksort, ``Series.order`` to mergesort
* ``Series.sort`` sorts by the values, while ``DataFrame.sort`` sorts by the index by default
* ``DataFrame.sort_index`` can also sort on the values (despite the name)
* ``sortlevel`` should more logically be included in ``sort_index``

Proposal and discussion points
------------------------------


### Unified sort/order into ``sorted`` method

**Current PR [#10726](https://github.com/pydata/pandas/pull/10726)**:

We have introduced a new method, ``.sorted()``, which is the merger of ``DataFrame.sort``, and ``Series.order``.
The existing methods: ``Series.sort``, ``Series.order``, ``DataFrame.sort`` will be deprecated and removed in a
future version of pandas. Note that the ``columns`` argument of ``DataFrame.sort`` has been renamed to ``by``.


Previous                        |   Replacement
------------------------------  |   -------------------------------
``Series.order()``              |   ``Series.sorted()``
``Series.sort()``               |   ``Series.sorted(inplace=True)``
``DataFrame.sort(columns=...)`` |   ``DataFrame.sorted(by=...)``

Furthermore, the following operations are implemented using ``.sorted()``; the original methods remain for convenience.

Previous                        |   Equivalent
------------------------------  |   -------------------------------
``Series.sort_index()``         |   ``Series.sorted(level=True)``
``Series.sortlevel(level=...)`` |   ``Series.sorted(level=...``)
``DataFrame.sort_index()``      |   ``DataFrame.sorted(level=True)``
``DataFrame.sortlevel(level=...)`` | ``DataFrame.sorted(level=...)``

The proposed signature:

```
DataFrame.sorted(self, by=None, axis=0, level=None, ascending=True, inplace=False,
                 kind='quicksort', na_position='last', sort_remaining=True)

Sort by labels (along either axis), by the values in column(s) or both.
If both, labels take precedence over columns. If neither is specified,
behavior is object-dependent: Series = on values, Dataframe = on index.

Parameters
----------
by : column name or list of column names
     if not None, sort on values in specified column name; perform nested
     sort if list of column names specified. this argument ignored by ``Series``
level : int or level name or list of ints or list of level names
     if not None, sort on values in specified index level(s)
axis : %(axes)s to direct sorting
ascending : bool or list of bool
     Sort ascending vs. descending. Specify list for multiple sort orders.
inplace : bool
     if True, perform operation in-place
kind : {`quicksort`, `mergesort`, `heapsort`}
     Choice of sorting algorithm. See also ndarray.np.sort for more information.
     `mergesort` is the only stable algorithm. For DataFrames, this option is
     only applied when sorting on a single column or label.
na_position : {'first', 'last'}
     `first` puts NaNs at the beginning, `last` puts NaNs at the end
sort_remaining : bool
     if true and sorting by level and index is multilevel, sort by other levels
     too (in order) after sorting by specified level
```

Discussion points:

1. Default sorting by labels or values?
   * Proposed PR: keep inconsistency of Series by values and DataFrame by labels.
   * Alternative: As sorting by the values (certainly for a Series) is very convenient, unify the `sorted` method to sort by values/columns for both Series/DataFrame. To have a convenient method to sort by the index, keep the specific `sort_index` method.

     a) Should ``sorted`` still be able to sort on the index? If not (as ``sort_index`` does this), we can leave ``level`` and ``sort_remaining`` keywords out of the signature.

     b) Should ``DataFrame.sorted`` require at least one column to be specified, or should it default to sort the values lexicographically in column order (equivalent to ``df.sorted(by=list(df.columns))``)?

     c) If we go with this clear separation of sorting by index/values in two separate functions, do we use ``sorted``, or something more specific as ``sort_values()`` alongside ``sort_index``

2. The keyword to select the columns to sort?
   * Current PR: chooses `by` over `columns`

3. Should ``Series.sort``, ``Series.order`` and ``DataFrame.sort`` be deprecated?
   * Current PR does deprecate these.
   * As these are widespread functions, a real deprecation/removal can have a large impact, and maybe a clear 'documented' deprecations is enough?

### Integrate ``sortlevel`` into ``sort_index``

Not controversial:

* Add ``level`` and ``sort_remaining`` to ``sort_index()``

Discussion points:

1. Should ``sort_index`` still be able to sort by the columns?
   * It is very strange to use ``sort_index`` to sort by the columns (but even the implementation of ``DataFrame.sort`` uses ``DataFrame.sort_index``).
   * We could deprecate this ability (the deprecating/discouraging the ``by`` keyword)
