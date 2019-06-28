"""
Behavioral based tests for offsets and date_range.

This file is adapted from https://github.com/pandas-dev/pandas/pull/18761 -
which was more ambitious but less idiomatic in its use of Hypothesis.

You may wish to consult the previous version for inspiration on further
tests, or when trying to pin down the bugs exposed by the tests below.
"""
import warnings

from hypothesis import assume, given, strategies as st
from hypothesis.extra.dateutil import timezones as dateutil_timezones
from hypothesis.extra.pytz import timezones as pytz_timezones
import pytest

import pandas as pd

from pandas.tseries.offsets import (
    BMonthBegin,
    BMonthEnd,
    BQuarterBegin,
    BQuarterEnd,
    BYearBegin,
    BYearEnd,
    MonthBegin,
    MonthEnd,
    QuarterBegin,
    QuarterEnd,
    YearBegin,
    YearEnd,
)

# ----------------------------------------------------------------
# Helpers for generating random data

with warnings.catch_warnings():
    warnings.simplefilter("ignore")
    min_dt = (pd.Timestamp(1900, 1, 1).to_pydatetime(),)
    max_dt = (pd.Timestamp(1900, 1, 1).to_pydatetime(),)

gen_date_range = st.builds(
    pd.date_range,
    start=st.datetimes(
        # TODO: Choose the min/max values more systematically
        min_value=pd.Timestamp(1900, 1, 1).to_pydatetime(),
        max_value=pd.Timestamp(2100, 1, 1).to_pydatetime(),
    ),
    periods=st.integers(min_value=2, max_value=100),
    freq=st.sampled_from("Y Q M D H T s ms us ns".split()),
    tz=st.one_of(st.none(), dateutil_timezones(), pytz_timezones()),
)

gen_random_datetime = st.datetimes(
    min_value=min_dt,
    max_value=max_dt,
    timezones=st.one_of(st.none(), dateutil_timezones(), pytz_timezones()),
)

# The strategy for each type is registered in conftest.py, as they don't carry
# enough runtime information (e.g. type hints) to infer how to build them.
gen_yqm_offset = st.one_of(
    *map(
        st.from_type,
        [
            MonthBegin,
            MonthEnd,
            BMonthBegin,
            BMonthEnd,
            QuarterBegin,
            QuarterEnd,
            BQuarterBegin,
            BQuarterEnd,
            YearBegin,
            YearEnd,
            BYearBegin,
            BYearEnd,
        ],
    )
)


# ----------------------------------------------------------------
# Offset-specific behaviour tests


# Based on CI runs: Always passes on OSX, fails on Linux, sometimes on Windows
@pytest.mark.xfail(strict=False, reason="inconsistent between OSs, Pythons")
@given(gen_random_datetime, gen_yqm_offset)
def test_on_offset_implementations(dt, offset):
    assume(not offset.normalize)
    # check that the class-specific implementations of onOffset match
    # the general case definition:
    #   (dt + offset) - offset == dt
    compare = (dt + offset) - offset
    assert offset.onOffset(dt) == (compare == dt)


@pytest.mark.xfail(
    reason="res_v2 below is incorrect, needs to use the "
    "commented-out version with tz_localize.  "
    "But with that fix in place, hypothesis then "
    "has errors in timezone generation."
)
@given(gen_yqm_offset, gen_date_range)
def test_apply_index_implementations(offset, rng):
    # offset.apply_index(dti)[i] should match dti[i] + offset
    assume(offset.n != 0)  # TODO: test for that case separately

    # rng = pd.date_range(start='1/1/2000', periods=100000, freq='T')
    ser = pd.Series(rng)

    res = rng + offset
    res_v2 = offset.apply_index(rng)
    # res_v2 = offset.apply_index(rng.tz_localize(None)).tz_localize(rng.tz)
    assert (res == res_v2).all()

    assert res[0] == rng[0] + offset
    assert res[-1] == rng[-1] + offset
    res2 = ser + offset
    # apply_index is only for indexes, not series, so no res2_v2
    assert res2.iloc[0] == ser.iloc[0] + offset
    assert res2.iloc[-1] == ser.iloc[-1] + offset
    # TODO: Check randomly assorted entries, not just first/last


@pytest.mark.xfail  # TODO: reason?
@given(gen_yqm_offset)
def test_shift_across_dst(offset):
    # GH#18319 check that 1) timezone is correctly normalized and
    # 2) that hour is not incorrectly changed by this normalization
    # Note that dti includes a transition across DST boundary
    dti = pd.date_range(
        start="2017-10-30 12:00:00",
        end="2017-11-06",
        freq="D",
        tz="US/Eastern",
    )
    assert (dti.hour == 12).all()  # we haven't screwed up yet

    res = dti + offset
    assert (res.hour == 12).all()
