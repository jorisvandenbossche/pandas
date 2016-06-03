# This file is distributed under the Bottleneck license:
#
# Copyright (c) 2011 Archipel Asset Management AB
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import numpy as np
cimport numpy as np
import cython
np.import_array()

cdef extern from "prioritylist.h":
    struct pl_node:
       double val
       int death
       pl_node *larger_node
       pl_node *smaller_node
    struct pl_list:
        np.npy_int64 len
        np.npy_intp is_max
        pl_node *head
        pl_node *tail
    void pl_insert_init(pl_list *pl, np.npy_float64 val, np.npy_int64 death) nogil
    void pl_print(pl_list *pl) nogil
    pl_node *pl_get_value(pl_list *pl) nogil
    int pl_remove(pl_list *pl, np.npy_intp *starti, np.npy_intp curval) nogil
    void pl_free(pl_list *pl) nogil
    pl_list *pl_new(np.npy_int64 len, np.npy_intp is_max) nogil
