"""
Functions for defining unary operations.
"""
from typing import Any

from pandas._typing import ArrayLike

from pandas.core.dtypes.generic import ABCExtensionArray

import numpy as np


def should_extension_dispatch(left: ArrayLike, right: Any) -> bool:
    """
    Identify cases where Series operation should dispatch to ExtensionArray method.

    Parameters
    ----------
    left : np.ndarray or ExtensionArray
    right : object

    Returns
    -------
    bool
    """
    return not isinstance(left, np.ndarray) or not isinstance(right, np.ndarray)
